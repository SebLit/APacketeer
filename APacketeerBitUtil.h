#ifndef APACKETEER_BIT_UTIL_H
#define APACKETEER_BIT_UTIL_H

#include <stdint.h>

class BitUtil {
public:
  static bool isFlagSet(uint32_t flags, uint8_t index) {
    if (index >= 32) {
      return false;
    }
    return (flags & (uint32_t(1) << index)) != 0;
  }

  template<typename... Indices>
  static uint32_t createFlags(Indices... indices) {
    uint32_t result = 0;
    setFlags(result, indices...);
    return result;
  }

  static uint32_t setFlag(uint32_t flags, uint8_t index, bool isSet) {
    if (index >= 32) {
      return flags;
    }

    uint32_t bit = uint32_t(1) << index;
    return isSet ? (flags | bit) : (flags & ~bit);
  }

  static uint8_t getByteAt(uint32_t value, uint8_t index) {
    if (index >= 4) {
      return 0;
    }
    return static_cast<uint8_t>(value >> (8 * index));
  }

  static uint16_t intFrom16Bit(uint8_t low, uint8_t high) {
    return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
  }

private:
  static void setFlags(uint32_t&) {
  }

  template<typename First, typename... Rest>
  static void setFlags(uint32_t& result, First first, Rest... rest) {
    uint8_t index = static_cast<uint8_t>(first);
    if (index < 32) {
      result |= (uint32_t(1) << index);
    }
    setFlags(result, rest...);
  }
};

#endif
