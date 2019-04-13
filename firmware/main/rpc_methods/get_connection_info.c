#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../state.h"
#include "../cjson_helper.h"


const char* get_connection_info_name = "get_connection_info";

static cJSON* state_to_json(const state_t state) {
    cJSON* result = cJSON_CreateObject();

    cJSON_AddStringToObject(result, "ssid", (char*) state.sta.ssid);
    cJSON_AddNumberToObject(result, "state", state.sta.state);

    if (state.sta.ip != 0) {
        char ip_buf[80];
        inet_ntop(AF_INET, &state.sta.ip, ip_buf, 80);
        cJSON_AddStringToObject(result, "ip", ip_buf);
    } else {
        cJSON_AddNullToObject(result, "ip");
    }

    return result;
}

cJSON* get_connection_info(const cJSON* params, const char* id) {
    state_t state = get_state();
    cJSON* res = construct_base_obj(id);
    cJSON_AddItemToObject(res, "result", state_to_json(state));

    return res;
}
