#include "xplane_discovery.h"
#include "config.h"
#include "plane_sprite.h"
#include <cstring>

void XPlaneDiscovery::begin() {
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

void XPlaneDiscovery::drawStatus(LGFX_Sprite& fb) {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    fb.fillSprite(COLOR_BG);
    fb.setTextDatum(middle_center);

    fb.setTextColor(COLOR_TITLE);
    fb.setTextSize(3.0);
    fb.drawString("Searching for X-Plane", cx, cy - 40);

    fb.setTextColor(COLOR_DIAL_RIM);
    fb.setTextSize(2.0);
    fb.drawString("Listening on port 49707", cx, cy + 20);

    // Animated plane sliding across middle third of screen
    static constexpr int PLANE_START = SCREEN_WIDTH / 3;
    static constexpr int PLANE_END = SCREEN_WIDTH * 2 / 3;
    static int planeX = PLANE_START;
    planeX += 8;
    if (planeX > PLANE_END) planeX = PLANE_START - PLANE_IMG_W;

    int planeY = cy + 55;
    // Draw using pushImage with transparent color
    fb.pushImage(planeX, planeY, PLANE_IMG_W, PLANE_IMG_H,
                 PLANE_IMG, PLANE_IMG_TRANS);
}
