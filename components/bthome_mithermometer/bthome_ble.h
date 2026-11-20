#pragma once

// Override of ESPHome 2026.10.2 bthome_mithermometer (GPLv3, https://github.com/esphome/esphome).
// Change: duplicates are detected per complete payload instead of per packet ID. pvvx firmware
// alternates data packets (battery/temperature/humidity, then battery voltage) with the same packet
// ID; upstream dropped the temperature packet whenever the voltage packet was received first.

#include <array>
#include "esphome/components/ble_device_base/ble_device.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

// No platform #ifdef: ble_device_base provides the BLE types on every platform; this
// component is only compiled when configured (which requires a BLE hub). bindkey (AES-CCM)
// decryption availability is selected per platform in the .cpp.
namespace esphome::bthome_mithermometer {

class BTHomeMiThermometer final : public ble_device_base::ESPBTDeviceListener, public Component {
 public:
  void set_address(uint64_t address) { this->address_ = address; }
  void set_bindkey(std::initializer_list<uint8_t> bindkey);

  void set_temperature(sensor::Sensor *temperature) { this->temperature_ = temperature; }
  void set_humidity(sensor::Sensor *humidity) { this->humidity_ = humidity; }
  void set_battery_level(sensor::Sensor *battery_level) { this->battery_level_ = battery_level; }
  void set_battery_voltage(sensor::Sensor *battery_voltage) { this->battery_voltage_ = battery_voltage; }
  void set_signal_strength(sensor::Sensor *signal_strength) { this->signal_strength_ = signal_strength; }

  void dump_config() override;
  bool parse_device(const ble_device_base::ESPBTDevice &device) override;

 protected:
  bool handle_service_data_(const ble_device_base::ServiceData &service_data,
                            const ble_device_base::ESPBTDevice &device);
  bool decrypt_bthome_payload_(const std::vector<uint8_t> &data, uint64_t source_address,
                               std::vector<uint8_t> &payload) const;

  // Returns true if this exact packet-ID payload was recently processed; otherwise remembers it.
  bool is_duplicate_payload_(const uint8_t *payload, size_t payload_size);

  struct RecentPayload {
    uint8_t size{0};
    std::array<uint8_t, 31> data{};
  };
  static constexpr size_t RECENT_PAYLOADS = 4;

  uint64_t address_{0};
  std::array<RecentPayload, RECENT_PAYLOADS> recent_payloads_{};
  size_t next_recent_payload_{0};
  bool has_bindkey_{false};
  uint8_t bindkey_[16];

  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *humidity_{nullptr};
  sensor::Sensor *battery_level_{nullptr};
  sensor::Sensor *battery_voltage_{nullptr};
  sensor::Sensor *signal_strength_{nullptr};
};

}  // namespace esphome::bthome_mithermometer
