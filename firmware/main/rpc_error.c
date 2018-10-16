#include <string.h>

#include "esp_libc.h"

#include "cJSON.h"


// No need to check errors since we control all the data
cJSON* construct_error(int code, const char* message, int id) {
    cJSON* ret = cJSON_CreateObject();
    cJSON* error = cJSON_CreateObject();

    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);

    if (id >= 0) {
        cJSON_AddNumberToObject(ret, "id", id);
    } else {
        cJSON_AddNullToObject(ret, "id");
    }
    
    cJSON_AddStringToObject(ret, "jsonrpc", "2.0");
    cJSON_AddItemToObject(ret, "error", error);

    return ret;
}

static char* concat(const char* a, const char* b) {
    const size_t a_len = strlen(a);
    const size_t b_len = strlen(b);
    const size_t ret_len = a_len + b_len + 1;

    char* ret = os_zalloc(ret_len * sizeof(char));
    strncat(ret, a, a_len);
    strncat(ret, b, b_len);

    return ret;
}

cJSON* parse_error(const char* details) {
    const int code = -32700;
    char* error = concat("Parse error: ", details);

    cJSON* ret = construct_error(code, error, -1);
    
    os_free(error);
    return ret;
}

cJSON* invalid_request(const char* details, int id) {
    const int code = -32600;
    char* error = concat("Invalid request: ", details);

    cJSON* ret = construct_error(code, error, id);
    
    os_free(error);
    return ret;
}

cJSON* method_not_found(int id) {
    return construct_error(-32601, "Method not found", id);
}
