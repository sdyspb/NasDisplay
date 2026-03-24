#ifndef UART_PARSER_H
#define UART_PARSER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float used;
    float total;
    char status[32];
} nas_data_t;

bool uart_parse_line(const char *line, nas_data_t *out);
bool uart_parse_interface(const char *token, char *iface, size_t iface_size, char *ip, size_t ip_size);
bool uart_parse_speed(const char *token, char *iface, size_t iface_size, int *speed);

#endif