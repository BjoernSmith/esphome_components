#include "zone_controller.h"
#include "esphome/core/log.h"

static const char* const TAG = "heatmesh.zone";

namespace esphome {
namespace heatmesh {

ZoneController::ZoneController(
    const std::string& zone_id,
    sensor::Sensor* temperature_sensor,
    switch_::Switch* valve_output,
    float target_temperature,
    uint32_t sensor_timeout_ms,
    float kp,
    float ki,
    uint32_t control_period_ms,
    uint32_t pwm_period_min
)
    : zone_id_(zone_id),
      temperature_sensor_(temperature_sensor),
      valve_output_(valve_output),
      target_temperature_(target_temperature),
      sensor_timeout_ms_(sensor_timeout_ms),
      kp_(kp),
      ki_(ki),
      control_period_ms_(control_period_ms),
      pwm_period_min_(pwm_period_min) {
  pwm_period_ms_ = pwm_period_min * 60000;  // Convert minutes to ms
  last_control_ms_ = millis();
  last_pwm_cycle_start_ms_ = millis();
}

void ZoneController::loop() {
  uint32_t now_ms = millis();
  
  // Update sensor input (freshness, age)
  update_sensor_input_();
  
  // Run PI controller at the specified interval
  if (now_ms - last_control_ms_ >= control_period_ms_) {
    uint32_t dt_ms = now_ms - last_control_ms_;
    run_pi_controller_(dt_ms);
    last_control_ms_ = now_ms;
  }
  
  // Handle timeproportional output switching
  handle_timeproportional_output_(now_ms);
}

void ZoneController::update_sensor_input_() {
  if (temperature_sensor_ == nullptr) {
    sensor_valid_ = false;
    sensor_age_ms_ = UINT32_MAX;
    return;
  }
  
  uint32_t now_ms = millis();
  
  // If sensor has a new state, update reading and timestamp
  if (temperature_sensor_->has_state()) {
    current_temperature_ = temperature_sensor_->get_state();
    last_sensor_update_ms_ = now_ms;
    sensor_age_ms_ = 0;
  } else {
    // Sensor hasn't provided data yet
    sensor_age_ms_ = UINT32_MAX;
  }
  
  // Calculate age since last update
  if (last_sensor_update_ms_ > 0) {
    sensor_age_ms_ = now_ms - last_sensor_update_ms_;
  }
  
  // Sensor is valid if age is within timeout
  if (sensor_age_ms_ <= sensor_timeout_ms_) {
    sensor_valid_ = true;
  } else {
    sensor_valid_ = false;
    ESP_LOGW(TAG, "[%s] Sensor timeout, age=%lu ms", zone_id_.c_str(), sensor_age_ms_);
  }
}

void ZoneController::run_pi_controller_(uint32_t dt_ms) {
  float error = 0.0f;
  
  if (!sensor_valid_) {
    // Failsafe mode: if failsafe is HEAT, aim for target; if OFF, aim for 0.
    if (failsafe_mode_ == FailsafeMode::HEAT) {
      error = target_temperature_ - 15.0f;  // Assume cold, request heat
    } else {
      error = 0.0f;  // No heat requested
    }
    ESP_LOGW(TAG, "[%s] FAILSAFE: sensor invalid, heat_demand=%.1f%%", 
             zone_id_.c_str(), heat_demand_);
  } else {
    error = target_temperature_ - current_temperature_;
  }
  
  // Proportional term
  float p_term = kp_ * error;
  
  // Integral term with anti-windup
  integrator_ += ki_ * error * (dt_ms / 1000.0f);
  if (integrator_ > integrator_max_) {
    integrator_ = integrator_max_;
  } else if (integrator_ < integrator_min_) {
    integrator_ = integrator_min_;
  }
  
  // Combine terms
  heat_demand_ = p_term + integrator_;
  
  // Clamp to 0-100%
  if (heat_demand_ < 0.0f) {
    heat_demand_ = 0.0f;
  } else if (heat_demand_ > 100.0f) {
    heat_demand_ = 100.0f;
  }
  
  ESP_LOGD(TAG, "[%s] PI: T=%.2f°C, target=%.1f°C, error=%.2f, p=%.2f, i=%.2f, demand=%.1f%%",
            zone_id_.c_str(), current_temperature_, target_temperature_, 
            error, p_term, integrator_, heat_demand_);
}

void ZoneController::handle_timeproportional_output_(uint32_t now_ms) {
  uint32_t elapsed_in_cycle = now_ms - last_pwm_cycle_start_ms_;
  
  // Check if we've completed a PWM cycle
  if (elapsed_in_cycle >= pwm_period_ms_) {
    last_pwm_cycle_start_ms_ = now_ms;
    
    // Calculate ON-time for this cycle based on current demand
    // E.g., if demand=50%, ON-time = 50% of period
    pwm_on_time_ms_ = (heat_demand_ / 100.0f) * pwm_period_ms_;
    
    // Ensure minimum ON time of 0 (or could enforce a floor)
    if (pwm_on_time_ms_ < 1000 && heat_demand_ > 0.0f) {
      pwm_on_time_ms_ = 1000;  // At least 1 second on
    }
    
    elapsed_in_cycle = 0;
  }
  
  // Determine valve state based on position in PWM cycle
  bool should_be_on = (elapsed_in_cycle < pwm_on_time_ms_);
  
  if (should_be_on != valve_on_) {
    apply_valve_state_(should_be_on, now_ms);
  }
}

void ZoneController::apply_valve_state_(bool new_state, uint32_t now_ms) {
  valve_on_ = new_state;
  valve_switch_count_++;
  
  if (valve_output_ != nullptr) {
    if (new_state) {
      valve_output_->turn_on();
    } else {
      valve_output_->turn_off();
    }
  }
  
  ESP_LOGI(TAG, "[%s] Valve %s (switch #%lu, demand=%.1f%%)",
           zone_id_.c_str(), new_state ? "ON" : "OFF", 
           valve_switch_count_, heat_demand_);
  
  last_valve_change_ms_ = now_ms;
}

void ZoneController::set_target_temperature(float temp) {
  if (temp < 5.0f) temp = 5.0f;
  if (temp > 30.0f) temp = 30.0f;
  
  if (target_temperature_ != temp) {
    ESP_LOGI(TAG, "[%s] Setpoint changed: %.1f°C -> %.1f°C",
             zone_id_.c_str(), target_temperature_, temp);
    target_temperature_ = temp;
    // On setpoint change, reset integrator to avoid windup
    integrator_ = 0.0f;
  }
}

}  // namespace heatmesh
}  // namespace esphome
