#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

typedef struct {
    char* method;
    char* uri;
    char* version;
    char* body;
} HttpRequest;

#endif
