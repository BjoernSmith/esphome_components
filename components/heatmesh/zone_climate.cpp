#include "zone_climate.h"
#include "zone_controller.h"
#include <cmath>

namespace esphome {
namespace heatmesh {

// Compares climate values while treating two NaN temperatures as unchanged.
static bool same_value(float a, float b) { return (std::isnan(a) && std::isnan(b)) || a == b; }

// Restores saved climate state or initializes HEAT mode from the zone's YAML setpoint.
void ZoneClimate::restore() {
  auto restored = this->restore_state_();
  if (restored.has_value()) {
    restored->to_call(this).perform();
    return;
  }
  // First boot: start in HEAT with the setpoint from YAML.
  this->mode = climate::CLIMATE_MODE_HEAT;
  this->target_temperature = zone_->get_target_temperature();
  zone_->set_enabled(true);
  this->current_temperature = zone_->get_current_temperature();
  this->action = this->zone_action_();
  this->publish_state();
}

// Forwards a climate request to the zone and publishes the accepted thermostat state.
void ZoneClimate::control(const climate::ClimateCall &call) {
  auto mode = call.get_mode();
  if (mode.has_value())
    this->mode = *mode;
  auto target = call.get_target_temperature();
  if (target.has_value())
    zone_->set_target_temperature(*target);
  // Report the value the zone accepted (clamped to 5..30 C).
  this->target_temperature = zone_->get_target_temperature();
  zone_->set_enabled(this->mode != climate::CLIMATE_MODE_OFF);
  this->current_temperature = zone_->get_current_temperature();
  this->action = this->zone_action_();
  this->publish_state();
}

// Publishes the zone's current temperature and action when either has changed.
void ZoneClimate::sync_from_zone() {
  const float current = zone_->get_current_temperature();
  const climate::ClimateAction action = this->zone_action_();
  if (same_value(current, this->current_temperature) && action == this->action)
    return;
  this->current_temperature = current;
  this->action = action;
  this->publish_state();
}

// Derives climate action from OFF mode and the zone's commanded valve state.
climate::ClimateAction ZoneClimate::zone_action_() const {
  if (this->mode == climate::CLIMATE_MODE_OFF)
    return climate::CLIMATE_ACTION_OFF;
  return zone_->get_valve_state() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
}

// Advertises the climate features, modes, and setpoint range supported by this entity.
climate::ClimateTraits ZoneClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  traits.set_visual_min_temperature(5.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_target_temperature_step(0.5f);
  traits.set_visual_current_temperature_step(0.1f);
  return traits;
}

}  // namespace heatmesh
}  // namespace esphome
