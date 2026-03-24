/**
 * @file settings_screen.c
 * @brief Network interfaces screen with IP display and speed meters.
 * 
 * Shows a list of interfaces (eth0, wifi0, wifi1, lo) with their IP addresses.
 * Also displays a speed meter with two arcs (red for eth0, blue for wifi0).
 */

#include "settings_screen.h"
#include "ui_common.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include <stdio.h>
#include <string.h>
#include "bsp_config.h"

static const char *TAG = "settings_screen";

// List of monitored interfaces (order determines IP display order)
static const char* interface_names[] = {
    "eth0",
    "eth1",
    "wifi0",
    "fqdn",
    NULL
};

typedef struct {
    const char *name;
    lv_obj_t *value_label;      // label for IP address
} interface_item_t;

static interface_item_t s_interfaces[5];
static int s_interface_count = 0;

static lv_obj_t *s_meter = NULL;
static lv_meter_indicator_t *s_arc1 = NULL; // red arc for eth0
static lv_meter_indicator_t *s_arc2 = NULL; // blue arc for wifi0

/**
 * @brief Update speed value for a given interface.
 */
void settings_screen_update_speed(const char *iface, int speed)
{
    if (!s_meter) return;
    if (strcmp(iface, "eth0") == 0 && s_arc1) {
        if (speed < 0) speed = 0;
        if (speed > 10000) speed = 10000;
        if (lvgl_port_lock(0)) {
            lv_meter_set_indicator_end_value(s_meter, s_arc1, speed);
            lvgl_port_unlock();
        }
    } else if (strcmp(iface, "wifi0") == 0 && s_arc2) {
        if (speed < 0) speed = 0;
        if (speed > 10000) speed = 10000;
        if (lvgl_port_lock(0)) {
            lv_meter_set_indicator_end_value(s_meter, s_arc2, speed);
            lvgl_port_unlock();
        }
    }
}

/**
 * @brief Creates the circular speed meter (two arcs).
 */
static void create_speed_meter(lv_obj_t *parent)
{
    s_meter = lv_meter_create(parent);
    lv_obj_set_size(s_meter, METER_SIZE, METER_SIZE);
    lv_obj_align(s_meter, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(s_meter, LV_OPA_TRANSP, 0);   // transparent background
    lv_obj_set_style_radius(s_meter, 8, 0);
    lv_obj_set_style_border_width(s_meter, 0, 0);          // no border

    // Remove the central circle (indicator)
    lv_obj_remove_style(s_meter, NULL, LV_PART_INDICATOR);

    // Add scale
    lv_meter_scale_t *scale = lv_meter_add_scale(s_meter);
    lv_meter_set_scale_ticks(s_meter, scale, 9, 2, 8, lv_color_hex(0x666666));
    lv_meter_set_scale_major_ticks(s_meter, scale, 4, 2, 15, lv_color_hex(0xCCCCCC), -23);
    lv_meter_set_scale_range(s_meter, scale, 0, 10000, 270, 135);

    // Add arcs (indicators)
    s_arc1 = lv_meter_add_arc(s_meter, scale, 5, METER_ARC1_COLOR, 0);
    s_arc2 = lv_meter_add_arc(s_meter, scale, 5, METER_ARC2_COLOR, -10);

    // Set text color for potential tick labels (if added later)
    lv_obj_set_style_text_color(s_meter, lv_color_white(), LV_PART_TICKS);
}

/**
 * @brief Creates the Settings screen.
 * @return Pointer to the created screen object.
 */
lv_obj_t* settings_screen_create(void)
{
    ESP_LOGI(TAG, "Creating Settings screen");

    // Create a separate screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // Header (blue bar with title)
    lv_obj_t *header = create_screen_header(scr, "Network Interfaces");

    // Content container (left‑aligned column)
    lv_obj_t *cont = create_content_container(scr);

    s_interface_count = 0;
    for (int i = 0; interface_names[i] != NULL; i++) {
        // Create a row (horizontal container)
        lv_obj_t *row = lv_obj_create(cont);
        lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_0, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Interface name label (white, content font)
        lv_obj_t *name_label = lv_label_create(row);
        char name_buf[32];
        snprintf(name_buf, sizeof(name_buf), "%s: ", interface_names[i]);
        lv_label_set_text(name_label, name_buf);
        lv_obj_set_style_text_color(name_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(name_label, CONTENT_FONT, 0);

        // IP value label (white initially, becomes green when set)
        lv_obj_t *value_label = lv_label_create(row);
        lv_label_set_text(value_label, "--");
        lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(value_label, CONTENT_FONT, 0);

        s_interfaces[s_interface_count].name = interface_names[i];
        s_interfaces[s_interface_count].value_label = value_label;
        s_interface_count++;
    }

    // Position the content container below the header (margin 35)
    position_content_below_header(cont, header, 43);

    // Create the speed meter in bottom right corner
    create_speed_meter(scr);

    return scr;
}

/**
 * @brief Update IP address for a specific interface.
 */
void settings_screen_update_interface(const char *iface, const char *ip)
{
    for (int i = 0; i < s_interface_count; i++) {
        if (strcmp(s_interfaces[i].name, iface) == 0) {
            if (lvgl_port_lock(0)) {
                if (ip && ip[0] != '\0') {
                    lv_label_set_text(s_interfaces[i].value_label, ip);
                    // Определяем цвет в зависимости от типа интерфейса
                    if (strcmp(iface, "fqdn") == 0) {
                        lv_obj_set_style_text_color(s_interfaces[i].value_label, lv_color_hex(0xFFDD00), 0);
                    } else {
                        lv_obj_set_style_text_color(s_interfaces[i].value_label, lv_color_hex(0x00FF00), 0); // зелёный
                    }
                } else {
                    lv_label_set_text(s_interfaces[i].value_label, "");
                    lv_obj_set_style_text_color(s_interfaces[i].value_label, lv_color_white(), 0);
                }
                lvgl_port_unlock();
            }
            break;
        }
    }
}