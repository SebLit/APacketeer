#ifndef APACKETEER_INCOMING_PACKET_H
#define APACKETEER_INCOMING_PACKET_H

#include <stdint.h>
#include "APacketeerPacket.h"

class IncomingPacket : public Packet {
public:
  IncomingPacket(uint8_t type, uint8_t version, uint8_t flags)
    : Packet(type, version, flags) {
  }

  virtual ~IncomingPacket() {
  }

  virtual int process(const uint8_t* payload, uint16_t payloadLength) = 0;
};

#endif
