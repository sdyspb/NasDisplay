#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"

static SemaphoreHandle_t lvgl_mutex = NULL;

esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg)
{
    lv_init();
    lvgl_mutex = xSemaphoreCreateMutex();
    if (!lvgl_mutex) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    lvgl_port_display_cfg_t *disp_cfg = (lvgl_port_display_cfg_t *)drv->user_data;
    esp_lcd_panel_draw_bitmap(disp_cfg->panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

lv_disp_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *cfg)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_disp_drv_t disp_drv;

    void *buf1 = heap_caps_malloc(cfg->buffer_size, MALLOC_CAP_DMA);
    void *buf2 = cfg->double_buffer ? heap_caps_malloc(cfg->buffer_size, MALLOC_CAP_DMA) : NULL;
    if (!buf1) {
        ESP_LOGE("lvgl_port", "No buffer");
        return NULL;
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, cfg->buffer_size);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = cfg->hres;
    disp_drv.ver_res = cfg->vres;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = (void *)cfg;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    return disp;
}

bool lvgl_port_lock(uint32_t timeout_ms)
{
    return xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    xSemaphoreGive(lvgl_mutex);
}