#ifndef __CTRL_SERVER_H
#define __CTRL_SERVER_H

#include <stdint.h>

#include "freertos/task.h"


int init_ctrl();
void update_ctrl_config(TaskHandle_t handle, uint32_t ip, uint32_t port);
void ctrl_server_task(void* _);

#endif
