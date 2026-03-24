#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t task_priority;
    uint32_t task_stack;
    int task_affinity;
    uint32_t task_max_sleep_ms;
    uint32_t timer_period_ms;
} lvgl_port_cfg_t;

esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

typedef struct {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t panel_handle;
    size_t buffer_size;
    bool double_buffer;
    uint16_t hres;
    uint16_t vres;
    bool monochrome;
    struct {
        bool swap_xy;
        bool mirror_x;
        bool mirror_y;
    } rotation;
    struct {
        bool buff_dma;
        bool swap_bytes;
    } flags;
} lvgl_port_display_cfg_t;

lv_disp_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *cfg);

// Touch doesn't used, so
// typedef struct {
//     lv_disp_t *disp;
//     esp_lcd_touch_handle_t handle;
// } lvgl_port_touch_cfg_t;
// lv_indev_t *lvgl_port_add_touch(const lvgl_port_touch_cfg_t *cfg);

bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif