#ifndef BSP_DISPLAY_H
#define BSP_DISPLAY_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

void bsp_display_init(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel, size_t max_transfer_sz);
void bsp_display_brightness_init(void);
void bsp_display_set_brightness(uint8_t brightness);
uint8_t bsp_display_get_brightness(void);

#endif