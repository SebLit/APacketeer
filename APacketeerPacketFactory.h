#ifndef APACKETEER_PACKET_FACTORY_H
#define APACKETEER_PACKET_FACTORY_H

#include <stdint.h>
#include "APacketeerIncomingPacket.h"

class PacketFactory {
public:
  virtual ~PacketFactory() {
  }

  virtual IncomingPacket* create(
    uint8_t protocolVersion,
    uint8_t type,
    uint8_t version,
    uint8_t flags,
    int& error) = 0;
};

#endif
