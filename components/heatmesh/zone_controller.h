#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <string>

namespace esphome {
namespace heatmesh {

class ZoneClimate;

class ValveStartupCoordinator {
 public:
  void update(uint32_t now_ms);
  bool can_start() const { return !waiting_; }
  void record_start(uint32_t now_ms) {
    waiting_ = true;
    last_start_ms_ = now_ms;
  }

 private:
  bool waiting_{false};
  uint32_t last_start_ms_{0};
};

class ZoneController {
 public:
  // Stores the zone's sensor, output, control parameters, and boost duration.
  ZoneController(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                 switch_::Switch *valve_output, float target_temperature,
                 uint32_t sensor_timeout_ms, float kp, float ki,
                 uint32_t control_period_ms, uint32_t pwm_period_min, uint32_t min_on_time_ms,
                 uint32_t min_off_time_ms, uint32_t boost_time_ms);

  // Initializes the output, sensor callback, diagnostics, and restored climate state.
  void setup();
  // Updates sensor validity, PI demand, valve output, boost, and periodic diagnostics.
  void loop();
  // Forces the valve off and marks the zone ready for shutdown.
  void shutdown();
  // Changes the target temperature and applies the matching PI reset or boost behavior.
  void set_target_temperature(float temp);
  // Enables heating in HEAT mode or forces the output off in OFF mode.
  void set_enabled(bool enabled);
  // Configures shared first-start spacing and the initial PWM period.
  void configure_startup(ValveStartupCoordinator *coordinator, std::size_t index, std::size_t count);
  // Reports whether this zone is in HEAT mode.
  bool is_enabled() const { return enabled_; }

  // Attaches the optional climate entity created from the zone YAML.
  void set_climate(ZoneClimate *climate) { climate_ = climate; }
  // Attaches the optional heat-demand diagnostic sensor.
  void set_heat_demand_sensor(sensor::Sensor *sensor) { heat_demand_sensor_ = sensor; }
  // Attaches the optional sensor-age diagnostic sensor.
  void set_sensor_age_sensor(sensor::Sensor *sensor) { sensor_age_sensor_ = sensor; }
  // Attaches the optional valve-switch-count diagnostic sensor.
  void set_valve_switch_count_sensor(sensor::Sensor *sensor) { valve_switch_count_sensor_ = sensor; }
  // Attaches the optional sensor-validity diagnostic binary sensor.
  void set_sensor_valid_binary_sensor(binary_sensor::BinarySensor *sensor) { sensor_valid_binary_sensor_ = sensor; }
  // Attaches the optional commanded-valve-state diagnostic binary sensor.
  void set_valve_state_binary_sensor(binary_sensor::BinarySensor *sensor) { valve_state_binary_sensor_ = sensor; }

  // Returns the configured target temperature in degrees Celsius.
  float get_target_temperature() const { return target_temperature_; }
  // Returns the most recently received room temperature in degrees Celsius.
  float get_current_temperature() const { return current_temperature_; }
  // Returns the current PI heat demand as a percentage.
  float get_heat_demand_percent() const { return heat_demand_; }
  // Electrical output command, NOT measured mechanical valve position.
  bool get_valve_state() const { return valve_on_; }
  // Returns the age of the last valid temperature sample in milliseconds.
  uint32_t get_sensor_age_ms() const { return sensor_age_ms_; }
  // Reports whether the latest temperature sample is fresh and valid.
  bool get_sensor_valid() const { return sensor_valid_; }
  // Reports whether the temporary setpoint boost is active.
  bool is_boost_active() const { return boost_active_; }
  // Returns the number of commanded valve state changes since boot.
  uint32_t get_valve_switch_count() const { return valve_switch_count_; }
  // Returns the stable identifier configured for this zone.
  const std::string &get_zone_id() const { return zone_id_; }

 private:
  // Processes a new temperature publication and immediately handles invalid samples.
  void on_temperature_(float value);
  // Updates sample age and transitions sensor validity when the timeout expires.
  void update_sensor_input_(uint32_t now_ms);
  // Calculates bounded PI heat demand and applies anti-windup.
  void run_pi_controller_(uint32_t dt_ms);
  // Converts current demand into a time-proportional valve command.
  void handle_timeproportional_output_(uint32_t now_ms);
  // Calculates the next PWM ON duration, retaining pulses below the minimum.
  uint32_t next_pwm_on_time_ms_(float wanted_ms, uint32_t period_ms);
  // Checks the shared first-start spacing without reserving a slot.
  bool first_start_allowed_();
  // Applies an output transition and publishes associated diagnostics.
  void apply_valve_state_(bool state, uint32_t now_ms);
  // Clears heating demand and PI/PWM state, then forces the output off.
  void stop_heating_(uint32_t now_ms);
  // Starts a timed boost when the zone is enabled, valid, and below setpoint.
  void start_boost_if_below_setpoint_();
  // Ends the active boost and schedules a fresh PI evaluation.
  void end_boost_(const char *reason);
  // Publishes optional demand and sensor-age entities at their configured interval.
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
  uint32_t min_off_time_ms_;
  uint32_t boost_time_ms_;
  uint32_t first_pwm_period_ms_;

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
  bool first_pwm_period_pending_{true};
  bool first_valve_start_done_{false};
  bool startup_wait_logged_{false};
  bool boost_active_{false};
  bool boost_waiting_for_valve_{false};
  bool min_off_waiting_{false};
  bool min_off_logged_{false};
  bool off_period_elapsed_{false};
  uint32_t boost_start_ms_{0};
  uint32_t sensor_age_ms_{UINT32_MAX};
  uint32_t last_sensor_update_ms_{0};
  uint32_t last_control_ms_{0};
  uint32_t last_valve_change_ms_{0};
  uint32_t valve_switch_count_{0};
  uint32_t pwm_on_time_ms_{0};
  uint32_t active_pwm_period_ms_{0};
  float pwm_carry_ms_{0.0f};
  float cycle_start_demand_{0.0f};
  float cycle_carry_before_ms_{0.0f};
  float cycle_carry_applied_ms_{0.0f};
  uint32_t last_pwm_cycle_start_ms_{0};
  bool enabled_{true};
  uint32_t last_publish_ms_{0};

  ValveStartupCoordinator *startup_coordinator_{nullptr};
  ZoneClimate *climate_{nullptr};
  sensor::Sensor *heat_demand_sensor_{nullptr};
  sensor::Sensor *sensor_age_sensor_{nullptr};
  sensor::Sensor *valve_switch_count_sensor_{nullptr};
  binary_sensor::BinarySensor *sensor_valid_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *valve_state_binary_sensor_{nullptr};
};

}  // namespace heatmesh
}  // namespace esphome
