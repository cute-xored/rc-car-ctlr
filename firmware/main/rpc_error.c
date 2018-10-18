#include <string.h>

#include "esp_libc.h"

#include "cJSON.h"

#include "cjson_helper.h"


static const int WIFI_ERROR_CODE = -32001;


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

    cJSON* ret = construct_error(code, error, NULL);
    
    os_free(error);
    return ret;
}

cJSON* invalid_request(const char* details, const char* id) {
    const int code = -32600;
    char* error = concat("Invalid request: ", details);

    cJSON* ret = construct_error(code, error, id);
    
    os_free(error);
    return ret;
}

cJSON* method_not_found(const char* id) {
    return construct_error(-32601, "Method not found", id);
}

cJSON* wifi_error(const char* details, const char* id) {
    char* error = concat("Wifi error: ", details);

    cJSON* ret = construct_error(WIFI_ERROR_CODE, error, id);

    os_free(error);
    return ret;
}
