#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace esphome {
template<typename T> using optional = std::optional<T>;

static constexpr size_t MAC_ADDRESS_SIZE = 6;
static constexpr size_t MAC_ADDRESS_PRETTY_BUFFER_SIZE = 18;

constexpr size_t format_hex_pretty_size(size_t n) { return n * 3; }
inline const char *format_hex_pretty_to(char *buffer, const uint8_t *, size_t, char) { return buffer; }
inline uint16_t encode_uint16(uint8_t msb, uint8_t lsb) { return (uint16_t(msb) << 8) | lsb; }
}  // namespace esphome
