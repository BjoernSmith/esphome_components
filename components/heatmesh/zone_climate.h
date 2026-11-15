#pragma once

#include "esphome/components/climate/climate.h"

namespace esphome {
namespace heatmesh {

class ZoneController;

// Thermostat front-end for one zone. Regulation stays in ZoneController;
// this class only forwards mode/setpoint and reports state.
class ZoneClimate : public climate::Climate {
 public:
  // Binds the thermostat entity to the zone whose state it exposes.
  explicit ZoneClimate(ZoneController *zone) : zone_(zone) {}

  // Restores mode and setpoint from flash. Called from ZoneController::setup().
  void restore();
  // Publishes current temperature and action when they have changed.
  void sync_from_zone();

 protected:
  // Applies climate mode and target-temperature requests to the zone.
  void control(const climate::ClimateCall &call) override;
  // Declares the supported climate modes, features, and temperature range.
  climate::ClimateTraits traits() override;
  // Maps the zone's mode and commanded output to the climate action.
  climate::ClimateAction zone_action_() const;

  ZoneController *zone_;
};

}  // namespace heatmesh
}  // namespace esphome
