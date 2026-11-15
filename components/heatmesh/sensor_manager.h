#pragma once

#include "esphome/components/sensor/sensor.h"
#include <cmath>
#include <cstdint>
#include <map>
#include <string>

namespace esphome {
namespace heatmesh {

// Kept for the shared-sensor work planned for v0.3. Local zones do not use this registry.
class SensorManager {
 public:
  struct SensorReading {
    float temperature{NAN};
    float humidity{NAN};  // Not supplied by this temperature-only registry yet.
    uint32_t age_ms{UINT32_MAX};
    uint32_t sample_time_ms{0};
    bool valid{false};  // Finite sample received, not a freshness guarantee.
  };
  // Registers a sensor and records each finite publication's value and timestamp.
  void register_sensor(const std::string &sensor_id, sensor::Sensor *sensor);
  // Kept for compatibility; registered sensor callbacks update readings directly.
  void loop() {}
  // Returns the latest reading and its age, or the default invalid reading if absent.
  SensorReading get_reading(const std::string &sensor_id);
  // Reports whether a registered sensor has a valid sample younger than the timeout.
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
