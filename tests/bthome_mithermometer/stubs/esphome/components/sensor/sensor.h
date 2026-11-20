#pragma once

#include <vector>

#define LOG_SENSOR(...) ((void) 0)

namespace esphome {
namespace sensor {
// Records every published value.
class Sensor {
 public:
  void publish_state(float value) { values.push_back(value); }
  std::vector<float> values;
};
}  // namespace sensor
}  // namespace esphome
