#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace ble_device_base {
// Encrypted packets are not exercised by the host test.
inline bool aes_ccm_auth_decrypt(const uint8_t *, const uint8_t *, size_t, const uint8_t *, size_t, const uint8_t *,
                                 size_t, uint8_t *, const uint8_t *, size_t) {
  return false;
}
}  // namespace ble_device_base
}  // namespace esphome
