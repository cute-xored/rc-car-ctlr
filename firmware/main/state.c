#include <string.h>

#include "FreeRTOS.h"
#include "freertos/semphr.h"

#include "config.h"
#include "state.h"


static state_t state = {
    .ap = {
        .ssid = AP_SSID,
        .pass = AP_PASS,

        .ip = AP_IP_ADDR
    },
    .sta = {
        .state = DISCONNECTED
    }
};
static SemaphoreHandle_t mutex;

int init_state() {
    mutex = xSemaphoreCreateMutex();

    if (mutex == NULL) {
        return -1;
    }

    return 0;
}

state_t get_state() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    state_t ret = state;
    xSemaphoreGive(mutex);

    return ret;
}

void update_sta_state(
    const uint8_t** ssid,
    const uint8_t** pass,
    const uint32_t* ip,
    const uint32_t* gw,
    const connection_state_t* sta_state
) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (ssid != NULL) {
        strncpy((char*) state.sta.ssid, (char*) *ssid, 32);
    }

    if (pass != NULL) {
        strncpy((char*) state.sta.pass, (char*) *pass, 64);
    }

    if (ip != NULL) {
        state.sta.ip = *ip;
    }

    if (gw != NULL) {
        state.sta.gw = *gw;
    }

    if (sta_state != NULL) {
        state.sta.state = *sta_state;
    }
    xSemaphoreGive(mutex);
}
