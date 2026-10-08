#ifndef APACKETEER_ERRORS_H
#define APACKETEER_ERRORS_H

namespace APacketeerError {
static const int SUCCESS = 0;
static const int TIMEOUT = 0xF000;
static const int PACKET_FAILURE = 0xF001;
static const int PAYLOAD_TOO_LARGE = 0xF002;
static const int BUFFER_ALLOCATION = 0xF003;
static const int INVALID_PAYLOAD = 0xF004;
}

#endif
