#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "zone_controller.h"
#include <memory>
#include <string>
#include <vector>

namespace esphome {
namespace heatmesh {

struct ZoneConfig {
  std::string zone_id;
  sensor::Sensor *temperature_sensor;
  switch_::Switch *valve_output;
  float target_temperature;
  uint32_t sensor_timeout_ms;
  float kp;
  float ki;
  uint32_t control_period_ms;
  uint32_t pwm_period_min;
  uint32_t min_on_time_ms;
};

class HeatmeshComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  // After I/O hardware, before network/BLE starts delivering measurements.
  float get_setup_priority() const override { return setup_priority::DATA; }

  void add_zone(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                switch_::Switch *valve_output, float target_temperature,
                uint32_t sensor_timeout_ms, float kp, float ki,
                uint32_t control_period_ms, uint32_t pwm_period_min, uint32_t min_on_time_ms);

  ZoneController *get_zone(const std::string &zone_id);
  size_t get_zone_count() const { return zones_.size(); }

  // Backward compatibility only. New YAML should use id(heatmesh_controller).
  static HeatmeshComponent *get_instance() { return instance_; }

 private:
  static HeatmeshComponent *instance_;
  std::vector<std::unique_ptr<ZoneController>> zones_;
  std::vector<ZoneConfig> zone_configs_;
  bool setup_complete_{false};
};

}  // namespace heatmesh
}  // namespace esphome
