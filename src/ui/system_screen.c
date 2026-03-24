/**
 * @file system_screen.c
 * @brief System status screen (CPU, memory, temperature, uptime) with a meter.
 */

#include "system_screen.h"
#include "ui_common.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include <stdio.h>
#include <string.h>
#include "bsp_config.h"

static const char *TAG = "system_screen";

// Pointers to value labels (to update later)
static lv_obj_t *s_cpu_label = NULL;
static lv_obj_t *s_mem_label = NULL;
static lv_obj_t *s_temp_label = NULL;
static lv_obj_t *s_uptime_label = NULL;

// Meter objects
static lv_obj_t *s_meter = NULL;
static lv_meter_indicator_t *s_cpu_arc = NULL;   // red
static lv_meter_indicator_t *s_mem_arc = NULL;   // blue
static lv_meter_indicator_t *s_temp_arc = NULL;  // green

/**
 * @brief Creates the circular meter (0‑100) with three arcs.
 */
static void create_system_meter(lv_obj_t *parent)
{
    s_meter = lv_meter_create(parent);
    lv_obj_set_size(s_meter, METER_SIZE, METER_SIZE);
    lv_obj_align(s_meter, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(s_meter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(s_meter, 8, 0);
    lv_obj_set_style_border_width(s_meter, 0, 0);

    // Remove central circle
    lv_obj_remove_style(s_meter, NULL, LV_PART_INDICATOR);

    // Add scale
    lv_meter_scale_t *scale = lv_meter_add_scale(s_meter);
    lv_meter_set_scale_ticks(s_meter, scale, 11, 2, 8, lv_color_hex(0x666666));
    lv_meter_set_scale_major_ticks(s_meter, scale, 5, 2, 15, lv_color_hex(0xCCCCCC), 5);
    lv_meter_set_scale_range(s_meter, scale, 0, 100, 270, 90);

    // Add three arcs
    s_cpu_arc = lv_meter_add_arc(s_meter, scale, 5, lv_color_hex(0xFF0000), 0);   // red
    s_mem_arc = lv_meter_add_arc(s_meter, scale, 5, lv_color_hex(0x0088FF), -10);  // blue
    s_temp_arc = lv_meter_add_arc(s_meter, scale, 5, lv_color_hex(0x00FF00), -20); // green

    // Set text color (for future tick labels)
    lv_obj_set_style_text_color(s_meter, lv_color_white(), LV_PART_TICKS);
}

/**
 * @brief Creates the System Status screen.
 */
lv_obj_t* system_screen_create(void)
{
    ESP_LOGI(TAG, "Creating System screen");

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Header
    lv_obj_t *header = create_screen_header(scr, "System Status");

    // Content container (left-aligned column)
    lv_obj_t *cont = create_content_container(scr);

    // ---- CPU Usage row ----
    lv_obj_t *cpu_row = lv_obj_create(cont);
    lv_obj_set_size(cpu_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cpu_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(cpu_row, 0, 0);
    lv_obj_set_style_pad_all(cpu_row, 0, 0);
    lv_obj_set_flex_flow(cpu_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cpu_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *cpu_name = lv_label_create(cpu_row);
    lv_label_set_text(cpu_name, "CPU Usage: ");
    lv_obj_set_style_text_color(cpu_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(cpu_name, CONTENT_FONT, 0);

    s_cpu_label = lv_label_create(cpu_row);
    lv_label_set_text(s_cpu_label, "--%");
    lv_obj_set_style_text_color(s_cpu_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_cpu_label, CONTENT_FONT, 0);

    // ---- Memory row ----
    lv_obj_t *mem_row = lv_obj_create(cont);
    lv_obj_set_size(mem_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mem_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(mem_row, 0, 0);
    lv_obj_set_style_pad_all(mem_row, 0, 0);
    lv_obj_set_flex_flow(mem_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mem_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mem_name = lv_label_create(mem_row);
    lv_label_set_text(mem_name, "RAM: ");
    lv_obj_set_style_text_color(mem_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(mem_name, CONTENT_FONT, 0);

    s_mem_label = lv_label_create(mem_row);
    lv_label_set_text(s_mem_label, "--/-- GB");
    lv_obj_set_style_text_color(s_mem_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_mem_label, CONTENT_FONT, 0);

    // ---- CPU Temperature row ----
    lv_obj_t *temp_row = lv_obj_create(cont);
    lv_obj_set_size(temp_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(temp_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(temp_row, 0, 0);
    lv_obj_set_style_pad_all(temp_row, 0, 0);
    lv_obj_set_flex_flow(temp_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *temp_name = lv_label_create(temp_row);
    lv_label_set_text(temp_name, "CPU Temp: ");
    lv_obj_set_style_text_color(temp_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(temp_name, CONTENT_FONT, 0);

    s_temp_label = lv_label_create(temp_row);
    lv_label_set_text(s_temp_label, "--°C");
    lv_obj_set_style_text_color(s_temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_temp_label, CONTENT_FONT, 0);

    // ---- Uptime row ----
    lv_obj_t *uptime_row = lv_obj_create(cont);
    lv_obj_set_size(uptime_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(uptime_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(uptime_row, 0, 0);
    lv_obj_set_style_pad_all(uptime_row, 0, 0);
    lv_obj_set_flex_flow(uptime_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(uptime_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *uptime_name = lv_label_create(uptime_row);
    lv_label_set_text(uptime_name, "Uptime: ");
    lv_obj_set_style_text_color(uptime_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(uptime_name, CONTENT_FONT, 0);

    s_uptime_label = lv_label_create(uptime_row);
    lv_label_set_text(s_uptime_label, "-- days");
    lv_obj_set_style_text_color(s_uptime_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_uptime_label, CONTENT_FONT, 0);

    position_content_below_header(cont, header, 43);

    create_system_meter(scr);

    return scr;
}

/**
 * @brief Update all system data on the screen.
 */
void system_screen_update(int cpu, float mem_used, float mem_total, int temp, float uptime)
{
    char buf[64];
    if (lvgl_port_lock(0)) {
        // CPU usage
        snprintf(buf, sizeof(buf), "%d%%", cpu);
        lv_label_set_text(s_cpu_label, buf);
        lv_obj_set_style_text_color(s_cpu_label, lv_color_hex(0x00FF00), 0);

        // Memory
        snprintf(buf, sizeof(buf), "%.1f/%.1f GB", mem_used, mem_total);
        lv_label_set_text(s_mem_label, buf);
        lv_obj_set_style_text_color(s_mem_label, lv_color_hex(0x00FF00), 0);

        // Temperature
        snprintf(buf, sizeof(buf), "%d°C", temp);
        lv_label_set_text(s_temp_label, buf);
        lv_obj_set_style_text_color(s_temp_label, lv_color_hex(0x00FF00), 0);

        // Uptime – convert weeks to days
        float days = uptime * 7.0f;
        snprintf(buf, sizeof(buf), "%.1f days", days);
        lv_label_set_text(s_uptime_label, buf);
        lv_obj_set_style_text_color(s_uptime_label, lv_color_hex(0x00FF00), 0);

        // Update meter
        if (s_meter) {
            // CPU load (red)
            int cpu_val = cpu;
            if (cpu_val < 0) cpu_val = 0;
            if (cpu_val > 100) cpu_val = 100;
            lv_meter_set_indicator_end_value(s_meter, s_cpu_arc, cpu_val);

            // Memory usage percentage
            int mem_percent = 0;
            if (mem_total > 0) {
                mem_percent = (int)((mem_used / mem_total) * 100);
                if (mem_percent > 100) mem_percent = 100;
            }
            lv_meter_set_indicator_end_value(s_meter, s_mem_arc, mem_percent);

            // Temperature (clamp to 0-100)
            int temp_val = temp;
            if (temp_val < 0) temp_val = 0;
            if (temp_val > 100) temp_val = 100;
            lv_meter_set_indicator_end_value(s_meter, s_temp_arc, temp_val);
        }

        lvgl_port_unlock();
    }
}