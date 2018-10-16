#ifndef __ERROR_H
#define __ERROR_H

#include "cJSON.h"

cJSON* construct_error(int code, const char* message, int id);
cJSON* parse_error(const char* details);
cJSON* invalid_request(const char* details, int id);
cJSON* method_not_found(int id);

#endif
