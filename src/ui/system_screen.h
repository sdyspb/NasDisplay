#ifndef SYSTEM_SCREEN_H
#define SYSTEM_SCREEN_H

#include "lvgl.h"

lv_obj_t* system_screen_create(void);
void system_screen_update(int cpu, float mem_used, float mem_total, int temp, float uptime);

#endif