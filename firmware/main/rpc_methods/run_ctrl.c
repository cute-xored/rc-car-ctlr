#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "cJSON.h"

#include "../config.h"
#include "../cjson_helper.h"
#include "../rpc_error.h"
#include "../ctrl_server.h"


const char* run_ctrl_name = "run_ctrl";

cJSON* run_ctrl(const cJSON* params, const char* id) {
    const char* ip = get_string_prop(params, "ip");

    if (ip == NULL) {
        return invalid_request("Missing ip parameter", ip);
    }

    uint32_t port = CTRL_SERVER_PORT;
    const uint32_t param_port = get_int_prop(params, "port");

    if (param_port != 0) {
        port = param_port;
    }

    uint32_t target_ip;
    inet_pton(AF_INET, ip, &target_ip);

    update_ctrl_config(ctrl_task_handle, target_ip, port);

    return construct_empty_resp(id);
}
