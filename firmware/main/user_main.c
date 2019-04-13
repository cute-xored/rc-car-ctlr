#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

#include "lwip/ip_addr.h"

#include "config.h"
#include "logging.h"
#include "state.h"
#include "ap_server.h"
#include "ctrl_server.h"
#include "wifi.h"


static TaskHandle_t ctrl_task_handle;

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    connection_state_t state;
    esp_err_t err;
    
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
        state = CONNECTED;
        update_sta_state(
            &event->event_info.connected.ssid,
            NULL,
            NULL,
            NULL,
            &state);
        ESP_LOGI(
            WIFI_STA_TAG,
            "Сonnected to \"%s\"",
            event->event_info.connected.ssid
        );
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        state = DISCONNECTED;
        update_sta_state(
            NULL,
            NULL,
            NULL,
            NULL,
            &state);
        ESP_LOGI(WIFI_STA_TAG, "Disconnected. Reconnecting...");

        err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(WIFI_STA_TAG, "Failed to connect to AP");
        }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        state = CONNECTED;
        update_sta_state(
            NULL,
            NULL,
            (uint32_t*) &event->event_info.got_ip.ip_info.ip,
            (uint32_t*) &event->event_info.got_ip.ip_info.gw,
            &state);

        update_ctrl_config(ctrl_task_handle, *((uint64_t*) &event->event_info.got_ip.ip_info.ip), CTRL_SERVER_PORT);
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
        state = DISCONNECTED;
        update_sta_state(
            NULL,
            NULL,
            NULL,
            NULL,
            &state);

        ESP_LOGI(WIFI_STA_TAG, "Lost IP");

        err = esp_wifi_disconnect();
        if (err != ESP_OK) {
            ESP_LOGE(WIFI_STA_TAG, "Failed to disconnect from AP");
        }
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

    if (init_state() != 0) {
        ESP_LOGE(COMMON_TAG, "Failed to initialize state");
        return;
    }

    if (init_ctrl() < 0) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to initialize controller task");
        return;
    }

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

    if (pdPASS != xTaskCreate(&ctrl_server_task, "ctrl_server", 8192, NULL, 6, &ctrl_task_handle)) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to create controller server task");
        return;
    }

    if (pdPASS != xTaskCreate(&ap_server_task, "ap_server", 8192, NULL, 6, NULL)) {
        ESP_LOGE(AP_SERVER_TAG, "Failed to create access point server task");
        return;
    }
}
