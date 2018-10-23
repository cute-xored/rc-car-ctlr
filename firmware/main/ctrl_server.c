#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_libc.h"

#include "lwip/sockets.h"

#include "config.h"
#include "logging.h"


struct {
    uint32_t ip;
    uint32_t port;
} config;
static SemaphoreHandle_t mutex;


int init_ctrl() {
    mutex = xSemaphoreCreateMutex();

    if (mutex == NULL) {
        return -1;
    }

    return 0;
}

void update_ctrl_config(TaskHandle_t handle, uint32_t ip, uint32_t port) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    config.ip = ip;
    config.port = port;

    struct in_addr ip_addr = { .s_addr = ip };
    ESP_LOGI(CTRL_SERVER_TAG, "Config was updated: %s:%d\n", inet_ntoa(ip_addr), port);

    xTaskNotify(handle, 0, eNoAction); // There is only one action for now thus no action
    xSemaphoreGive(mutex);
}

void ctrl_server_task(void* _) {
    uint8_t error_occured = 1; // It's 1 because for first time we need to wait for address

    struct sockaddr_in addr;
    int server_sock = -1;
    uint8_t datagram[CTRL_DATAGRAM_SIZE];

    fd_set rfds;

    struct timeval select_timeout = {
        .tv_sec = 5,
        .tv_usec = 0
    };

    while(1) {
        if (pdTRUE == xTaskNotifyWait(0, 0, NULL, error_occured ? portMAX_DELAY : 0)) {
            ESP_LOGI(CTRL_SERVER_TAG, "NOTIFIED");

            error_occured = 0;
            if (server_sock) {
                close(server_sock);
                server_sock = -1;
            }

            server_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (server_sock < 0) {
                ESP_LOGE(CTRL_SERVER_TAG, "Failed to create socket: %s", strerror(errno));
                return;
            }

            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;

            xSemaphoreTake(mutex, portMAX_DELAY);
            addr.sin_addr.s_addr = config.ip;
            addr.sin_port = htons(config.port);
            xSemaphoreGive(mutex);

            if (bind(server_sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
                ESP_LOGE(CTRL_SERVER_TAG, "Cannot bind socket: %s", strerror(errno));

                error_occured = 1;
                continue;
            }

            char mybuf[80];
            inet_ntop(AF_INET, &addr.sin_addr, mybuf, 80);
            ESP_LOGI(CTRL_SERVER_TAG, "Binded to %s:%d", mybuf, ntohs(addr.sin_port));
        }

        if (server_sock < 0) {
            continue;
        }
        
        FD_ZERO(&rfds);
        FD_SET(server_sock, &rfds);
        int ret = select(server_sock + 1, &rfds, NULL, NULL, &select_timeout);
        if (ret < 0) {
            ESP_LOGE(CTRL_SERVER_TAG, "Select error: %s", strerror(errno));
            error_occured = 1;
            continue;
        } else if (ret == 0) {
            ESP_LOGE(CTRL_SERVER_TAG, "Well there is nothing");
            continue;
        }

        int read_len = recvfrom(
            server_sock,
            datagram,
            CTRL_DATAGRAM_SIZE,  
            MSG_WAITALL,
            NULL,
            NULL);

        if (read_len < 0) {
            ESP_LOGE(CTRL_SERVER_TAG, "recvfrom error: %s", strerror(errno));
            error_occured = 1;
            continue;
        }

        ESP_LOGI(CTRL_SERVER_TAG, "Got: (%d %d %d %d)", datagram[0], datagram[1], datagram[2], datagram[3]);
    }
}
