#ifndef BSP_UART_H
#define BSP_UART_H

#include "esp_err.h"

typedef void (*uart_nas_callback_t)(const char *line);

void bsp_uart_init(void);
void bsp_uart_register_callback(uart_nas_callback_t callback);
void bsp_uart_poll(void);

#endif