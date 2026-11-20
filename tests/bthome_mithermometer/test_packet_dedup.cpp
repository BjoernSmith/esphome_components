#include "../../components/bthome_mithermometer/bthome_ble.h"

#include <cstdio>
#include <cstdlib>

using esphome::ble_device_base::ESPBTDevice;
using esphome::bthome_mithermometer::BTHomeMiThermometer;

static const uint64_t MAC = 0xA4C138097B7BULL;
static int failures = 0;

static void check(bool condition, const char *message) {
  if (!condition) {
    std::printf("FAIL: %s\n", message);
    failures++;
  }
}

// Unencrypted pvvx data1: packet ID, battery, temperature (0.01 C), humidity (0.01 %).
static ESPBTDevice data1(uint8_t packet_id, int16_t centi_c) {
  ESPBTDevice d;
  d.address = MAC;
  d.service_datas.push_back({{0xD2, 0xFC},
                             {0x40, 0x00, packet_id, 0x01, 88, 0x02, uint8_t(centi_c), uint8_t(centi_c >> 8), 0x03,
                              0xDF, 0x0F}});
  return d;
}

// Unencrypted pvvx data2: packet ID and battery voltage (mV), sent with the same ID as data1.
static ESPBTDevice data2(uint8_t packet_id) {
  ESPBTDevice d;
  d.address = MAC;
  d.service_datas.push_back({{0xD2, 0xFC}, {0x40, 0x00, packet_id, 0x0C, 0xB8, 0x0B}});
  return d;
}

int main() {
  BTHomeMiThermometer dev;
  esphome::sensor::Sensor temperature;
  dev.set_address(MAC);
  dev.set_temperature(&temperature);

  // Voltage packet first must not hide the temperature packet with the same ID.
  dev.parse_device(data2(5));
  check(dev.parse_device(data1(5, 2028)), "temperature packet after voltage packet with same ID accepted");
  check(temperature.values.size() == 1 && temperature.values[0] > 20.27f && temperature.values[0] < 20.29f,
        "20.28 C published");

  // Retransmissions of the same measurement are still ignored.
  dev.parse_device(data2(5));
  check(!dev.parse_device(data1(5, 2028)), "identical retransmission ignored");
  check(temperature.values.size() == 1, "retransmission not republished");

  // The next measurement is accepted, even if the temperature is unchanged.
  check(dev.parse_device(data1(6, 2028)), "next packet ID accepted");
  check(temperature.values.size() == 2, "unchanged temperature with new ID published");

  if (failures == 0)
    std::printf("All BTHome duplicate tests passed\n");
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
