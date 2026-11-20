#pragma once

#include "esphome/core/helpers.h"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace esphome {
namespace ble_device_base {

struct UUID {
  uint8_t lo;
  uint8_t hi;
  bool contains(uint8_t a, uint8_t b) const { return lo == a && hi == b; }
};

struct ServiceData {
  UUID uuid;
  std::vector<uint8_t> data;
};

class ESPBTDevice {
 public:
  uint64_t address{0};
  std::vector<ServiceData> service_datas;
  const std::vector<ServiceData> &get_service_datas() const { return service_datas; }
  int get_rssi() const { return -60; }
  uint64_t address_uint64() const { return address; }
  const char *address_str_to(char *buffer) const {
    std::snprintf(buffer, MAC_ADDRESS_PRETTY_BUFFER_SIZE, "%012llX", static_cast<unsigned long long>(address));
    return buffer;
  }
};

class ESPBTDeviceListener {
 public:
  virtual ~ESPBTDeviceListener() = default;
  virtual bool parse_device(const ESPBTDevice &device) = 0;
};

}  // namespace ble_device_base
}  // namespace esphome
