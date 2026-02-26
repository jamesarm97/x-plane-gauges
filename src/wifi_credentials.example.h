#pragma once
#include "wifi_manager.h"

// Copy this file to wifi_credentials.h and fill in your WiFi networks.
// wifi_credentials.h is gitignored and will not be committed.

static const WiFiCredential WIFI_NETWORKS[] = {
    {"YourSSID", "YourPassword"},
    // {"AnotherNetwork", "AnotherPassword"},
};
static constexpr int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);
