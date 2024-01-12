#include "server.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "helpers.h"

const size_t MAX_HEADERS = 10;
const char* SERVER_FOLDER = "./files/";
const char* POST_ENDPOINTS[] = {"/addimg", "/addpost"};
const size_t NUM_ENDPOINTS = 2;

int create_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(-1);
    }

    int option = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("Socket option cannot be set!");
        exit(-1);
    }
    return sockfd;
}

void bind_to_port(int socket, int port) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    // Internet
    server_address.sin_family = AF_INET;

    // Port number
    server_address.sin_port = htons(port);

    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding to socket");
        exit(-1);
    }
}

void start_listening(int socket) {
    if (listen(socket, 5) < 0) {
        perror("Error starting listening on socket");
        exit(-1);
    }
}

int accept_connection(int server_socket) {
    struct sockaddr_in client_info;
    socklen_t client_len = sizeof(client_info);

    int client_socket =
        accept(server_socket, (struct sockaddr*)&client_info, &client_len);
    if (client_socket < 0) {
        perror("Error accepting client connection");
        exit(-1);
    }
    return client_socket;
}

/* Using unsigned char* for byte array! */
/* The big assumption in this function is that all the content is sent at once, properly formatted. */
HttpRequest read_and_parse_request(int client_socket) {
    size_t buffer_size = 512;
    unsigned char* buffer = malloc(buffer_size);

    if (buffer == NULL) {
        perror("Could not allocate heap memory!");
        exit(-1);
    }

    int bytes_read = 0;
    HttpRequest request_obj;

    bytes_read = recv(client_socket, buffer, buffer_size, 0);
    if (bytes_read < 0) {
        perror("Error reading from socket!");
        free(buffer);
        exit(-1);
    }
    if (bytes_read == buffer_size) {
        perror("HTTP content more than allowed!");
        free(buffer);
        exit(-1);
    }

    buffer[bytes_read] = '\0';

    printf("%s", buffer);

    parse_request_except_body(buffer, &request_obj, bytes_read);

    int content_length = get_content_length(&request_obj);
    if (strcmp(request_obj.method, "POST") == 0 && content_length == -1) {
        printf("content length not specified for POST request!\n");
        exit(-1);
    }

    if (content_length > 0) {
        size_t end_of_header = strstr((char*) buffer, "\r\n\r\n") - (char*) buffer + 4;
        parse_body(buffer, &request_obj, end_of_header, content_length);
    } else {
        request_obj.body_length = 0;
    }

    free(buffer);
    return request_obj;
}

void parse_request_except_body(unsigned char* request_bytes, HttpRequest* request_obj,
        size_t data_length) {
    request_obj->num_headers = 0;

    // Use pointer arithmetic to find the index of the first header line.
    size_t request_line_end = strstr((char*) request_bytes, "\r\n") - (char*) request_bytes + 2; 
    if (request_line_end == 0) {
        perror("Invalid HTTP request!");
        exit(-1);
    }
    parse_request_line(request_bytes, request_obj);

    size_t current_pos = request_line_end;
    size_t header_end = strstr((char*) request_bytes, "\r\n\r\n") - (char*) request_bytes;
    size_t i = 0;
    size_t line_end;

    while (current_pos < header_end && i < MAX_HEADERS) {
        line_end = strstr((char*) request_bytes + current_pos, "\r\n") - (char*) request_bytes;

        parse_header_line(request_bytes + current_pos, line_end - current_pos,
                &request_obj->headers[i++]);

        // Move the current_pos to after "\r\n"
        current_pos = line_end + 2;
        ++(request_obj->num_headers);
    }
}

void parse_body(unsigned char* request_bytes, HttpRequest* request_obj,
        size_t header_end, int content_length) {
    if (content_length > sizeof(request_obj->body)) {
        perror("HTTP content bigger than allowed!");
        exit(-1);
    }
    memcpy(request_obj->body, request_bytes + header_end, content_length);
    request_obj->body[content_length] = '\0';
    request_obj->body_length = content_length;
}

