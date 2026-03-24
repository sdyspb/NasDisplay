#include "uart_parser.h"
#include <stdio.h>
#include <string.h>

bool uart_parse_line(const char *line, nas_data_t *out)
{
    float used = 0, total = 0;
    char status[32] = {0};
    int n = sscanf(line, "USED=%f,TOTAL=%f,STATUS=%31s", &used, &total, status);
    if (n == 3) {
        out->used = used;
        out->total = total;
        strncpy(out->status, status, sizeof(out->status)-1);
        out->status[sizeof(out->status)-1] = '\0';
        return true;
    }
    return false;
}

bool uart_parse_interface(const char *token, char *iface, size_t iface_size, char *ip, size_t ip_size)
{
    char *eq = strchr(token, '=');
    if (!eq) return false;

    size_t name_len = eq - token;
    if (name_len >= iface_size) name_len = iface_size - 1;
    strncpy(iface, token, name_len);
    iface[name_len] = '\0';

    const char *ip_str = eq + 1;
    strncpy(ip, ip_str, ip_size - 1);
    ip[ip_size - 1] = '\0';

    return true;
}

bool uart_parse_speed(const char *token, char *iface, size_t iface_size, int *speed)
{
    char *eq = strchr(token, '=');
    if (!eq) return false;

    size_t name_len = eq - token;
    if (name_len >= iface_size) name_len = iface_size - 1;
    strncpy(iface, token, name_len);
    iface[name_len] = '\0';

    const char *val_str = eq + 1;
    char *endptr;
    long val = strtol(val_str, &endptr, 10);
    if (endptr == val_str) return false;
    *speed = (int)val;
    return true;
}