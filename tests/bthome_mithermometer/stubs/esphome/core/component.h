#pragma once

namespace esphome {
class Component {
 public:
  virtual ~Component() = default;
  virtual void dump_config() {}
};
}  // namespace esphome
