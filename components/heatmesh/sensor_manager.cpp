#include "sensor_manager.h"
#include "esphome/core/log.h"

static const char* const TAG = "heatmesh.sensor_mgr";

namespace esphome {
namespace heatmesh {

void SensorManager::register_sensor(const std::string& sensor_id, sensor::Sensor* esphome_sensor) {
  if (esphome_sensor == nullptr) {
    ESP_WARN(TAG, "Cannot register null sensor for %s", sensor_id.c_str());
    return;
  }
  
  ManagedSensor ms;
  ms.sensor = esphome_sensor;
  ms.has_valid_reading = false;
  sensors_[sensor_id] = ms;
  
  ESP_INFO(TAG, "Registered sensor: %s", sensor_id.c_str());
}

void SensorManager::loop() {
  uint32_t now_ms = millis();
  
  for (auto& [sensor_id, ms] : sensors_) {
    if (ms.sensor == nullptr) {
      continue;
    }
    
    if (ms.sensor->has_state()) {
      float new_value = ms.sensor->get_state();
      ms.last_value = new_value;
      ms.last_sample_time_ms = now_ms;
      ms.has_valid_reading = true;
    }
  }
}

SensorManager::SensorReading SensorManager::get_reading(const std::string& sensor_id) {
  SensorReading result;
  
  auto it = sensors_.find(sensor_id);
  if (it == sensors_.end()) {
    ESP_WARN(TAG, "Sensor %s not found in registry", sensor_id.c_str());
    return result;
  }
  
  const ManagedSensor& ms = it->second;
  
  if (!ms.has_valid_reading) {
    return result;
  }
  
  uint32_t now_ms = millis();
  result.temperature = ms.last_value;
  result.sample_time_ms = ms.last_sample_time_ms;
  result.age_ms = now_ms - ms.last_sample_time_ms;
  result.valid = true;
  
  return result;
}

bool SensorManager::is_fresh(const std::string& sensor_id, uint32_t timeout_ms) {
  SensorReading reading = get_reading(sensor_id);
  return reading.valid && reading.age_ms <= timeout_ms;
}

}  // namespace heatmesh
}  // namespace esphome
