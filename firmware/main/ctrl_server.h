#ifndef __CTRL_SERVER_H
#define __CTRL_SERVER_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


extern TaskHandle_t ctrl_task_handle;

int init_ctrl();
void update_ctrl_config(TaskHandle_t handle, uint32_t ip, uint32_t port);
void ctrl_server_task(void* _);

#endif
