#include "uart_nas_parser.h"
#include <string.h>
#include <stdio.h>

bool uart_nas_parse(const char *line, nas_data_t *out)
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