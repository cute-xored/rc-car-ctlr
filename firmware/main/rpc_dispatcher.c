#include <string.h>

#include "cJSON.h"

#include "esp_libc.h"

#include "rpc_methods/methods.h"
#include "rpc_error.h"


static char* get_rpc_version(cJSON* req) {
    const cJSON* rpc_version = cJSON_GetObjectItemCaseSensitive(req, "rpcversion");

    if (cJSON_IsString(rpc_version)) {
        return rpc_version->valuestring;
    }

    return NULL;
}

static int get_id(cJSON* req) {
    const cJSON* id = cJSON_GetObjectItemCaseSensitive(req, "id");

    if (cJSON_IsNumber(id)) {
        return id->valueint;
    }

    return -1;
}

static char* get_method(cJSON* req) {
    const cJSON* method = cJSON_GetObjectItemCaseSensitive(req, "method");

    if (cJSON_IsString(method)) {
        return method->valuestring;
    }

    return NULL;
}

static cJSON* get_params(cJSON* req) {
    return cJSON_GetObjectItemCaseSensitive(req, "params");
}

char* dispatch(const char* json_str) {
    cJSON* req = cJSON_Parse(json_str);
    if (req == NULL) {
        const char* details = cJSON_GetErrorPtr(); // Maybe need to be freed

        cJSON* err = parse_error(details);
        char* ret = cJSON_Print(err);
        cJSON_Delete(err);

        return ret;
    }

    const char* supported_ver = "2.0";
    const int supported_ver_len = strlen(supported_ver);

    const char* req_rpc_ver = get_rpc_version(req);
    const int req_id = get_id(req);

    if (req_rpc_ver == NULL || strncmp(supported_ver, req_rpc_ver, supported_ver_len) != 0) {
        cJSON* err = invalid_request("Missing or unsuppoted rpcversion", req_id);
        char* ret = cJSON_Print(err);
        cJSON_Delete(err);

        return ret;
    }

    const char* method_name = get_method(req);
    if (method_name == NULL) {
        cJSON* err = invalid_request("Missing method property", req_id);
        char* ret = cJSON_Print(err);
        cJSON_Delete(err);

        return ret;
    }

    method_handler_t handler = get_method_handler(method_name);
    if (handler == NULL) {
        cJSON* err = method_not_found(req_id);
        char* ret = cJSON_Print(err);
        cJSON_Delete(err);

        return ret;
    }
    

    cJSON* res = handler(get_params(req));
    char* ret = cJSON_Print(res);

    cJSON_Delete(res);
    return ret;
}
