#ifndef __CJSON_HELPER_H
#define __CJSON_HELPER_H

#include <stdint.h>

#include "cJSON.h"


const cJSON* get_prop(const cJSON* obj, const char* field_name);
int32_t get_int_prop(const cJSON* obj, const char* field_name);
const char* get_string_prop(const cJSON* obj, const char* field_name);

cJSON* construct_base_obj(const char* id);
cJSON* construct_error(int code, const char* message, const char* id);

cJSON* construct_empty_resp(const char* id);

#endif
