#include <string.h>

#include "http_objects.h"

int create_socket();

void bind_to_port(int socket, int port);

void start_listening(int socket);

int accept_connection(int server_socket);

HttpRequest read_and_parse_request(int client_socket);

void parse_request_except_body(unsigned char* request_bytes, HttpRequest* request_obj,
                               size_t data_length);

void parse_body(unsigned char* request_bytes, HttpRequest* request_obj,
                size_t header_end, int content_length);

HttpResponse* prepare_response(const HttpRequest* request);

void send_response(int client_socket, HttpResponse* response);

void close_connection(int client_socket);
