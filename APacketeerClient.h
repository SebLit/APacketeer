#ifndef APACKETEER_CLIENT_H
#define APACKETEER_CLIENT_H

#include <stdint.h>
#include <new>
#include "APacketeerPacket.h"
#include "APacketeerPacketFactory.h"
#include "APacketeerNetworkAdapter.h"
#include "APacketeerErrors.h"

class Client {
public:
  Client(
    uint8_t protocolVersion,
    uint8_t maxSendAttempts,
    NetworkAdapter& adapter,
    PacketFactory& factory,
    uint16_t maxPayloadSize)
    : protocolVersion(protocolVersion),
      maxSendAttempts(maxSendAttempts),
      adapter(adapter),
      factory(factory),
      maxPayloadSize(maxPayloadSize),
      payloadBuffer(nullptr),
      messageCount(0),
      latestReceivedMessage(0),
      pendingStates(0),
      failureStates(0),
      processingStates(0) {
    if (maxPayloadSize > 0) {
      payloadBuffer = new (std::nothrow) uint8_t[maxPayloadSize];
    }
  }

  ~Client() {
    delete[] payloadBuffer;
  }

  uint8_t getProtocolVersion() const {
    return protocolVersion;
  }

  uint8_t getMaxSendAttempts() const {
    return maxSendAttempts;
  }

  uint16_t getMaxPayloadSize() const {
    return maxPayloadSize;
  }

  int send(
    const Packet& packet,
    const uint8_t* payload = nullptr,
    uint16_t payloadLength = 0) {

    if (payloadLength > 0 && payload == nullptr) {
      return APacketeerError::INVALID_PAYLOAD;
    }

    uint8_t messageId = messageCount;
    messageCount++;
    markAckPending(messageId, true, false);

    return send(messageId, packet, payload, payloadLength);
  }

  int receive() {
    uint8_t startByte;
    uint8_t header[9];
    uint8_t receivedHeaderChecksum[2];

    while (true) {
      int error = adapter.read(&startByte, 1);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      if (startByte != START_BYTE_LOW) {
        continue;
      }

      error = adapter.read(&startByte, 1);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      if (startByte != START_BYTE_HIGH) {
        continue;
      }

      error = adapter.read(header, sizeof(header));
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      error = adapter.read(receivedHeaderChecksum, sizeof(receivedHeaderChecksum));
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      uint16_t expectedHeaderChecksum = BitUtil::intFrom16Bit(
        receivedHeaderChecksum[0],
        receivedHeaderChecksum[1]);
      uint16_t actualHeaderChecksum = createChecksum(header, sizeof(header));

      if (expectedHeaderChecksum != actualHeaderChecksum) {
        continue;
      }

      break;
    }

    uint8_t messageId = header[1];
    uint8_t type = header[2];
    uint8_t version = header[3];
    uint8_t flags = header[4];
    uint16_t payloadLength = BitUtil::intFrom16Bit(header[5], header[6]);
    uint16_t expectedPayloadChecksum = BitUtil::intFrom16Bit(header[7], header[8]);

    if (type == Packet::TYPE_ACK) {
      markAckPending(
        messageId,
        false,
        BitUtil::isFlagSet(flags, 1));
      return APacketeerError::SUCCESS;
    }

    if (payloadLength > maxPayloadSize) {
      int error = adapter.skip(payloadLength);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      if (BitUtil::isFlagSet(flags, 0)) {
        int ackError = sendAck(messageId, true);
        if (ackError != APacketeerError::SUCCESS) {
          return ackError;
        }
      }

      return APacketeerError::PAYLOAD_TOO_LARGE;
    }

    if (payloadLength > 0) {
      if (payloadBuffer == nullptr) {
        int error = adapter.skip(payloadLength);
        if (error != APacketeerError::SUCCESS) {
          return error;
        }

        if (BitUtil::isFlagSet(flags, 0)) {
          int ackError = sendAck(messageId, true);
          if (ackError != APacketeerError::SUCCESS) {
            return ackError;
          }
        }

        return APacketeerError::BUFFER_ALLOCATION;
      }

      int error = adapter.read(payloadBuffer, payloadLength);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }
    }

