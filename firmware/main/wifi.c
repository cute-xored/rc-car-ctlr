#include <string.h>
#include "esp_wifi.h"

#include "config.h"

wifi_config_t get_wifi_ap_config() {
    wifi_config_t config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen(AP_PASS) == 0) {
        config.ap.authmode = WIFI_AUTH_OPEN;
    }

    return config;
}
