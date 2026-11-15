#include "heatmesh.h"
#include "esphome/core/log.h"

namespace esphome {
namespace heatmesh {

static const char *const TAG = "heatmesh";
static const char *const HEATMESH_VERSION = "0.2.0";
HeatmeshComponent *HeatmeshComponent::instance_ = nullptr;

// Initializes each zone once after registering this component as the active instance.
void HeatmeshComponent::setup() {
  if (setup_complete_)
    return;
  instance_ = this;
  ESP_LOGI(TAG, "HeatMesh v%s: setup starting; %zu zones configured", HEATMESH_VERSION, zones_.size());
  for (auto *zone : zones_)
    zone->setup();
  setup_complete_ = true;
  ESP_LOGI(TAG, "HeatMesh setup complete; waiting for new temperature publications");
}

// Runs one control-loop iteration for every configured zone.
void HeatmeshComponent::loop() {
  for (auto *zone : zones_)
    zone->loop();
}

// Invokes each zone's safe shutdown path.
void HeatmeshComponent::on_shutdown() {
  for (auto *zone : zones_)
    zone->shutdown();
}

// Writes the configured zone count and current state to the ESPHome log.
void HeatmeshComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "HeatMesh v%s: %zu zones; failsafe=output OFF", HEATMESH_VERSION, zones_.size());
  for (const auto *zone : zones_) {
    ESP_LOGCONFIG(TAG, "  '%s': mode=%s, T=%.2f C, setpoint=%.1f C, demand=%.1f%%, output=%s",
                  zone->get_zone_id().c_str(), zone->is_enabled() ? "HEAT" : "OFF",
                  zone->get_current_temperature(), zone->get_target_temperature(),
                  zone->get_heat_demand_percent(), zone->get_valve_state() ? "ON" : "OFF");
  }
}

// Returns the zone matching the requested ID, or nullptr if it is not configured.
ZoneController *HeatmeshComponent::get_zone(const std::string &zone_id) {
  for (auto *zone : zones_) {
    if (zone->get_zone_id() == zone_id)
      return zone;
  }
  return nullptr;
}

}  // namespace heatmesh
}  // namespace esphome
