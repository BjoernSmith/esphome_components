#pragma once

namespace esphome {
namespace setup_priority {
constexpr float DATA = 600.0f;
}

class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual void loop() {}
  virtual void dump_config() {}
  virtual void on_shutdown() {}
  virtual float get_setup_priority() const { return 0.0f; }
};
}  // namespace esphome
