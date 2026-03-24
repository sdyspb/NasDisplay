#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "lvgl.h"

lv_obj_t* settings_screen_create(void);
void settings_screen_update_interface(const char *iface, const char *ip);
void settings_screen_update_speed(const char *iface, int speed);

#endif