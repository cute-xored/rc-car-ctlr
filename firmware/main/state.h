#ifndef __STATE_H
#define __STATE_H

#include <stdint.h>

typedef enum {
    CONNECTED,
    RECONNECTING,
    DISCONNECTED 
} connection_state_t;

typedef struct {
    struct {
        uint8_t ssid[32];
        uint8_t pass[64];

        uint32_t ip;
        uint32_t gw;

        connection_state_t state;
    } sta;
    struct {
        uint8_t ssid[32];
        uint8_t pass[64];

        uint32_t ip;
    } ap;
} state_t;

int init_state();
state_t get_state();
void update_sta_state(
    const uint8_t** ssid,
    const uint8_t** pass,
    const uint32_t* ip,
    const uint32_t* gw,
    const connection_state_t* state
);

#endif
