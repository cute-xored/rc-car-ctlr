#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "esp_log.h"
#include "esp_libc.h"

#include "config.h"
#include "logging.h"
#include "rpc.h"

static char* recv_message(int socket) {
    const size_t chunk_size = 1 << 10;
    size_t begin = 0;
    char *buffer = (char*) os_malloc(chunk_size * sizeof(char));

    if (buffer == NULL) {
        ESP_LOGE(
            AP_SERVER_TAG,
            "Failed to allocate memory"
        );

        return NULL;
    }

    int actual_read_size = 0;
    while (1) {
        actual_read_size = recv(socket, buffer + begin, chunk_size, 0);

        if (actual_read_size <= 0) {
            ESP_LOGE(
                AP_SERVER_TAG,
                "Failed to read from socket: %s",
                strerror(errno)
            );

            os_free(buffer);
            return NULL;
        }

        size_t last_char_ind = begin + actual_read_size - 1;
        
        if (buffer[last_char_ind] == 0x04) { // EOT is found
            buffer[last_char_ind] = '\0';

            break;
        }

        begin = begin + actual_read_size;

        char* new_buffer = (char*) os_realloc(buffer, (begin + chunk_size) * sizeof(char));
        if (new_buffer == NULL) {
            ESP_LOGE(
                AP_SERVER_TAG,
                "Failed to allocate memory"
            );

            os_free(buffer);
            return NULL;
        }

        buffer = new_buffer;
    }

    return buffer;
}

void ap_server_task(void* _) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.91.1", &(addr.sin_addr));
    addr.sin_port = htons(AP_SERVER_PORT);

    socklen_t addr_len;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        ESP_LOGE(AP_SERVER_TAG, "Failed to create socket: %s", strerror(errno));
        return;
    }

    int ret = bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));

    if (ret) {
        ESP_LOGE(AP_SERVER_TAG, "Cannot bind socket: %s", strerror(errno));
        return;
    }

    ret = listen(socket_fd, 32);
    if (ret) {
        ESP_LOGE(AP_SERVER_TAG, "Cannot listen socket: %s", strerror(errno));
        return;
    }

    // There is only one live connection is possible, it's ok for now,
    // also that's why we don't need to synchronize this interaction
    while(1) {
        ESP_LOGI(AP_SERVER_TAG, "Waiting for the client...");

        int client_sock_fd = accept(socket_fd, (struct sockaddr*)&addr, &addr_len);
        if (client_sock_fd < 0) {
            ESP_LOGE(AP_SERVER_TAG, "Failed to accept: %s", strerror(errno));
            continue;
        }

        ESP_LOGI(AP_SERVER_TAG, "Connect accepted...");

        char* msg = recv_message(client_sock_fd);
        if (msg == NULL) {
            close(client_sock_fd);
            continue;
        }

        char* res = dispatch(msg);
        const size_t res_len = strlen(res);

        int ret = send(client_sock_fd, res, res_len, 0);
        if (ret <= 0) {
            ESP_LOGE(AP_SERVER_TAG, "Failed to response: %s", strerror(errno));
        }

        ESP_LOGI(AP_SERVER_TAG, "Closing connection...");

        os_free(res);
        os_free(msg);
        close(client_sock_fd);
    }

    return;
}
