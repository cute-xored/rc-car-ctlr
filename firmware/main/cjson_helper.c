#include "cJSON.h"


const char* get_string_prop(const cJSON* obj, const char* field_name) {
    const cJSON* prop = cJSON_GetObjectItemCaseSensitive(obj, field_name);

    if (cJSON_IsString(prop)) {
        return prop->valuestring;
    }

    return NULL;
}

const cJSON* get_prop(const cJSON* obj, const char* field_name) {
    return cJSON_GetObjectItemCaseSensitive(obj, field_name);
}


cJSON* constuct_base_obj(const char* id) {
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "jsonrpc", "2.0");
    if (id != NULL) {
        cJSON_AddStringToObject(obj, "id", id);
    } else {
        cJSON_AddNullToObject(obj, "id");
    }

    return obj;
}

// No need to check errors since we control all the data
cJSON* construct_error(int code, const char* message, const char* id) {
    cJSON* error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);

    cJSON* ret = constuct_base_obj(id);
    cJSON_AddItemToObject(ret, "error", error);

    return ret;
}

