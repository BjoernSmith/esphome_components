#pragma once

#include "esphome/core/component.h"
#include "zone_controller.h"
#include <string>
#include <vector>

namespace esphome {
namespace heatmesh {

class HeatmeshComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  // After I/O hardware, before network/BLE starts delivering measurements.
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Zones are created by codegen and live for the lifetime of the firmware.
  void add_zone(ZoneController *zone) { zones_.push_back(zone); }

  ZoneController *get_zone(const std::string &zone_id);
  size_t get_zone_count() const { return zones_.size(); }

  // Backward compatibility only. New YAML should use id(heatmesh_controller).
  static HeatmeshComponent *get_instance() { return instance_; }

 private:
  static HeatmeshComponent *instance_;
  std::vector<ZoneController *> zones_;
  bool setup_complete_{false};
};

}  // namespace heatmesh
}  // namespace esphome
