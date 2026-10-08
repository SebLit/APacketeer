A c++ port of [JPacketeer](https://github.com/SebLit/JPacketeer) for Arduino. Please refer to JPacketeer for full documentation. This documentation only lists 
differences to the Java version.

# API differences 
* Errors are not handled as Exceptions. They are represented by int values provided by the various API functions and will be carried through `Client` to the caller of `send()`and `receive()`. See `APacketeerErrors`
  * APacketeer reserves error codes in Range 0xF-0xFFFF and 0 for success. The remaining values may be used by the implementations for custom error codes
  * `SUCCESS(0)` means no error occurred
  * `TIMEOUT(0xF000)` means alls attempts to send a packet were used up and failed
  * `PACKET_FAILURE(0xF001)` means ack for packet wasn't received
  * `PAYLOAD_TOO_LARGE(0xF002)` means received payload is larger than available receive buffer (see points below)
  * `BUFFER_ALLOCATION(0xF003)` means packet with payload was received but receive buffer couldn't be initialized/allocated
  * `INVALID_PAYLOAD(0xF004)` means packet without payload but with payload length > 0 was attempted to be sent
* `Client` uses a fixed size buffer to receive the payload bytes in order to prevent dynamic memory allocation
  * If allocation should fail, `Client`'s `receive()` will return `APacketeerError::BUFFER_ALLOCATION`
  * If a received payload shouldn't be within bounds of the target size the `Client` will return `APacketeerError::PAYLOAD_TOO_LARGE`