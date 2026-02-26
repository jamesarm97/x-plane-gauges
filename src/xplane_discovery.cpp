#include "xplane_discovery.h"
#include "config.h"
#include <M5Dial.h>
#include <cstring>

void XPlaneDiscovery::begin() {
    // Join multicast group 239.255.1.1 — X-Plane sends BECN to this group
    // beginMulticast also binds to the port, so broadcast packets are received too
    _udp.beginMulticast(XPLANE_MULTICAST_ADDR, XPLANE_BEACON_PORT);
    _begun = true;
    _found = false;
    Serial.println("[Discovery] Listening for X-Plane BECN on 239.255.1.1:49707");
}

bool XPlaneDiscovery::listen() {
    if (!_begun) return false;
    if (_found) return true;

    int packetSize = _udp.parsePacket();
    if (packetSize < 6) return false;

    Serial.printf("[Discovery] Received %d bytes from %s:%d\n",
                  packetSize, _udp.remoteIP().toString().c_str(), _udp.remotePort());

    int bytesRead = _udp.read(_rxBuf, sizeof(_rxBuf));
    if (bytesRead < 6) return false;

    // BECN packet: "BECN\0" (5 bytes) + beacon data
    if (memcmp(_rxBuf, "BECN", 4) != 0) {
        Serial.printf("[Discovery] Not a BECN packet (header: %c%c%c%c)\n",
                      _rxBuf[0], _rxBuf[1], _rxBuf[2], _rxBuf[3]);
        return false;
    }

    // Beacon data structure (after 5-byte header):
    //   uint8_t  beacon_major_version (offset 5)
    //   uint8_t  beacon_minor_version (offset 6)
    //   int32_t  application_host_id  (offset 7)  — 1 = X-Plane
    //   int32_t  version_number       (offset 11) — e.g., 120500 for 12.05
    //   uint32_t role                 (offset 15)
    //   uint16_t port                 (offset 19) — port X-Plane listens on
    //   char[]   computer_name        (offset 21) — null-terminated

    if (bytesRead < 21) return false;

    // Verify it's X-Plane (app host id == 1)
    int32_t appId;
    memcpy(&appId, _rxBuf + 7, 4);
    if (appId != 1) return false;

    // Extract version
    memcpy(&_version, _rxBuf + 11, 4);

    // Extract port
    memcpy(&_port, _rxBuf + 19, 2);

    // Extract computer name
    size_t nameLen = bytesRead - 21;
    if (nameLen > sizeof(_name) - 1) nameLen = sizeof(_name) - 1;
    memcpy(_name, _rxBuf + 21, nameLen);
    _name[nameLen] = '\0';

    // The sender's IP is the X-Plane host
    _ip = _udp.remoteIP();

    Serial.printf("[Discovery] Found X-Plane %d at %s:%d (%s)\n",
                  _version, _ip.toString().c_str(), _port, _name);

    // Stop listening — we found it
    _udp.stop();
    _found = true;
    return true;
}

void XPlaneDiscovery::drawStatus(M5GFX& display) {
    display.fillScreen(COLOR_BG);
    display.setTextDatum(middle_center);

    display.setTextColor(COLOR_TITLE);
    display.setTextSize(1.5);
    display.drawString("Searching for", CENTER_X, CENTER_Y - 35);
    display.drawString("X-Plane...", CENTER_X, CENTER_Y - 5);

    display.setTextColor(COLOR_DIAL_RIM);
    display.setTextSize(1.0);
    display.drawString("Listening on port 49707", CENTER_X, CENTER_Y + 30);

    // Animated dots
    static int dots = 0;
    dots = (dots + 1) % 4;
    char buf[8] = "   ";
    for (int i = 0; i < dots; i++) buf[i] = '.';
    display.setTextColor(COLOR_VALUE);
    display.setTextSize(2.0);
    display.drawString(buf, CENTER_X, CENTER_Y + 60);
}
