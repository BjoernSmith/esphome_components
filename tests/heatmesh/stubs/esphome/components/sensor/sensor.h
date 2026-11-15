#pragma once

#include <functional>
#include <vector>

namespace esphome {
namespace sensor {
class Sensor {
 public:
  // Subscribes a callback to published sensor values.
  void add_on_state_callback(std::function<void(float)> callback) { callbacks_.push_back(callback); }
  // Delivers a value to the subscribed callbacks.
  void publish_state(float value) {
    for (const auto &callback : callbacks_)
      callback(value);
  }

 private:
  std::vector<std::function<void(float)>> callbacks_;
};
}  // namespace sensor
}  // namespace esphome
