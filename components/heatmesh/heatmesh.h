#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace heatmesh {

class HeatmeshComponent : public Component {
 public:
  HeatmeshComponent() = default;
  
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  static HeatmeshComponent* get_instance() { return instance_; }

 private:
  static HeatmeshComponent* instance_;
};

}  // namespace heatmesh
}  // namespace esphome

