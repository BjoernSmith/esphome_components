#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include <map>
#include <string>

namespace esphome {
namespace heatmesh {

class SensorManager {
 public:
  SensorManager() = default;
  
  struct SensorReading {
    float temperature = 0.0f;
    float humidity = 0.0f;
    uint32_t age_ms = UINT32_MAX;
    uint32_t sample_time_ms = 0;
    bool valid = false;
  };

  void register_sensor(const std::string& sensor_id, sensor::Sensor* esphome_sensor);
  void loop();
  
  SensorReading get_reading(const std::string& sensor_id);
  bool is_fresh(const std::string& sensor_id, uint32_t timeout_ms);

 private:
  struct ManagedSensor {
    sensor::Sensor* sensor;
    uint32_t last_sample_time_ms = 0;
    float last_value = 0.0f;
    bool has_valid_reading = false;
  };
  
  std::map<std::string, ManagedSensor> sensors_;
};

}  // namespace heatmesh
}  // namespace esphome
