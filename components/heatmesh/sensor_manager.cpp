#include "sensor_manager.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace heatmesh {

static const char *const TAG = "heatmesh.sensor_mgr";

// Registers a sensor and timestamps each finite state published by it.
void SensorManager::register_sensor(const std::string &sensor_id, sensor::Sensor *sensor) {
  if (sensor == nullptr || sensors_.count(sensor_id) != 0) {
    ESP_LOGW(TAG, "Cannot register null/duplicate sensor: %s", sensor_id.c_str());
    return;
  }
  ManagedSensor entry;
  entry.sensor = sensor;
  sensors_.emplace(sensor_id, entry);
  // Updates the cached reading whenever ESPHome publishes a new sensor value.
  sensor->add_on_state_callback([this, sensor_id](float value) {
    auto &sample = sensors_.at(sensor_id);
    sample.last_value = value;
    sample.has_valid_reading = std::isfinite(value);
    if (sample.has_valid_reading)
      sample.last_sample_time_ms = millis();
  });
}

// Returns the cached reading and current age, or an invalid default if unavailable.
SensorManager::SensorReading SensorManager::get_reading(const std::string &sensor_id) {
  SensorReading result;
  auto it = sensors_.find(sensor_id);
  if (it == sensors_.end() || !it->second.has_valid_reading)
    return result;
  const auto &sample = it->second;
  result.temperature = sample.last_value;
  result.sample_time_ms = sample.last_sample_time_ms;
  result.age_ms = uint32_t(millis() - sample.last_sample_time_ms);
  result.valid = true;
  return result;
}

// Checks that a registered sensor has a valid sample younger than the timeout.
bool SensorManager::is_fresh(const std::string &sensor_id, uint32_t timeout_ms) {
  const auto reading = get_reading(sensor_id);
  return reading.valid && reading.age_ms < timeout_ms;
}

}  // namespace heatmesh
}  // namespace esphome
