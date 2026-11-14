#include "heatmesh.h"
#include "esphome/core/log.h"

static const char* const TAG = "heatmesh";

namespace esphome {
namespace heatmesh {

HeatmeshComponent* HeatmeshComponent::instance_ = nullptr;

void HeatmeshComponent::setup() {
  ESP_LOGI(TAG, "HeatmeshComponent::setup() CALLED - Component is ALIVE!");
  instance_ = this;
  ESP_LOGI(TAG, "instance_ pointer set to %p", (void*)instance_);
}

void HeatmeshComponent::loop() {
  // TODO
}

void HeatmeshComponent::dump_config() {
  ESP_LOGI(TAG, "HeatmeshComponent::dump_config() - Component initialized successfully");
}

}  // namespace heatmesh
}  // namespace esphome

