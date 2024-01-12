#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_objects.h"

void parse_request_line(unsigned char* request_bytes, HttpRequest* request_obj) {
    sscanf((char*)request_bytes, "%s %s %s", request_obj->method, request_obj->uri,
           request_obj->version);
}

void parse_header_line(unsigned char* line_start, size_t line_length,
                       HttpHeader* header) {
    char line[150];
    strncpy(line, (char*)line_start, line_length);
    line[line_length] = '\0'; // need for sscanf to work
    sscanf(line, "%49[^:]: %99[^\r\n]", header->key, header->value);
    // TODO: Add error checking for key-value pairs
}

int get_content_length(HttpRequest* request_obj) {
    for (size_t i = 0; i < request_obj->num_headers; ++i) {
        if (strcmp(request_obj->headers[i].key, "Content-Length") == 0) {
            return atoi(request_obj->headers[i].value);
        }
    }
    /* If no Content-Length, then default to 0 */
    return -1;
}
