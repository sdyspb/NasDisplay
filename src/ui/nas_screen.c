/**
 * @file nas_screen.c
 * @brief NAS storage screen implementation.
 * 
 * Displays used/total storage, status, and a usage progress bar.
 */

#include "nas_screen.h"
#include "ui_common.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include <stdio.h>
#include "bsp_config.h"

static const char *TAG = "nas_screen";

// Global pointers to value labels (for updates from UART)
lv_obj_t *g_used_value_label = NULL;
lv_obj_t *g_total_value_label = NULL;
lv_obj_t *g_status_value_label = NULL;

// Static pointer to the usage bar
static lv_obj_t *s_usage_bar = NULL;

/**
 * @brief Draw event callback for the usage bar.
 *        Draws the percentage text on the bar.
 */
static void bar_event_cb(lv_event_t *e)
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part != LV_PART_INDICATOR) return;

    lv_obj_t *bar = lv_event_get_target(e);
    int32_t value = lv_bar_get_value(bar);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)value);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = &lv_font_montserrat_20;
    label_dsc.letter_space = 0;
    label_dsc.line_space = 0;
    label_dsc.flag = LV_TEXT_FLAG_NONE;

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space,
                    label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    // Decide whether to put text inside or outside the indicator
    if (lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        txt_size.x += 10;
        label_dsc.color = lv_color_white();
        lv_area_t txt_area;
        txt_area.x1 = dsc->draw_area->x2 - txt_size.x;
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
        txt_area.y2 = txt_area.y1 + txt_size.y - 1;
        lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
    } else {
        label_dsc.color = lv_color_black();
        lv_area_t txt_area;
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
        txt_area.y2 = txt_area.y1 + txt_size.y - 1;
        lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
    }
}

/**
 * @brief Creates the usage progress bar.
 */
static void create_usage_bar(lv_obj_t *parent)
{
    s_usage_bar = lv_bar_create(parent);
    lv_obj_set_size(s_usage_bar, NAS_USAGE_BAR_WIDTH, 25);
    lv_obj_align(s_usage_bar, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(s_usage_bar, lv_color_hex(0x88AAFF), 0);
    lv_obj_set_style_bg_opa(s_usage_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_usage_bar, 4, 0);
    lv_obj_set_style_anim_time(s_usage_bar, 0, 0);
    lv_bar_set_range(s_usage_bar, 0, 100);
    lv_obj_set_style_bg_color(s_usage_bar, lv_color_hex(0x0088FF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_usage_bar, 4, LV_PART_INDICATOR);
    lv_obj_add_event_cb(s_usage_bar, bar_event_cb, LV_EVENT_DRAW_PART_END, NULL);
}

/**
 * @brief Updates the usage bar value based on used/total.
 */
static void update_usage_bar(float used, float total)
{
    if (!s_usage_bar) return;
    if (total <= 0) {
        lv_bar_set_value(s_usage_bar, 0, LV_ANIM_OFF);
        return;
    }
    int percent = (int)((used / total) * 100);
    if (percent > 100) percent = 100;
    lv_bar_set_value(s_usage_bar, percent, LV_ANIM_OFF);
}

/**
 * @brief Creates the NAS storage screen.
 * @return Pointer to the created screen object.
 */
lv_obj_t* nas_screen_create(void)
{
    ESP_LOGI(TAG, "Creating NAS screen");

    // Create a separate screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Create header (blue bar with title)
    lv_obj_t *header = create_screen_header(scr, "NAS Storage");

    // Create content container (left‑aligned, column flex)
    lv_obj_t *cont = create_content_container(scr);

    // ---- Used row ----
    lv_obj_t *used_row = lv_obj_create(cont);
    lv_obj_set_size(used_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(used_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(used_row, 0, 0);
    lv_obj_set_style_pad_all(used_row, 0, 0);
    lv_obj_set_flex_flow(used_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(used_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *used_name = lv_label_create(used_row);
    lv_label_set_text(used_name, "Used: ");
    lv_obj_set_style_text_color(used_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(used_name, CONTENT_FONT, 0);   // use global content font

    g_used_value_label = lv_label_create(used_row);
    lv_label_set_text(g_used_value_label, "-- GB");
    lv_obj_set_style_text_color(g_used_value_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(g_used_value_label, CONTENT_FONT, 0);

    // ---- Total row ----
    lv_obj_t *total_row = lv_obj_create(cont);
    lv_obj_set_size(total_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(total_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(total_row, 0, 0);
    lv_obj_set_style_pad_all(total_row, 0, 0);
    lv_obj_set_flex_flow(total_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(total_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *total_name = lv_label_create(total_row);
    lv_label_set_text(total_name, "Total: ");
    lv_obj_set_style_text_color(total_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(total_name, CONTENT_FONT, 0);

    g_total_value_label = lv_label_create(total_row);
    lv_label_set_text(g_total_value_label, "-- GB");
    lv_obj_set_style_text_color(g_total_value_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(g_total_value_label, CONTENT_FONT, 0);

    // ---- Status row ----
    lv_obj_t *status_row = lv_obj_create(cont);
    lv_obj_set_size(status_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(status_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(status_row, 0, 0);
    lv_obj_set_style_pad_all(status_row, 0, 0);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *status_name = lv_label_create(status_row);
    lv_label_set_text(status_name, "Status: ");
    lv_obj_set_style_text_color(status_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_name, CONTENT_FONT, 0);

    g_status_value_label = lv_label_create(status_row);
    lv_label_set_text(g_status_value_label, "--");
    lv_obj_set_style_text_color(g_status_value_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(g_status_value_label, CONTENT_FONT, 0);

    // Position the content container below the header (margin 35)
    position_content_below_header(cont, header, 43);

    // Create the usage bar
    create_usage_bar(scr);

    return scr;
}

/**
 * @brief Updates all NAS data on the screen.
 * @param data Pointer to parsed NAS data.
 */
void nas_screen_update(const nas_data_t *data)
{
    if (!data) return;

    char buf[64];
    if (lvgl_port_lock(0)) {
        // Used value
        snprintf(buf, sizeof(buf), "%.1f GB", data->used);
        lv_label_set_text(g_used_value_label, buf);
        lv_obj_set_style_text_color(g_used_value_label, lv_color_hex(0x00FF00), 0);

        // Total value
        snprintf(buf, sizeof(buf), "%.1f GB", data->total);
        lv_label_set_text(g_total_value_label, buf);
        lv_obj_set_style_text_color(g_total_value_label, lv_color_hex(0x00FF00), 0);

        // Status value
        lv_label_set_text(g_status_value_label, data->status);
        if (strcmp(data->status, "OK") == 0) {
            lv_obj_set_style_text_color(g_status_value_label, lv_color_hex(0x00FF00), 0);
        } else {
            lv_obj_set_style_text_color(g_status_value_label, lv_color_hex(0xFF0000), 0);
        }

        // Update the usage bar
        update_usage_bar(data->used, data->total);

        lvgl_port_unlock();
    }
}