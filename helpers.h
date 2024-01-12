#include <stdio.h>
#include <string.h>

#include "http_objects.h"

/* Parse the request line of the HTTP request */
void parse_request_line(unsigned char* request_bytes, HttpRequest* request_obj);

/* Find next line's end given a starting pos */
size_t find_next_line_end(unsigned char* request_bytes, size_t start,
                          size_t bytes_length);

/* Return end of headers given a byte array */
size_t find_header_end(unsigned char* request_bytes, size_t length);

/* Parse header line given start and length and store in a HttpHeader object */
void parse_header_line(unsigned char* line_start, size_t line_length, HttpHeader* header);

/* Find the Content-Length of a HTTP request */
size_t get_content_length(HttpRequest* request_obj);
