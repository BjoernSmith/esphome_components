#include "heatmesh.h"
#include "esphome/core/log.h"

static const char* const TAG = "heatmesh";

namespace esphome {
namespace heatmesh {

// Static member initialization
HeatmeshComponent* HeatmeshComponent::instance_ = nullptr;

HeatmeshComponent* HeatmeshComponent::get_instance() {
  if (instance_ == nullptr) {
    ESP_LOGE(TAG, "WARNING: get_instance() called but instance_ is NULL!");
  }
  return instance_;
}

void HeatmeshComponent::setup() {
  ESP_LOGI(TAG, "HeatmeshComponent::setup() CALLED");
  instance_ = this;  // Set global singleton instance
  ESP_LOGI(TAG, "instance_ set to %p", (void*)instance_);
  ESP_LOGI(TAG, "HeatmeshComponent setup starting; %zu zones configured", zone_configs_.size());
  
  // Create zone controllers from configs
  for (const auto& cfg : zone_configs_) {
    auto zone = std::make_unique<ZoneController>(
        cfg.zone_id,
        cfg.temperature_sensor,
        cfg.valve_output,
        cfg.target_temperature,
        cfg.sensor_timeout_ms,
        cfg.kp,
        cfg.ki,
        cfg.control_period_ms,
        cfg.pwm_period_min
    );
    zones_.push_back(std::move(zone));
    
    ESP_LOGI(TAG, "  Zone '%s': target=%.1f°C, timeout=%lu ms, kp=%.2f, ki=%.2f",
             cfg.zone_id.c_str(), cfg.target_temperature, 
             cfg.sensor_timeout_ms, cfg.kp, cfg.ki);
  }
  
  ESP_LOGI(TAG, "HeatmeshComponent setup complete, instance_=%p", (void*)instance_);
}

void HeatmeshComponent::loop() {
  sensor_manager_.loop();
  
  for (auto& zone : zones_) {
    zone->loop();
  }
}

void HeatmeshComponent::dump_config() {
  ESP_LOGI(TAG, "HeatmeshComponent Configuration:");
  ESP_LOGI(TAG, "  Total zones: %zu", zones_.size());
  for (size_t i = 0; i < zones_.size(); ++i) {
    ESP_LOGI(TAG, "    [%zu] %s: T=%.1f°C, demand=%.1f%%, valve=%s",
             i, zones_[i]->get_zone_id().c_str(),
             zones_[i]->get_current_temperature(),
             zones_[i]->get_heat_demand_percent(),
             zones_[i]->get_valve_state() ? "ON" : "OFF");
  }
}

void HeatmeshComponent::add_zone(
    const std::string& zone_id,
    sensor::Sensor* temperature_sensor,
    switch_::Switch* valve_output,
    float target_temperature,
    uint32_t sensor_timeout_ms,
    float kp,
    float ki,
    uint32_t control_period_ms,
    uint32_t pwm_period_min
) {
  ZoneConfig cfg;
  cfg.zone_id = zone_id;
  cfg.temperature_sensor = temperature_sensor;
  cfg.valve_output = valve_output;
  cfg.target_temperature = target_temperature;
  cfg.sensor_timeout_ms = sensor_timeout_ms;
  cfg.kp = kp;
  cfg.ki = ki;
  cfg.control_period_ms = control_period_ms;
  cfg.pwm_period_min = pwm_period_min;
  
  zone_configs_.push_back(cfg);
  sensor_manager_.register_sensor(zone_id, temperature_sensor);
}

ZoneController* HeatmeshComponent::get_zone(const std::string& zone_id) {
  for (auto& zone : zones_) {
    if (zone->get_zone_id() == zone_id) {
      return zone.get();
    }
  }
  return nullptr;
}

}  // namespace heatmesh
}  // namespace esphome
