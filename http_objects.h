#ifndef HTTP_OBJECTS_H
#define HTTP_OBJECTS_H

#include <stddef.h>

typedef struct {
    char key[50];
    char value[100];
} HttpHeader;

typedef struct {
    char method[10];
    char uri[50];
    char version[10];
    HttpHeader headers[10];
    char body[500];
    size_t num_headers;
    size_t body_length;
} HttpRequest;

typedef struct {
    int status_code;
    char reason_phrase[20];
    HttpHeader headers[10];
    char body[1000];
    size_t num_headers;
} HttpResponse;

#endif
