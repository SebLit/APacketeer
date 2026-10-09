#ifndef APACKETEER_ERRORS_H
#define APACKETEER_ERRORS_H

namespace APacketeerError {
static const int SUCCESS = 0;
static const int TIMEOUT = 0xFF00;
static const int PACKET_FAILURE = 0xFF01;
static const int PAYLOAD_TOO_LARGE = 0xFF02;
static const int BUFFER_ALLOCATION = 0xFF03;
static const int INVALID_PAYLOAD = 0xFF04;
}

#endif
