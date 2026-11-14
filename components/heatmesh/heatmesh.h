#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "zone_controller.h"
#include "sensor_manager.h"
#include <vector>
#include <memory>

namespace esphome {
namespace heatmesh {

struct ZoneConfig {
  std::string zone_id;
  sensor::Sensor* temperature_sensor;
  switch_::Switch* valve_output;
  float target_temperature;
  uint32_t sensor_timeout_ms;
  float kp;
  float ki;
  uint32_t control_period_ms;
  uint32_t pwm_period_min;
};

class HeatmeshComponent : public Component {
 public:
  HeatmeshComponent() = default;

  float get_setup_priority() const override { return setup_priority::BUS - 1.0f; }

  void setup() override;
  void loop() override;
  void dump_config() override;

  void add_zone(
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

  ZoneController* get_zone(const std::string& zone_id);
  size_t get_zone_count() const { return zones_.size(); }

  // Utility for lambda access (set in codegen)
  static HeatmeshComponent* get_instance() { return instance_; }

 private:
  static HeatmeshComponent* instance_;
  std::vector<std::unique_ptr<ZoneController>> zones_;
  std::vector<ZoneConfig> zone_configs_;
  SensorManager sensor_manager_;
};

}  // namespace heatmesh
}  // namespace esphome
