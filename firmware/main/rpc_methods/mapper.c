#include <string.h>

#include "methods.h"


method_handler_t get_method_handler(const char* method_name) {
    if (strncmp(connect_name, method_name, strlen(connect_name)) == 0) {
        return connect;
    }

    if (strncmp(get_connection_info_name, method_name, strlen(get_connection_info_name)) == 0) {
        return get_connection_info;
    }

    return NULL;
}
