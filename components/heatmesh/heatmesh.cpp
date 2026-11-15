#include "heatmesh.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace heatmesh {

static const char *const TAG = "heatmesh";
HeatmeshComponent *HeatmeshComponent::instance_ = nullptr;

void HeatmeshComponent::setup() {
  if (setup_complete_)
    return;
  instance_ = this;
  ESP_LOGI(TAG, "HeatMesh fix v0.1.1: setup starting; %zu zones configured", zone_configs_.size());
  for (const auto &cfg : zone_configs_) {
    auto zone = std::make_unique<ZoneController>(
        cfg.zone_id, cfg.temperature_sensor, cfg.valve_output, cfg.target_temperature,
        cfg.sensor_timeout_ms, cfg.kp, cfg.ki, cfg.control_period_ms, cfg.pwm_period_min,
        cfg.min_on_time_ms);
    zone->setup();
    zones_.push_back(std::move(zone));
    ESP_LOGI(TAG, "  Zone '%s': target=%.1f C, timeout=%" PRIu32 " ms, kp=%.3f, ki=%.4f",
             cfg.zone_id.c_str(), cfg.target_temperature, cfg.sensor_timeout_ms, cfg.kp, cfg.ki);
  }
  setup_complete_ = true;
  ESP_LOGI(TAG, "HeatMesh setup complete; waiting for new temperature publications");
}

void HeatmeshComponent::loop() {
  for (auto &zone : zones_)
    zone->loop();
}

void HeatmeshComponent::on_shutdown() {
  for (auto &zone : zones_)
    zone->shutdown();
}

void HeatmeshComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "HeatMesh fix v0.1.1: %zu zones; failsafe=output OFF", zones_.size());
  for (const auto &zone : zones_) {
    ESP_LOGCONFIG(TAG, "  '%s': T=%.2f C, setpoint=%.1f C, demand=%.1f%%, output=%s",
                  zone->get_zone_id().c_str(), zone->get_current_temperature(),
                  zone->get_target_temperature(), zone->get_heat_demand_percent(),
                  zone->get_valve_state() ? "ON" : "OFF");
  }
}

void HeatmeshComponent::add_zone(const std::string &zone_id, sensor::Sensor *temperature_sensor,
                                switch_::Switch *valve_output, float target_temperature,
                                uint32_t sensor_timeout_ms, float kp, float ki,
                                uint32_t control_period_ms, uint32_t pwm_period_min,
                                uint32_t min_on_time_ms) {
  zone_configs_.push_back({zone_id, temperature_sensor, valve_output, target_temperature,
                          sensor_timeout_ms, kp, ki, control_period_ms, pwm_period_min,
                          min_on_time_ms});
}

ZoneController *HeatmeshComponent::get_zone(const std::string &zone_id) {
  for (auto &zone : zones_) {
    if (zone->get_zone_id() == zone_id)
      return zone.get();
  }
  return nullptr;
}

}  // namespace heatmesh
}  // namespace esphome
