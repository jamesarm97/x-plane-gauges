#pragma once

#include <WiFiUdp.h>
#include <cstdint>

class XPlaneClient {
public:
    void begin(const char* ip, uint16_t port);
    void subscribe(const char* dataref, int frequency, int index);
    void unsubscribe(const char* dataref, int index);
    bool receive(int& outIndex, float& outValue);

private:
    WiFiUDP _udp;
    const char* _ip = nullptr;
    uint16_t _port = 49000;
    bool _begun = false;
    static constexpr size_t RREF_PACKET_SIZE = 413;
    uint8_t _rxBuf[512];

    // Buffered multi-pair parsing
    int _rxLen = 0;         // Bytes in current packet
    int _rxOffset = 0;      // Current read offset within packet

    void sendRREF(const char* dataref, int frequency, int index);
};
