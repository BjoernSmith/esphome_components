#pragma once

#include "esphome/components/sensor/sensor.h"
#include <cmath>
#include <cstdint>
#include <map>
#include <string>

namespace esphome {
namespace heatmesh {

// Kept for future shared-sensor work. The local v0.1.1 zone path does not use this registry.
class SensorManager {
 public:
  struct SensorReading {
    float temperature{NAN};
    float humidity{NAN};  // Not supplied by this temperature-only registry yet.
    uint32_t age_ms{UINT32_MAX};
    uint32_t sample_time_ms{0};
    bool valid{false};  // Finite sample received, not a freshness guarantee.
  };
  void register_sensor(const std::string &sensor_id, sensor::Sensor *sensor);
  void loop() {}  // Compatibility: callbacks, not polling, update sample timestamps.
  SensorReading get_reading(const std::string &sensor_id);
  bool is_fresh(const std::string &sensor_id, uint32_t timeout_ms);

 private:
  struct ManagedSensor {
    sensor::Sensor *sensor{nullptr};
    uint32_t last_sample_time_ms{0};
    float last_value{NAN};
    bool has_valid_reading{false};
  };
  std::map<std::string, ManagedSensor> sensors_;
};

}  // namespace heatmesh
}  // namespace esphome
