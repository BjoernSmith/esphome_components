#pragma once

#include "esphome/core/component.h"
#include "zone_controller.h"
#include <string>
#include <vector>

namespace esphome {
namespace heatmesh {

class HeatmeshComponent : public Component {
 public:
  // Initializes each configured zone and records this component instance.
  void setup() override;
  // Advances every zone's control loop.
  void loop() override;
  // Logs the configured zones and their current control state.
  void dump_config() override;
  // Safely shuts down each zone by forcing its output off.
  void on_shutdown() override;
  // After I/O hardware, before network/BLE starts delivering measurements.
  // Returns the setup priority required for safe sensor and output initialization.
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Zones are created by codegen and live for the lifetime of the firmware.
  void add_zone(ZoneController *zone) { zones_.push_back(zone); }

  // Finds a configured zone by its stable identifier, or returns nullptr.
  ZoneController *get_zone(const std::string &zone_id);
  // Returns the number of configured zones.
  size_t get_zone_count() const { return zones_.size(); }

  // Backward compatibility only. New YAML should use id(heatmesh_controller).
  static HeatmeshComponent *get_instance() { return instance_; }

 private:
  static HeatmeshComponent *instance_;
  std::vector<ZoneController *> zones_;
  ValveStartupCoordinator startup_coordinator_;
  bool setup_complete_{false};
};

}  // namespace heatmesh
}  // namespace esphome