    const uint8_t* payload = payloadLength > 0 ? payloadBuffer : nullptr;
    uint16_t actualPayloadChecksum = createChecksum(payload, payloadLength);

    if (expectedPayloadChecksum != actualPayloadChecksum) {
      return APacketeerError::SUCCESS;
    }

    bool shouldProcess = trackIncomingMessage(messageId);

    if (!shouldProcess) {
      if (BitUtil::isFlagSet(flags, 0)) {
        return sendAck(messageId, false);
      }
      return APacketeerError::SUCCESS;
    }

    IncomingPacket* packet = nullptr;
    int processingError = APacketeerError::SUCCESS;

    packet = factory.create(
      header[0],
      type,
      version,
      flags,
      processingError);

    bool success = false;

    if (processingError == APacketeerError::SUCCESS) {
        processingError = packet->process(payload, payloadLength);
        success = processingError == APacketeerError::SUCCESS;
    }

    delete packet;
    markAsProcessed(messageId);

    if (BitUtil::isFlagSet(flags, 0)) {
      int ackError = sendAck(messageId, !success);
      if (processingError == APacketeerError::SUCCESS && ackError != APacketeerError::SUCCESS) {
        return ackError;
      }
    }

    return processingError;
  }

private:
  static const uint8_t START_BYTE_LOW = 0x0F;
  static const uint8_t START_BYTE_HIGH = 0x0A;

  uint8_t protocolVersion;
  uint8_t maxSendAttempts;
  NetworkAdapter& adapter;
  PacketFactory& factory;
  uint16_t maxPayloadSize;
  uint8_t* payloadBuffer;

  uint8_t messageCount;
  uint8_t latestReceivedMessage;
  uint32_t pendingStates;
  uint32_t failureStates;
  uint32_t processingStates;

  int send(
    uint8_t messageId,
    const Packet& packet,
    const uint8_t* payload,
    uint16_t payloadLength) {
    uint16_t payloadChecksum = createChecksum(payload, payloadLength);

    uint8_t header[9] = {
      protocolVersion,
      messageId,
      packet.getType(),
      packet.getVersion(),
      packet.getFlags(),
      BitUtil::getByteAt(payloadLength, 0),
      BitUtil::getByteAt(payloadLength, 1),
      BitUtil::getByteAt(payloadChecksum, 0),
      BitUtil::getByteAt(payloadChecksum, 1)
    };

    uint16_t headerChecksum = createChecksum(header, sizeof(header));
    bool requiresAck = packet.isFlagSet(0);
    bool sendCompleted = !requiresAck;
    uint8_t sendAttempts = 0;

    do {
      int error = adapter.write(&START_BYTE_LOW, 1);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      error = adapter.write(&START_BYTE_HIGH, 1);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      error = adapter.write(header, sizeof(header));
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      uint8_t checksumBytes[2] = {
        BitUtil::getByteAt(headerChecksum, 0),
        BitUtil::getByteAt(headerChecksum, 1)
      };

      error = adapter.write(checksumBytes, sizeof(checksumBytes));
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      if (payloadLength > 0) {
        error = adapter.write(payload, payloadLength);
        if (error != APacketeerError::SUCCESS) {
          return error;
        }
      }

      sendAttempts++;

      if (requiresAck) {
        bool ackPending = false;
        error = isAckPending(messageId, ackPending);
        if (error != APacketeerError::SUCCESS) {
          return error;
        }

        while (ackPending) {
          error = receive();
          if (error != APacketeerError::SUCCESS) {
            return error;
          }

          error = isAckPending(messageId, ackPending);
          if (error != APacketeerError::SUCCESS) {
            return error;
          }
        }

        sendCompleted = true;
      }
    } while (!sendCompleted && sendAttempts < maxSendAttempts);

    if (!sendCompleted) {
      return APacketeerError::TIMEOUT;
    }

    if (requiresAck) {
      bool ackFailed = false;
      int error = isAckFailed(messageId, ackFailed);
      if (error != APacketeerError::SUCCESS) {
        return error;
      }

      if (ackFailed) {
        return APacketeerError::PACKET_FAILURE;
      }
    }

    return APacketeerError::SUCCESS;
  }

  int sendAck(uint8_t messageId, bool failed) {
    uint8_t flags = failed
                      ? static_cast<uint8_t>(BitUtil::createFlags(1))
                      : 0;
    Packet ack(Packet::TYPE_ACK, 1, flags);
    return send(messageId, ack, nullptr, 0);
  }

  void markAckPending(uint8_t messageId, bool pending, bool failed) {
    uint8_t diff = static_cast<uint8_t>(messageCount - messageId);

    if (diff == 0 || diff > 32) {
      return;
    }

    uint8_t stateIndex = diff - 1;
    pendingStates = BitUtil::setFlag(pendingStates, stateIndex, pending);
    failureStates = BitUtil::setFlag(failureStates, stateIndex, failed);
  }

  int isAckPending(uint8_t messageId, bool& pending) {
    return checkState(pendingStates, messageId, pending);
  }

  int isAckFailed(uint8_t messageId, bool& failed) {
    return checkState(failureStates, messageId, failed);
  }

  int checkState(uint32_t states, uint8_t messageId, bool& state) {
    uint8_t diff = static_cast<uint8_t>(messageCount - messageId);

    if (diff == 0 || diff > 32) {
      return APacketeerError::TIMEOUT;
    }

    state = BitUtil::isFlagSet(states, diff - 1);
    return APacketeerError::SUCCESS;
  }

  bool trackIncomingMessage(uint8_t messageId) {
    uint8_t forwardDistance = static_cast<uint8_t>(messageId - latestReceivedMessage);

    if (forwardDistance == 0) {
      return !BitUtil::isFlagSet(processingStates, 0);
    }

    if (forwardDistance < 128) {
      if (forwardDistance >= 32) {
        processingStates = 0;
      } else {
        processingStates <<= forwardDistance;
      }
      latestReceivedMessage = messageId;
      return true;
    }

    uint8_t backwardDistance = static_cast<uint8_t>(latestReceivedMessage - messageId);
    if (backwardDistance > 31) {
      return false;
    }

    return !BitUtil::isFlagSet(processingStates, backwardDistance);
  }

  void markAsProcessed(uint8_t messageId) {
    uint8_t forwardDistance = static_cast<uint8_t>(messageId - latestReceivedMessage);

    if (forwardDistance < 128) {
      if (forwardDistance >= 32) {
        processingStates = 0;
        latestReceivedMessage = messageId;
        forwardDistance = 0;
      } else if (forwardDistance > 0) {
        processingStates <<= forwardDistance;
        latestReceivedMessage = messageId;
        forwardDistance = 0;
      }
    }

    uint8_t backwardDistance = static_cast<uint8_t>(latestReceivedMessage - messageId);
    if (backwardDistance > 31) {
      return;
    }

    processingStates = BitUtil::setFlag(processingStates, backwardDistance, true);
  }

  static uint16_t createChecksum(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0x0000;
    const uint16_t polynomial = 0xA001;

    if (data == nullptr) {
      return 0;
    }

    for (uint16_t i = 0; i < length; i++) {
      crc ^= data[i];

      for (uint8_t bit = 0; bit < 8; bit++) {
        if ((crc & 0x0001) != 0) {
          crc = static_cast<uint16_t>((crc >> 1) ^ polynomial);
        } else {
          crc >>= 1;
        }
      }
    }

    return crc;
  }
};

#endif
