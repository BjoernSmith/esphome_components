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

ZoneController::ZoneController(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                               switch_::Switch *valve_output, float target_temperature,
                               uint32_t sensor_timeout_ms, float kp, float ki,
                               uint32_t control_period_ms, uint32_t pwm_period_min,
                               uint32_t min_on_time_ms)
    : zone_id_(zone_id), temperature_sensor_(temperature_sensor), valve_output_(valve_output),
      target_temperature_(target_temperature), sensor_timeout_ms_(sensor_timeout_ms),
      kp_(kp), ki_(ki), control_period_ms_(control_period_ms),
      pwm_period_ms_(pwm_period_min * 60000UL), min_on_time_ms_(min_on_time_ms) {}

void ZoneController::setup() {
  if (setup_complete_)
    return;
  setup_complete_ = true;
  last_control_ms_ = millis();
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
  ESP_LOGI(TAG, "[%s] Initialized: target=%.1f C, timeout=%" PRIu32 " ms, kp=%.3f, ki=%.4f; output OFF",
           zone_id_.c_str(), target_temperature_, sensor_timeout_ms_, kp_, ki_);

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

void ZoneController::loop() {
  if (!setup_complete_)
    return;
  const uint32_t now_ms = millis();
  update_sensor_input_(now_ms);
  if (!sensor_valid_ || !enabled_) {
    stop_heating_(now_ms);
  } else {
    const uint32_t dt_ms = now_ms - last_control_ms_;
    if (control_due_ || dt_ms >= control_period_ms_) {
      // At startup/recovery/setpoint change, evaluate P without inventing elapsed I time.
      run_pi_controller_(control_due_ ? 0 : std::min(dt_ms, control_period_ms_));
      control_due_ = false;
      last_control_ms_ = now_ms;
    }
    handle_timeproportional_output_(now_ms);
  }
  publish_periodic_(now_ms);
}

void ZoneController::publish_periodic_(uint32_t now_ms) {
  if (now_ms - last_publish_ms_ < DIAGNOSTIC_PUBLISH_INTERVAL_MS)
    return;
  last_publish_ms_ = now_ms;
  if (heat_demand_sensor_ != nullptr)
    heat_demand_sensor_->publish_state(heat_demand_);
  if (sensor_age_sensor_ != nullptr)
    sensor_age_sensor_->publish_state(sensor_age_ms_ == UINT32_MAX ? NAN : sensor_age_ms_ / 1000.0f);
}

void ZoneController::stop_heating_(uint32_t now_ms) {
  heat_demand_ = 0.0f;
  integrator_ = 0.0f;
  pwm_on_time_ms_ = 0;
  pwm_carry_ms_ = 0.0f;
  pwm_started_ = false;
  control_due_ = true;
  last_control_ms_ = now_ms;
  apply_valve_state_(false, now_ms);
}

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

void ZoneController::handle_timeproportional_output_(uint32_t now_ms) {
  if (heat_demand_ <= 0.0f || pwm_period_ms_ == 0) {
    pwm_started_ = false;
    pwm_on_time_ms_ = 0;
    pwm_carry_ms_ = 0.0f;
    apply_valve_state_(false, now_ms);
    return;
  }
  uint32_t elapsed_ms = now_ms - last_pwm_cycle_start_ms_;
  if (!pwm_started_ || elapsed_ms >= pwm_period_ms_) {
    // Start the first cycle immediately, not one full PWM period after boot.
    last_pwm_cycle_start_ms_ = now_ms;
    pwm_started_ = true;
    elapsed_ms = 0;
    pwm_on_time_ms_ = next_pwm_on_time_ms_();
  }
  // Positive demand is latched per PWM cycle; zero demand and faults stop immediately.
  apply_valve_state_(elapsed_ms < pwm_on_time_ms_, now_ms);
}

uint32_t ZoneController::next_pwm_on_time_ms_() {
  // Pulses shorter than min_on_time_ms_ cannot move a slow actuator, so they are carried into later cycles.
  const float wanted_ms = (heat_demand_ / 100.0f) * static_cast<float>(pwm_period_ms_) + pwm_carry_ms_;
  if (wanted_ms < static_cast<float>(min_on_time_ms_)) {
    pwm_carry_ms_ = wanted_ms;
    return 0;
  }
  const float on_ms = std::min(wanted_ms, static_cast<float>(pwm_period_ms_));
  pwm_carry_ms_ = wanted_ms - on_ms;
  return static_cast<uint32_t>(on_ms);
}

void ZoneController::apply_valve_state_(bool state, uint32_t now_ms) {
  if (state != valve_on_) {
    valve_on_ = state;
    ++valve_switch_count_;
    last_valve_change_ms_ = now_ms;
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

void ZoneController::set_target_temperature(float temp) {
  if (!std::isfinite(temp)) {
    ESP_LOGW(TAG, "[%s] Ignoring non-finite setpoint", zone_id_.c_str());
    return;
  }
  temp = std::max(5.0f, std::min(30.0f, temp));
  if (target_temperature_ == temp)
    return;
  ESP_LOGI(TAG, "[%s] Setpoint: %.1f -> %.1f C", zone_id_.c_str(), target_temperature_, temp);
  target_temperature_ = temp;
  // A setpoint change is not a fault: keep the integrator and the running PWM cycle.
  control_due_ = true;
}

void ZoneController::set_enabled(bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  ESP_LOGI(TAG, "[%s] Mode %s", zone_id_.c_str(), enabled ? "HEAT" : "OFF; output forced OFF");
  if (enabled) {
    control_due_ = true;
  } else {
    stop_heating_(millis());
  }
}

void ZoneController::shutdown() {
  stop_heating_(millis());
  setup_complete_ = false;
}

}  // namespace heatmesh
}  // namespace esphome
