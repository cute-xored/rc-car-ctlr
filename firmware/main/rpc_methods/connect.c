#include <string.h>

#include "cJSON.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "../logging.h"
#include "../cjson_helper.h"
#include "../rpc_error.h"


const char* connect_name = "connect";

cJSON* connect(const cJSON* params, const char* id) {
    const char* ssid = get_string_prop(params, "ssid");

    if (ssid == NULL) {
        return invalid_request("Missing ssid parameter", id);
    }

    const char* pass = "";
    const char* param_pass = get_string_prop(params, "pass");

    if (param_pass != NULL) {
        pass = param_pass;
    }

    wifi_sta_config_t sta_config;

    memcpy(sta_config.ssid, ssid, 32 * sizeof(char));
    memcpy(sta_config.password, pass, 64 * sizeof(char));

    wifi_config_t wifi_config = {
        .sta = sta_config
    };

    // Find a way to translate err to something more useful and then get rid of "meh"
    esp_err_t err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(RPC_TAG, "Failed to set STA config");
        return wifi_error("meh", id);
    }

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(RPC_TAG, "Failed to connect to AP:, %s:%s", ssid, pass);
        return wifi_error("meh", id);
    }

    return construct_empty_resp(id);
}
