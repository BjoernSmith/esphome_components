#pragma once

namespace esphome {
namespace switch_ {
class Switch {
 public:
  // Records an ON output command.
  void turn_on() { state = true; }
  // Records an OFF output command.
  void turn_off() { state = false; }
  bool state{false};
};
}  // namespace switch_
}  // namespace esphome
