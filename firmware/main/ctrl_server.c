#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_libc.h"

#include "driver/gpio.h"
#include "driver/pwm.h"

#include "lwip/sockets.h"

#include "config.h"
#include "logging.h"


TaskHandle_t ctrl_task_handle;

#define A_MOTOR_POWER GPIO_NUM_5
#define B_MOTOR_POWER GPIO_NUM_4

#define A_MOTOR_DIRECTION GPIO_NUM_0
#define B_MOTOR_DIRECTION GPIO_NUM_2

const uint32_t pins[2] = {
    A_MOTOR_POWER,
    B_MOTOR_POWER
};

static const uint32_t PERIOD = 255;

uint32_t duties[2]  = { 0, 0 };
int16_t phases[2] = { 0, 0 };

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

    gpio_config_t gpio_conf = {
    	.intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (0x1 << A_MOTOR_DIRECTION) | (0x1 << B_MOTOR_DIRECTION),
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    if (ESP_OK != gpio_config(&gpio_conf)) {
    	ESP_LOGE(CTRL_SERVER_TAG, "Failed to set up GPIO pins");
	return 1;
    }

    if (ESP_OK != gpio_set_level(A_MOTOR_DIRECTION, 0)) {
    	ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_DIRECTION level");
	return 1;
    }

    if (ESP_OK != gpio_set_level(B_MOTOR_DIRECTION, 0)) {
    	ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_DIRECTION level");
	return 1;
    }

    if (ESP_OK != pwm_init(PERIOD, duties, 2, pins)) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to init PWM");
        return 1;
    }

    if (ESP_OK != pwm_set_phases(phases)) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to set PWM phases");
        return 1;
    }

    if (ESP_OK != pwm_start()) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to start PWM");
        return 1;
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

// A bit more about future plans for this:
// I'd like to have this logic in different event loop that handles
// all GPIO, at least just to make sure that we are working with them
// in different independent task and not gonna perform any usage collisions.
// But for prototype v0.0.1 this should be enough :)
static void set_gpio(uint8_t* data) {
    if (data[0] > 0) {
	const uint32_t duty = data[0];

        if (ESP_OK != pwm_set_duty(0, duty)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_POWER level");
            return;
        }
        if (ESP_OK != pwm_set_duty(1,  duty)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_POWER level");
            return;
        }

	if (ESP_OK != gpio_set_level(A_MOTOR_DIRECTION, 0)) {
	    ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_DIRECTION level");
	    return;
	}
	if (ESP_OK != gpio_set_level(B_MOTOR_DIRECTION, 0)) {
	    ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_DIRECTION level");
	    return;
	}
    } else if (data[1] > 0) {
	const uint32_t duty = data[1];

        if (ESP_OK != pwm_set_duty(0, duty)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_POWER level");
            return;
        }
        if (ESP_OK != pwm_set_duty(1,  duty)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_POWER level");
            return;
        }

	if (ESP_OK != gpio_set_level(A_MOTOR_DIRECTION, 1)) {
	    ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_DIRECTION level");
	    return;
	}
	if (ESP_OK != gpio_set_level(B_MOTOR_DIRECTION, 1)) {
	    ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_DIRECTION level");
	    return;
	}
    } else {
        if (ESP_OK != pwm_set_duty(0, 0)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set A_MOTOR_POWER level");
            return;
        }
        if (ESP_OK != pwm_set_duty(1,  0)) {
            ESP_LOGE(CTRL_SERVER_TAG, "Failed to set B_MOTOR_POWER level");
            return;
        }
    }

    if (ESP_OK != pwm_start()) {
        ESP_LOGE(CTRL_SERVER_TAG, "Failed to set restart PWM");
        return;
    }
}

void ctrl_server_task(void* _) {
    uint8_t error_occured = 1; // It's 1 because for first time we need to wait for address

    struct sockaddr_in addr;
    int server_sock = -1;
    uint8_t datagram[CTRL_DATAGRAM_SIZE];

    fd_set rfds;

    struct timeval select_timeout = {
        .tv_sec = 2,
        .tv_usec = 0
    };

    while(1) {
        if (pdTRUE == xTaskNotifyWait(0, 0, NULL, error_occured ? portMAX_DELAY : 0)) {
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
        set_gpio(datagram);
    }
}
