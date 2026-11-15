#pragma once

namespace esphome {
namespace climate {
class ClimateCall {};
class ClimateTraits {};
enum ClimateAction { CLIMATE_ACTION_OFF, CLIMATE_ACTION_HEATING, CLIMATE_ACTION_IDLE };
class Climate {
 protected:
  virtual void control(const ClimateCall &) = 0;
  virtual ClimateTraits traits() = 0;
};
}  // namespace climate
}  // namespace esphome
