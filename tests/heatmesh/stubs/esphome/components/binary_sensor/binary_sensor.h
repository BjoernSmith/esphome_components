#pragma once

namespace esphome {
namespace binary_sensor {
class BinarySensor {
 public:
  // Records the latest binary sensor state.
  void publish_state(bool value) { state = value; }
  // Records the initial binary sensor state.
  void publish_initial_state(bool value) { state = value; }
  bool state{false};
};
}  // namespace binary_sensor
}  // namespace esphome
