#pragma once

#include <M5Dial.h>
#include <WiFiUdp.h>
#include <IPAddress.h>
#include <cstdint>

// X-Plane broadcasts BECN packets on this port every ~1 second
#define XPLANE_BEACON_PORT 49707

class XPlaneDiscovery {
public:
    void begin();
    bool listen();           // Returns true when a beacon is found
    void drawStatus(M5GFX& display);

    IPAddress   foundIP()   const { return _ip; }
    uint16_t    foundPort() const { return _port; }
    const char* computerName() const { return _name; }
    int         xplaneVersion() const { return _version; }

private:
    WiFiUDP   _udp;
    bool      _begun = false;
    bool      _found = false;
    IPAddress _ip;
    uint16_t  _port = 49000;
    char      _name[64] = {0};
    int       _version = 0;
    uint8_t   _rxBuf[256];
};
