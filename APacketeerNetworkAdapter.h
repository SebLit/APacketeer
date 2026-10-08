#ifndef APACKETEER_NETWORK_ADAPTER_H
#define APACKETEER_NETWORK_ADAPTER_H

#include <stdint.h>

class NetworkAdapter {
public:
  virtual ~NetworkAdapter() {
  }

  virtual int read(uint8_t* buffer, uint16_t count) = 0;
  virtual int write(const uint8_t* data, uint16_t count) = 0;
  virtual int skip(uint16_t count) = 0;
};

#endif