HttpResponse* prepare_response(const HttpRequest* request) {
    HttpResponse* obj = (HttpResponse*) malloc(sizeof(HttpResponse));

    char fullPath[200];
    strcpy(fullPath, SERVER_FOLDER);
    strncat(fullPath, request->uri, sizeof(fullPath) - strlen(SERVER_FOLDER) - 1);

    if (obj == NULL) {
        perror("Can't allocate memory for response!");
        exit(-1);
    }

    if (strcmp(request->method, "GET") == 0) {
        if (access(fullPath, F_OK) != 0) {
            obj->status_code = 200;
            strcpy(obj->reason_phrase, "OK");

            FILE* file = fopen(fullPath, "r");
            if (file == NULL) {
                perror("Error opening file");
                exit(-1);
            }
            size_t bytes_read = fread(obj->body, 1, sizeof(obj->body), file);
            fclose(file);
            obj->body[bytes_read] = '\0';
            strcpy(obj->headers[0].value, "text/html");
            sprintf(obj->headers[1].value, "%d",(int) bytes_read);
        }
        else {
            obj->status_code = 404;
            strcpy(obj->reason_phrase, "NOT FOUND");

            sprintf(obj->body, 
                    "<html>\n"
                    "<head><title>404 Not Found</title></head>\n"
                    "<body>\n"
                    "<h1>Not Found</h1>\n"
                    "<p>The requested URL %s was not found on this server.</p>\n"
                    "</body>\n"
                    "</html>\n", fullPath);

            strcpy(obj->headers[0].value, "text/html");
            sprintf(obj->headers[1].value, "%d", (int)strlen(obj->body));
        }

    } else if (strcmp(request->method, "POST") == 0) {
        int found = 0;

        for (size_t i=0; i < NUM_ENDPOINTS; ++i) {
            if (strcmp(POST_ENDPOINTS[i], request->uri) == 0) {
                    found = 1;
                }
        }

        if (!found) {
            obj->status_code = 404;
            strcpy(obj->reason_phrase, "NOT FOUND");
            sprintf(obj->body, 
                    "<html>\n"
                    "<head><title>404 Not Found</title></head>\n"
                    "<body>\n"
                    "<h1>Not Found</h1>\n"
                    "<p>The requested URL %s was not found on this server.</p>\n"
                    "</body>\n"
                    "</html>\n", fullPath);

            strcpy(obj->headers[0].value, "text/html");
            sprintf(obj->headers[1].value, "%d", (int)strlen(obj->body));
        } else {
            obj->status_code = 200;
            strcpy(obj->reason_phrase, "OK");
            strcpy(obj->headers[0].value, "text/html"); 
            strcpy(obj->body, 
                    "<html>\n"
                    "<head><title>200 OK</title></head>\n"
                    "<body>\n"
                    "<h1>Done</h1>\n"
                    "<p>Request handled successfully.</p>\n"
                    "</body>\n"
                    "</html>\n");
        }
    }
    // TODO: Handle other kinds of requests.
    else {
        perror("This request is not currently supported");
        exit(-1);
    } 
    strcpy(obj->headers[0].key, "Content-Type"); 
    strcpy(obj->headers[1].key, "Content-Length");

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strcpy(obj->headers[2].key, "Date");
    sprintf(obj->headers[2].value, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    strcpy(obj->headers[3].key, "Server");
    strcpy(obj->headers[3].value, "hbzengin HTTP 1.0");

    strcpy(obj->headers[4].key, "Connection");
    strcpy(obj->headers[4].value, "close");

    obj->num_headers = 5;

    return obj;
}

void send_response(int client_socket, HttpResponse* response) {
    char response_str[2048];
    int offset = 0;  // To keep track of the current position in response_str

    // Construct the status line
    offset += snprintf(response_str + offset, sizeof(response_str) - offset, "HTTP/1.0 %d %s\r\n",
                       response->status_code, response->reason_phrase);

    // Append headers
    for (size_t i = 0; i < response->num_headers; ++i) {
        offset += snprintf(response_str + offset, sizeof(response_str) - offset, "%s: %s\r\n",
                           response->headers[i].key, response->headers[i].value);
    }

    // Append an extra line break to end headers
    offset += snprintf(response_str + offset, sizeof(response_str) - offset, "\r\n");

    // Append body
    snprintf(response_str + offset, sizeof(response_str) - offset, "%s", response->body);

    // Send the response
    if (send(client_socket, response_str, strlen(response_str), 0) < 0) {
        perror("Unable to send response back to the client!");
        exit(-1);
    }
}

/* My server will close the connection to client once the response is sent. */
void close_connection(int client_socket) { close(client_socket); }
