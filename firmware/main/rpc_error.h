#ifndef __ERROR_H
#define __ERROR_H

#include "cJSON.h"

cJSON* parse_error(const char* details);
cJSON* invalid_request(const char* details, const char* id);
cJSON* method_not_found(const char* id);

#endif
