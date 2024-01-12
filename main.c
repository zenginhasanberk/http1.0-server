#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

void* log_running(void* arg) {
    while (1) {
        sleep(5);
        printf("Server is running...\n");
    }
    return NULL;
}

int main() {
    pthread_t log_thread;
    if (pthread_create(&log_thread, NULL, log_running, NULL) != 0) {
        perror("Failed to create log thread");
        return EXIT_FAILURE;
    }

    int socket_id = create_socket();
    bind_to_port(socket_id, 8080);
    start_listening(socket_id);

    while (1) {
        int client_socket = accept_connection(socket_id);

        HttpRequest request = read_and_parse_request(client_socket);
        HttpResponse* response = prepare_response(&request);

        printf("testing\n");
        for (size_t i = 0; i < response->num_headers; ++i) {
            printf("%s %s\r\n", response->headers[i].key, response->headers[i].value);      
        }
        printf("\r\n");
        printf("body\n");
        printf("%s", response->body);


        send_response(client_socket, response);
        free(response);

        /* Connections immediately end after response */
        close_connection(client_socket);
    }

    return 0;
}
