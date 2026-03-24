#ifndef NAS_SCREEN_H
#define NAS_SCREEN_H

#include "lvgl.h"
#include "uart/uart_parser.h"   // для nas_data_t

lv_obj_t* nas_screen_create(void);
void nas_screen_update(const nas_data_t *data);

#endif