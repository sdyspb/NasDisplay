#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl.h"

// Macros for content styling
#define CONTENT_FONT &lv_font_montserrat_18
#define CONTENT_ROW_PAD 8

// Global pointers to NAS value labels (used by nas_screen.c and main.c)
extern lv_obj_t *g_used_value_label;
extern lv_obj_t *g_total_value_label;
extern lv_obj_t *g_status_value_label;

// Helper functions
lv_obj_t *create_screen_header(lv_obj_t *parent, const char *text);
void position_content_below_header(lv_obj_t *content, lv_obj_t *header, int margin);
lv_obj_t *create_content_container(lv_obj_t *parent);

#endif