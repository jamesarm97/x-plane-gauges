#include "xplane_client.h"
#include <cstring>

void XPlaneClient::begin(const char* ip, uint16_t port) {
    _ip = ip;
    _port = port;
    _udp.begin(49001);  // Local listening port
    _begun = true;
}

void XPlaneClient::subscribe(const char* dataref, int frequency, int index) {
    sendRREF(dataref, frequency, index);
}

void XPlaneClient::unsubscribe(const char* dataref, int index) {
    sendRREF(dataref, 0, index);
}

void XPlaneClient::sendRREF(const char* dataref, int frequency, int index) {
    if (!_begun) return;

    // RREF packet: "RREF\0" (5 bytes) + int32 freq + int32 index + 400-byte dataref string
    uint8_t packet[RREF_PACKET_SIZE];
    memset(packet, 0, RREF_PACKET_SIZE);

    // Header
    memcpy(packet, "RREF\0", 5);

    // Frequency (little-endian int32 — native on ESP32)
    memcpy(packet + 5, &frequency, 4);

    // Index (little-endian int32)
    memcpy(packet + 9, &index, 4);

    // Dataref path (null-terminated, 400 bytes max)
    size_t len = strlen(dataref);
    if (len > 399) len = 399;
    memcpy(packet + 13, dataref, len);

    _udp.beginPacket(_ip, _port);
    _udp.write(packet, RREF_PACKET_SIZE);
    _udp.endPacket();
}

bool XPlaneClient::receive(int& outIndex, float& outValue) {
    // If we have buffered pairs from a previous packet, return the next one
    if (_rxOffset > 0 && _rxOffset + 8 <= _rxLen) {
        memcpy(&outIndex, _rxBuf + _rxOffset, 4);
        memcpy(&outValue, _rxBuf + _rxOffset + 4, 4);
        _rxOffset += 8;
        return true;
    }

    // No buffered data — try to read a new packet
    _rxOffset = 0;
    _rxLen = 0;

    int packetSize = _udp.parsePacket();
    if (packetSize < 13) return false;  // Minimum: 5-byte header + one 8-byte pair

    int bytesRead = _udp.read(_rxBuf, sizeof(_rxBuf));
    if (bytesRead < 13) return false;

    // Verify header is "RREF"
    if (memcmp(_rxBuf, "RREF", 4) != 0) return false;

    _rxLen = bytesRead;
    _rxOffset = 5;  // Skip 5-byte header

    // Return first pair
    memcpy(&outIndex, _rxBuf + _rxOffset, 4);
    memcpy(&outValue, _rxBuf + _rxOffset + 4, 4);
    _rxOffset += 8;
    return true;
}
