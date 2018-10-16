#ifndef __METHODS_H
#define __METHODS_H

#include "cJSON.h"

typedef cJSON* (*method_handler_t)(const cJSON*);

extern const char* connect_name;
cJSON* connect(const cJSON* params);

extern const char* get_connection_info_name;
cJSON* get_connection_info(const cJSON* params);


method_handler_t get_method_handler(const char* method_name);

#endif
