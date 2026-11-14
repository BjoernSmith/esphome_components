#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include <cstdint>
#include <string>

namespace esphome {
namespace heatmesh {

class ZoneController {
 public:
  ZoneController(
      const std::string& zone_id,
      sensor::Sensor* temperature_sensor,
      switch_::Switch* valve_output,
      float target_temperature,
      uint32_t sensor_timeout_ms,
      float kp,
      float ki,
      uint32_t control_period_ms,
      uint32_t pwm_period_min
  );

  void loop();
  void set_target_temperature(float temp);
  float get_target_temperature() const { return target_temperature_; }
  float get_current_temperature() const { return current_temperature_; }
  float get_heat_demand_percent() const { return heat_demand_; }
  bool get_valve_state() const { return valve_on_; }
  uint32_t get_sensor_age_ms() const { return sensor_age_ms_; }
  bool get_sensor_valid() const { return sensor_valid_; }
  uint32_t get_valve_switch_count() const { return valve_switch_count_; }
  const std::string& get_zone_id() const { return zone_id_; }

  enum class FailsafeMode : uint8_t {
    OFF = 0,
    HEAT = 1,
  };
  
  FailsafeMode get_failsafe_mode() const { return failsafe_mode_; }
  void set_failsafe_mode(FailsafeMode mode) { failsafe_mode_ = mode; }

 private:
  std::string zone_id_;
  sensor::Sensor* temperature_sensor_;
  switch_::Switch* valve_output_;

  // Configuration
  float target_temperature_;
  uint32_t sensor_timeout_ms_;
  float kp_;
  float ki_;
  uint32_t control_period_ms_;
  uint32_t pwm_period_min_;

  // State
  float current_temperature_ = 20.0f;
  float heat_demand_ = 0.0f;
  bool valve_on_ = false;
  bool sensor_valid_ = false;
  uint32_t sensor_age_ms_ = 0;
  uint32_t last_sensor_update_ms_ = 0;  // Track when sensor last updated
  uint32_t last_control_ms_ = 0;
  uint32_t last_valve_change_ms_ = 0;
  uint32_t valve_switch_count_ = 0;

  // PI controller state
  float integrator_ = 0.0f;
  float last_error_ = 0.0f;
  const float integrator_max_ = 100.0f;
  const float integrator_min_ = -100.0f;

  // PWM/timeproportional state
  uint32_t pwm_period_ms_ = 900000;  // 15 minutes in ms
  uint32_t pwm_on_time_ms_ = 0;
  uint32_t last_pwm_cycle_start_ms_ = 0;

  FailsafeMode failsafe_mode_ = FailsafeMode::OFF;

  void update_sensor_input_();
  void run_pi_controller_(uint32_t dt_ms);
  void handle_timeproportional_output_(uint32_t now_ms);
  void apply_valve_state_(bool new_state, uint32_t now_ms);
};

}  // namespace heatmesh
}  // namespace esphome
