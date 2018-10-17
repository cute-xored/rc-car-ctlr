#include <string.h>

#include "cJSON.h"

#include "esp_libc.h"

#include "cjson_helper.h"
#include "rpc_methods/methods.h"
#include "rpc_error.h"


char* dispatch(const char* json_str) {
    cJSON* req = cJSON_Parse(json_str);
    if (req == NULL) {
        const char* details = cJSON_GetErrorPtr(); // Maybe need to be freed

        cJSON* err = parse_error(details);
        char* ret = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);

        return ret;
    }

    const char* supported_ver = "2.0";
    const int supported_ver_len = strlen(supported_ver);

    const char* req_rpc_ver = get_string_prop(req, "rpcversion");
    const char* req_id = get_string_prop(req, "id");

    if (req_rpc_ver == NULL || strncmp(supported_ver, req_rpc_ver, supported_ver_len) != 0) {
        cJSON* err = invalid_request("Missing or unsuppoted rpcversion", req_id);
        char* ret = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);

        return ret;
    }

    const char* method_name = get_string_prop(req, "method");
    if (method_name == NULL) {
        cJSON* err = invalid_request("Missing method property", req_id);
        char* ret = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);

        return ret;
    }

    method_handler_t handler = get_method_handler(method_name);
    if (handler == NULL) {
        cJSON* err = method_not_found(req_id);
        char* ret = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);

        return ret;
    }
    

    cJSON* res = handler(get_prop(req, "params"), req_id);
    char* ret = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return ret;
}
