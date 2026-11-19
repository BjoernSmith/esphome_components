#include "zone_controller.h"
#include "zone_climate.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cinttypes>

namespace esphome {
namespace heatmesh {

static const char *const TAG = "heatmesh.zone";
static const uint32_t DIAGNOSTIC_PUBLISH_INTERVAL_MS = 10000;
static const uint32_t FIRST_VALVE_START_SPACING_MS = 5 * 60000UL;

void ValveStartupCoordinator::update(uint32_t now_ms) {
  if (waiting_ && uint32_t(now_ms - last_start_ms_) >= FIRST_VALVE_START_SPACING_MS)
    waiting_ = false;
}

// Stores the zone's control dependencies and converts configured periods to milliseconds.
ZoneController::ZoneController(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                               switch_::Switch *valve_output, float target_temperature,
                               uint32_t sensor_timeout_ms, float kp, float ki,
                               uint32_t control_period_ms, uint32_t pwm_period_min,
                               uint32_t min_on_time_ms, uint32_t min_off_time_ms, uint32_t boost_time_ms)
    : zone_id_(zone_id), temperature_sensor_(temperature_sensor), valve_output_(valve_output),
      target_temperature_(target_temperature), sensor_timeout_ms_(sensor_timeout_ms),
      kp_(kp), ki_(ki), control_period_ms_(control_period_ms),
      pwm_period_ms_(pwm_period_min * 60000UL), min_on_time_ms_(min_on_time_ms),
      min_off_time_ms_(min_off_time_ms), boost_time_ms_(boost_time_ms),
      first_pwm_period_ms_(pwm_period_min * 60000UL) {}

void ZoneController::configure_startup(ValveStartupCoordinator *coordinator, std::size_t index, std::size_t count) {
  startup_coordinator_ = coordinator;
  // Spread the first cycle ends without allowing a pulse shorter than min_on_time.
  const uint32_t staggered_period =
      static_cast<uint32_t>(static_cast<uint64_t>(pwm_period_ms_) * (count - index) / count);
  first_pwm_period_ms_ = std::max(std::max(min_on_time_ms_, uint32_t(1)), staggered_period);
}

// Starts the valve safely, subscribes to temperature updates, and restores climate state.
void ZoneController::setup() {
  if (setup_complete_)
    return;
  setup_complete_ = true;
  last_control_ms_ = millis();
  last_valve_change_ms_ = last_control_ms_;
  min_off_waiting_ = false;
  min_off_logged_ = false;
  if (valve_output_ != nullptr)
    valve_output_->turn_off();
  if (temperature_sensor_ != nullptr) {
    temperature_sensor_->add_on_state_callback([this](float value) { this->on_temperature_(value); });
  } else {
    ESP_LOGE(TAG, "[%s] Missing temperature sensor", zone_id_.c_str());
  }
  if (valve_output_ == nullptr)
    ESP_LOGE(TAG, "[%s] Missing valve output", zone_id_.c_str());
  // Deliberately do not stamp a pre-existing cached sensor state as 'new'.
  ESP_LOGI(TAG, "[%s] Initialized: target=%.1f C, timeout=%" PRIu32 " ms, kp=%.3f, ki=%.4f, min_off=%" PRIu32
                " ms, boost=%" PRIu32 " s; output OFF",
           zone_id_.c_str(), target_temperature_, sensor_timeout_ms_, kp_, ki_, min_off_time_ms_,
           boost_time_ms_ / 1000);

  if (sensor_valid_binary_sensor_ != nullptr)
    sensor_valid_binary_sensor_->publish_initial_state(false);
  if (valve_state_binary_sensor_ != nullptr)
    valve_state_binary_sensor_->publish_initial_state(false);
  if (valve_switch_count_sensor_ != nullptr)
    valve_switch_count_sensor_->publish_state(valve_switch_count_);
  // Publish the remaining diagnostics on the first loop.
  last_publish_ms_ = last_control_ms_ - DIAGNOSTIC_PUBLISH_INTERVAL_MS;
  if (climate_ != nullptr)
    climate_->restore();
}

// Accepts fresh sensor samples or forces the zone safe when a value is invalid.
void ZoneController::on_temperature_(float value) {
  if (!std::isfinite(value)) {
    sample_valid_ = false;
    current_temperature_ = NAN;
    // Invalid publications do not refresh the last valid measurement timestamp.
    ESP_LOGW(TAG, "[%s] Invalid temperature; forcing output OFF", zone_id_.c_str());
    sensor_valid_ = false;
    if (sensor_valid_binary_sensor_ != nullptr)
      sensor_valid_binary_sensor_->publish_state(false);
    stop_heating_(millis());
    if (climate_ != nullptr)
      climate_->sync_from_zone();
    return;
  }
  current_temperature_ = value;
  last_sensor_update_ms_ = millis();
  sensor_age_ms_ = 0;
  ever_received_ = true;
  sample_valid_ = true;
  ESP_LOGD(TAG, "[%s] Temperature update: %.2f C", zone_id_.c_str(), value);
  if (climate_ != nullptr)
    climate_->sync_from_zone();
}

// Recomputes sample age and latches sensor validity or timeout transitions.
void ZoneController::update_sensor_input_(uint32_t now_ms) {
  sensor_age_ms_ = ever_received_ ? uint32_t(now_ms - last_sensor_update_ms_) : UINT32_MAX;
  const bool valid = temperature_sensor_ != nullptr && sample_valid_ &&
                     std::isfinite(current_temperature_) && sensor_age_ms_ < sensor_timeout_ms_;
  if (!valid && sample_valid_) {
    // Latch expiry. millis() wrapping again must not revive an ancient reading.
    sample_valid_ = false;
  }
  if (valid != sensor_valid_) {
    if (valid) {
      ESP_LOGI(TAG, "[%s] Sensor valid; local control enabled", zone_id_.c_str());
      control_due_ = true;
      last_control_ms_ = now_ms;
    } else {
      ESP_LOGW(TAG, "[%s] Sensor timeout/invalid (age=%" PRIu32 " ms); output OFF",
               zone_id_.c_str(), sensor_age_ms_);
    }
    if (sensor_valid_binary_sensor_ != nullptr)
      sensor_valid_binary_sensor_->publish_state(valid);
  }
  sensor_valid_ = valid;
}

// Runs sensor supervision, boost timing, PI control, output scheduling, and diagnostics.
void ZoneController::loop() {
  if (!setup_complete_)
    return;
  const uint32_t now_ms = millis();
  if (!valve_on_ && !off_period_elapsed_ && uint32_t(now_ms - last_valve_change_ms_) >= pwm_period_ms_)
    off_period_elapsed_ = true;
  if (min_off_waiting_ && uint32_t(now_ms - last_valve_change_ms_) >= min_off_time_ms_) {
    min_off_waiting_ = false;
    min_off_logged_ = false;
  }
  update_sensor_input_(now_ms);
  if (!sensor_valid_ || !enabled_) {
    stop_heating_(now_ms);
  } else {
    if (boost_active_) {
      const bool setpoint_reached = !(current_temperature_ < target_temperature_);
      // A boost pulse that has started is held for min_on_time; one that has not started is simply cancelled.
      const bool min_on_done = !valve_on_ || uint32_t(now_ms - last_valve_change_ms_) >= min_on_time_ms_;
      if (setpoint_reached && min_on_done) {
        end_boost_("setpoint reached");
      } else if (!boost_waiting_for_valve_ && now_ms - boost_start_ms_ >= boost_time_ms_) {
        end_boost_("boost time elapsed; PI takes over");
      }
    }
    const uint32_t dt_ms = now_ms - last_control_ms_;
    if (control_due_ || dt_ms >= control_period_ms_) {
      // At startup/recovery/setpoint change, evaluate P without inventing elapsed I time.
      // During boost the output is forced ON, so I is frozen to avoid overshoot afterwards.
      run_pi_controller_((control_due_ || boost_active_) ? 0 : std::min(dt_ms, control_period_ms_));
      control_due_ = false;
      last_control_ms_ = now_ms;
    }
    if (boost_active_) {
      apply_valve_state_(true, now_ms);
    } else {
      handle_timeproportional_output_(now_ms);
    }
  }
  publish_periodic_(now_ms);
}

// Publishes optional heat-demand and sensor-age values at a fixed interval.
void ZoneController::publish_periodic_(uint32_t now_ms) {
  if (now_ms - last_publish_ms_ < DIAGNOSTIC_PUBLISH_INTERVAL_MS)
    return;
  last_publish_ms_ = now_ms;
  if (heat_demand_sensor_ != nullptr)
    heat_demand_sensor_->publish_state(heat_demand_);
  if (sensor_age_sensor_ != nullptr)
    sensor_age_sensor_->publish_state(sensor_age_ms_ == UINT32_MAX ? NAN : sensor_age_ms_ / 1000.0f);
}

// Resets heating state and forces the valve output off.
void ZoneController::stop_heating_(uint32_t now_ms) {
  if (boost_active_)
    end_boost_("output forced OFF");
  heat_demand_ = 0.0f;
  integrator_ = 0.0f;
  pwm_on_time_ms_ = 0;
  pwm_carry_ms_ = 0.0f;
  pwm_started_ = false;
  control_due_ = true;
  last_control_ms_ = now_ms;
  apply_valve_state_(false, now_ms);
}

// Calculates bounded PI demand and prevents integral windup at output limits.
void ZoneController::run_pi_controller_(uint32_t dt_ms) {
  const float error = target_temperature_ - current_temperature_;
  const float p_term = kp_ * error;
  const float delta_i = ki_ * error * (dt_ms / 1000.0f);
  // Conditional anti-windup: integrate up to the output rail, not beyond it.
  // If error changes sign, the integral may always move back out of saturation.
  const float proposed = std::max(-100.0f, std::min(100.0f, integrator_ + delta_i));
  const float proposed_output = p_term + proposed;
  if (proposed_output > 100.0f && delta_i > 0.0f) {
    integrator_ = std::max(integrator_, std::max(-100.0f, std::min(100.0f, 100.0f - p_term)));
  } else if (proposed_output < 0.0f && delta_i < 0.0f) {
    integrator_ = std::min(integrator_, std::max(-100.0f, std::min(100.0f, -p_term)));
  } else {
    integrator_ = proposed;
  }
  heat_demand_ = std::max(0.0f, std::min(100.0f, p_term + integrator_));
  if (!std::isfinite(heat_demand_)) {
    integrator_ = 0.0f;
    heat_demand_ = 0.0f;
  }
  ESP_LOGD(TAG, "[%s] PI: T=%.2f C, target=%.1f C, P=%.2f, I=%.2f, demand=%.1f%%",
           zone_id_.c_str(), current_temperature_, target_temperature_, p_term, integrator_, heat_demand_);
}

// Schedules the valve pulse for the active PWM period based on heat demand.
void ZoneController::handle_timeproportional_output_(uint32_t now_ms) {
  if (heat_demand_ <= 0.0f || pwm_period_ms_ == 0) {
    pwm_started_ = false;
    pwm_on_time_ms_ = 0;
    pwm_carry_ms_ = 0.0f;
    apply_valve_state_(false, now_ms);
    return;
  }
  uint32_t elapsed_ms = now_ms - last_pwm_cycle_start_ms_;
  bool cycle_due = !pwm_started_ || elapsed_ms >= active_pwm_period_ms_;
  if (pwm_started_ && !cycle_due) {
    const float adjusted_on_ms = std::min(static_cast<float>(pwm_on_time_ms_),
        (heat_demand_ / 100.0f) * static_cast<float>(active_pwm_period_ms_) + cycle_carry_applied_ms_);
    const bool pulse_active = valve_on_ &&
        (static_cast<float>(elapsed_ms) < adjusted_on_ms ||
         uint32_t(now_ms - last_valve_change_ms_) < min_on_time_ms_);
    if (!pulse_active && !min_off_waiting_) {
      float carry_ms = pwm_carry_ms_;
      if (pwm_on_time_ms_ == 0) {
        // A skipped cycle has only earned the fraction of its carry that has elapsed.
        carry_ms = cycle_carry_before_ms_ + (pwm_carry_ms_ - cycle_carry_before_ms_) *
                   static_cast<float>(elapsed_ms) / static_cast<float>(active_pwm_period_ms_);
      }
      const float wanted_ms = (heat_demand_ / 100.0f) * static_cast<float>(pwm_period_ms_) + carry_ms;
      const float required_off_ms = (1.0f - heat_demand_ / 100.0f) * static_cast<float>(pwm_period_ms_);
      float off_ms = 0.0f;
      if (!valve_on_)
        off_ms = off_period_elapsed_ ? static_cast<float>(pwm_period_ms_)
                                     : static_cast<float>(uint32_t(now_ms - last_valve_change_ms_));
      const bool short_cycle_can_restart = pwm_on_time_ms_ != 0 || heat_demand_ > cycle_start_demand_;
      const bool no_forced_off = !valve_on_ || min_off_time_ms_ == 0;
      if (short_cycle_can_restart && no_forced_off && wanted_ms >= static_cast<float>(min_on_time_ms_) &&
          wanted_ms >= 1.0f && off_ms >= required_off_ms && first_start_allowed_()) {
        pwm_carry_ms_ = carry_ms;
        cycle_due = true;
      }
    }
    if (!cycle_due) {
      apply_valve_state_(pulse_active, now_ms);
      return;
    }
  }
  if (cycle_due) {
    if (!valve_on_ && min_off_waiting_) {
      if (!min_off_logged_) {
        ESP_LOGI(TAG, "[%s] Waiting for minimum OFF time before starting PWM", zone_id_.c_str());
        min_off_logged_ = true;
      }
      return;
    }
    const uint32_t period_ms = first_pwm_period_pending_ ? first_pwm_period_ms_ : pwm_period_ms_;
    const float wanted_ms = (heat_demand_ / 100.0f) * static_cast<float>(period_ms) + pwm_carry_ms_;
    const bool pulse_due = wanted_ms >= static_cast<float>(min_on_time_ms_) &&
                           std::min(wanted_ms, static_cast<float>(period_ms)) >= 1.0f;
    if (pulse_due && !first_start_allowed_()) {
      apply_valve_state_(false, now_ms);
      return;
    }
    last_pwm_cycle_start_ms_ = now_ms;
    active_pwm_period_ms_ = period_ms;
    first_pwm_period_pending_ = false;
    pwm_started_ = true;
    elapsed_ms = 0;
    cycle_start_demand_ = heat_demand_;
    cycle_carry_before_ms_ = pwm_carry_ms_;
    pwm_on_time_ms_ = next_pwm_on_time_ms_(wanted_ms, period_ms);
    cycle_carry_applied_ms_ = pwm_on_time_ms_ == 0 ? 0.0f : cycle_carry_before_ms_ - pwm_carry_ms_;
  }
  apply_valve_state_(elapsed_ms < pwm_on_time_ms_, now_ms);
}

// Converts demand to an ON duration and carries forward pulses below the minimum.
uint32_t ZoneController::next_pwm_on_time_ms_(float wanted_ms, uint32_t period_ms) {
  // Pulses shorter than min_on_time_ms_ cannot move a slow actuator, so they are carried into later cycles.
  if (wanted_ms < static_cast<float>(min_on_time_ms_)) {
    pwm_carry_ms_ = wanted_ms;
    return 0;
  }
  const float on_ms = std::min(wanted_ms, static_cast<float>(period_ms));
  pwm_carry_ms_ = wanted_ms - on_ms;
  return static_cast<uint32_t>(on_ms);
}

bool ZoneController::first_start_allowed_() {
  if (valve_on_ || first_valve_start_done_ || startup_coordinator_ == nullptr || startup_coordinator_->can_start())
    return true;
  if (!startup_wait_logged_) {
    ESP_LOGI(TAG, "[%s] Waiting for first valve start spacing after reboot", zone_id_.c_str());
    startup_wait_logged_ = true;
  }
  return false;
}

// Applies a valve command and updates switch-count, diagnostics, and climate state.
void ZoneController::apply_valve_state_(bool state, uint32_t now_ms) {
  if (state && !valve_on_ && !first_start_allowed_())
    state = false;
  if (state != valve_on_) {
    if (state && !first_valve_start_done_) {
      first_valve_start_done_ = true;
      startup_wait_logged_ = false;
      if (startup_coordinator_ != nullptr)
        startup_coordinator_->record_start(now_ms);
    }
    if (state && boost_active_ && boost_waiting_for_valve_) {
      boost_start_ms_ = now_ms;
      boost_waiting_for_valve_ = false;
    }
    valve_on_ = state;
    ++valve_switch_count_;
    last_valve_change_ms_ = now_ms;
    off_period_elapsed_ = false;
    min_off_waiting_ = !state && min_off_time_ms_ > 0;
    min_off_logged_ = false;
    ESP_LOGI(TAG, "[%s] Output %s (switch #%" PRIu32 ", demand=%.1f%%)",
             zone_id_.c_str(), state ? "ON" : "OFF", valve_switch_count_, heat_demand_);
    if (valve_state_binary_sensor_ != nullptr)
      valve_state_binary_sensor_->publish_state(state);
    if (valve_switch_count_sensor_ != nullptr)
      valve_switch_count_sensor_->publish_state(valve_switch_count_);
    if (climate_ != nullptr)
      climate_->sync_from_zone();
  }
  // Also undo an accidental external change to the software switch state.
  if (valve_output_ != nullptr && valve_output_->state != state) {
    if (state)
      valve_output_->turn_on();
    else
      valve_output_->turn_off();
  }
}

// Validates and applies a setpoint change, resetting PI or starting boost as needed.
void ZoneController::set_target_temperature(float temp) {
  if (!std::isfinite(temp)) {
    ESP_LOGW(TAG, "[%s] Ignoring non-finite setpoint", zone_id_.c_str());
    return;
  }
  temp = std::max(5.0f, std::min(30.0f, temp));
  if (target_temperature_ == temp)
    return;
  ESP_LOGI(TAG, "[%s] Setpoint: %.1f -> %.1f C", zone_id_.c_str(), target_temperature_, temp);
  const float previous = target_temperature_;
  target_temperature_ = temp;
  control_due_ = true;
  if (!enabled_ || !sensor_valid_)
    return;
  if (!(temp > current_temperature_)) {
    // Setpoint already reached: no heating, and the PI starts from scratch.
    if (boost_active_)
      end_boost_("setpoint already reached");
    integrator_ = 0.0f;
    pwm_carry_ms_ = 0.0f;
    pwm_on_time_ms_ = 0;
    pwm_started_ = false;
    ESP_LOGI(TAG, "[%s] Setpoint <= %.2f C: PI reset, no heating", zone_id_.c_str(), current_temperature_);
  } else if (temp > previous) {
    start_boost_if_below_setpoint_();
  }
}

// Starts the configured immediate-ON interval when the enabled zone is below target.
void ZoneController::start_boost_if_below_setpoint_() {
  if (boost_time_ms_ == 0 || !enabled_ || !sensor_valid_ || !(current_temperature_ < target_temperature_))
    return;
  boost_active_ = true;
  boost_start_ms_ = millis();
  boost_waiting_for_valve_ = !valve_on_;
  // The boost replaces the running PWM cycle; PI starts a fresh cycle when it takes over.
  pwm_started_ = false;
  pwm_on_time_ms_ = 0;
  pwm_carry_ms_ = 0.0f;
  ESP_LOGI(TAG, "[%s] Boost requested for up to %" PRIu32 " s (T=%.2f C, target=%.1f C)", zone_id_.c_str(),
           boost_time_ms_ / 1000, current_temperature_, target_temperature_);
}

// Stops boost and schedules a fresh PI evaluation.
void ZoneController::end_boost_(const char *reason) {
  boost_active_ = false;
  boost_waiting_for_valve_ = false;
  pwm_started_ = false;
  control_due_ = true;
  ESP_LOGI(TAG, "[%s] Boost ended: %s", zone_id_.c_str(), reason);
}

// Switches the zone between HEAT and OFF, starting boost or forcing output off.
void ZoneController::set_enabled(bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  ESP_LOGI(TAG, "[%s] Mode %s", zone_id_.c_str(), enabled ? "HEAT" : "OFF; output forced OFF");
  if (enabled) {
    control_due_ = true;
    start_boost_if_below_setpoint_();
  } else {
    stop_heating_(millis());
  }
}

// Forces the valve off and clears the zone's active lifecycle state.
void ZoneController::shutdown() {
  stop_heating_(millis());
  setup_complete_ = false;
}

}  // namespace heatmesh
}  // namespace esphome
