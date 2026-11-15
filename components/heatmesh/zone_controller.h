#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include <cmath>
#include <cstdint>
#include <string>

namespace esphome {
namespace heatmesh {

class ZoneClimate;

class ZoneController {
 public:
  ZoneController(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                 switch_::Switch *valve_output, float target_temperature,
                 uint32_t sensor_timeout_ms, float kp, float ki,
                 uint32_t control_period_ms, uint32_t pwm_period_min, uint32_t min_on_time_ms);

  void setup();
  void loop();
  void shutdown();
  void set_target_temperature(float temp);
  // false = climate mode OFF: output forced OFF, sensor supervision continues.
  void set_enabled(bool enabled);
  bool is_enabled() const { return enabled_; }

  // Optional Home Assistant entities, created from the zone YAML.
  void set_climate(ZoneClimate *climate) { climate_ = climate; }
  void set_heat_demand_sensor(sensor::Sensor *sensor) { heat_demand_sensor_ = sensor; }
  void set_sensor_age_sensor(sensor::Sensor *sensor) { sensor_age_sensor_ = sensor; }
  void set_valve_switch_count_sensor(sensor::Sensor *sensor) { valve_switch_count_sensor_ = sensor; }
  void set_sensor_valid_binary_sensor(binary_sensor::BinarySensor *sensor) { sensor_valid_binary_sensor_ = sensor; }
  void set_valve_state_binary_sensor(binary_sensor::BinarySensor *sensor) { valve_state_binary_sensor_ = sensor; }

  float get_target_temperature() const { return target_temperature_; }
  float get_current_temperature() const { return current_temperature_; }
  float get_heat_demand_percent() const { return heat_demand_; }
  // Electrical output command, NOT measured mechanical valve position.
  bool get_valve_state() const { return valve_on_; }
  uint32_t get_sensor_age_ms() const { return sensor_age_ms_; }
  bool get_sensor_valid() const { return sensor_valid_; }
  uint32_t get_valve_switch_count() const { return valve_switch_count_; }
  const std::string &get_zone_id() const { return zone_id_; }

 private:
  void on_temperature_(float value);
  void update_sensor_input_(uint32_t now_ms);
  void run_pi_controller_(uint32_t dt_ms);
  void handle_timeproportional_output_(uint32_t now_ms);
  uint32_t next_pwm_on_time_ms_();
  void apply_valve_state_(bool state, uint32_t now_ms);
  void stop_heating_(uint32_t now_ms);
  void publish_periodic_(uint32_t now_ms);

  std::string zone_id_;
  sensor::Sensor *temperature_sensor_;
  switch_::Switch *valve_output_;
  float target_temperature_;
  uint32_t sensor_timeout_ms_;
  float kp_;
  float ki_;
  uint32_t control_period_ms_;
  uint32_t pwm_period_ms_;
  uint32_t min_on_time_ms_;

  float current_temperature_{NAN};
  float heat_demand_{0.0f};
  float integrator_{0.0f};
  bool valve_on_{false};
  bool sensor_valid_{false};
  bool ever_received_{false};
  bool sample_valid_{false};
  bool setup_complete_{false};
  bool control_due_{true};
  bool pwm_started_{false};
  uint32_t sensor_age_ms_{UINT32_MAX};
  uint32_t last_sensor_update_ms_{0};
  uint32_t last_control_ms_{0};
  uint32_t last_valve_change_ms_{0};
  uint32_t valve_switch_count_{0};
  uint32_t pwm_on_time_ms_{0};
  float pwm_carry_ms_{0.0f};
  uint32_t last_pwm_cycle_start_ms_{0};
  bool enabled_{true};
  uint32_t last_publish_ms_{0};

  ZoneClimate *climate_{nullptr};
  sensor::Sensor *heat_demand_sensor_{nullptr};
  sensor::Sensor *sensor_age_sensor_{nullptr};
  sensor::Sensor *valve_switch_count_sensor_{nullptr};
  binary_sensor::BinarySensor *sensor_valid_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *valve_state_binary_sensor_{nullptr};
};

}  // namespace heatmesh
}  // namespace esphome
