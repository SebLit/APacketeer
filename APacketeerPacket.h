#ifndef APACKETEER_PACKET_H
#define APACKETEER_PACKET_H

#include <stdint.h>
#include "APacketeerBitUtil.h"

class Packet {
public:
  static const uint8_t TYPE_ACK = 0;

  Packet(uint8_t type, uint8_t version, uint8_t flags)
    : type(type), version(version), flags(flags) {
  }

  virtual ~Packet() {
  }

  uint8_t getType() const {
    return type;
  }

  uint8_t getVersion() const {
    return version;
  }

  uint8_t getFlags() const {
    return flags;
  }

  bool isFlagSet(uint8_t index) const {
    return BitUtil::isFlagSet(flags, index);
  }

private:
  uint8_t type;
  uint8_t version;
  uint8_t flags;
};

#endif
