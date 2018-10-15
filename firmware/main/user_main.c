#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
#include "wifi.h"

static const char* WIFI_AP_TAG = "WIFI_AP";
static const char* AP_SERVER_TAG = "AP_SERVER";

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
    default:
        break;
    }
    return ESP_OK;
}

static void ap_server_task(void* _) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.91.1", &(addr.sin_addr));
    addr.sin_port = htons(AP_SERVER_PORT);

    socklen_t addr_len;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        ESP_LOGE(
            AP_SERVER_TAG,
            "Failed to create socket"
        );

        return;
    }

    int ret = bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));

    if (ret) {
        ESP_LOGE(
            AP_SERVER_TAG,
            "Cannot bind socket: %d",
            ret
        );

        return;
    }

    ret = listen(socket_fd, 32);
    if (ret) {
        ESP_LOGE(
            AP_SERVER_TAG,
            "Cannot listen socket: %d",
            ret
        );

        return;
    }

    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN);

    ESP_LOGI(
        AP_SERVER_TAG,
        "Listen to %s:%d",
        str,
        addr.sin_port
    );

    // There is only live connection is possible, it's ok for now,
    // also that's why we don't need to synchronize this interaction
    while(true) {
        ESP_LOGI(
            AP_SERVER_TAG,
            "Waiting for the client..."
        );

        int client_sock_fd = accept(socket_fd, (struct sockaddr*)&addr, &addr_len);
        if (client_sock_fd < 0) {
            ESP_LOGE(
                AP_SERVER_TAG,
                "Failed to accept"
            );

            continue;
        }

        ESP_LOGI(
            AP_SERVER_TAG,
            "Connect accepted, closing connection..."
        );
        close(client_sock_fd);
    }

    return;
}

void app_main()
{
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
