#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "lwip/ip_addr.h"

#include "config.h"
#include "logging.h"
#include "ap_server.h"
#include "wifi.h"


static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(
            WIFI_AP_TAG,
            "MAC: "MACSTR" join, AID = %d",
            MAC2STR(event->event_info.sta_connected.mac),
            event->event_info.sta_connected.aid
        );
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(
            WIFI_AP_TAG,
            "STA connected to \"%s\"",
            event->event_info.connected.ssid
        );
        break;
    default:
        break;
    }
    return ESP_OK;
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    tcpip_adapter_init();

    tcpip_adapter_ip_info_t ip_info = {
        .ip = { .addr = AP_IP_ADDR },
        .gw = { .addr = AP_IP_ADDR },
        .netmask = { .addr = AP_NETMASK }
    };

    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    wifi_init_config_t default_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&default_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t wifi_ap_config = get_wifi_ap_config();
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(
        WIFI_AP_TAG,
        "AP Created. SSID: [%s], password: [%s]",
        wifi_ap_config.ap.ssid,
        wifi_ap_config.ap.password
    );

    xTaskCreate(&ap_server_task, "ap_server", 8192, NULL, 6, NULL);
}
