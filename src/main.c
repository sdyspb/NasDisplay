/**
 * @file main.c
 * @brief Main application entry point.
 *
 * This file initialises the hardware (SPI, display, backlight, touch, UART),
 * sets up LVGL, creates three screens (NAS, Settings, System), and runs the
 * main event loop. It also handles UART callbacks to parse incoming data
 * (NAS usage, interface IPs, speeds, system status) and updates the
 * corresponding UI elements. Screen switching can be triggered either by a
 * timer (every 10 seconds) or by a touch interrupt on GPIO21.
 */

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_lvgl_port.h"
#include "bsp_display.h"
#include "bsp_spi.h"
#include "driver/gpio.h"
#include "bsp_config.h"
#include "bsp_uart.h"
#include "uart/uart_parser.h"
#include "ui/ui_common.h"
#include "ui/nas_screen.h"
#include "ui/settings_screen.h"
#include "ui/system_screen.h"
#include "esp_timer.h"

static const char *TAG = "main";

// LVGL display handles
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lvgl_port_display_cfg_t s_disp_cfg;

// Screen objects
static lv_obj_t *nas_screen = NULL;
static lv_obj_t *settings_screen = NULL;
static lv_obj_t *system_screen = NULL;
static lv_obj_t *current_screen = NULL;

// Timer for automatic screen switching (10 seconds)
static uint64_t last_switch_time = 0;
static const uint64_t switch_interval_us = 10 * 1000 * 1000;

// Touch interrupt handling (GPIO21, active low)
static volatile bool touch_interrupt_pending = false;
static uint64_t last_touch_time = 0;

/**
 * @brief Touch interrupt service routine (ISR).
 * @param arg Unused.
 */
static void IRAM_ATTR touch_isr_handler(void *arg)
{
    touch_interrupt_pending = true;
}

/**
 * @brief Initialise GPIO for touch (interrupt on falling edge).
 */
static void gpio_touch_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_TP_INT),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(EXAMPLE_PIN_TP_INT, touch_isr_handler, NULL);
    ESP_LOGI(TAG, "Touch interrupt configured on GPIO %d", EXAMPLE_PIN_TP_INT);
}

/**
 * @brief UART callback – called whenever a complete line is received.
 *
 * It tries to parse the line in the following order:
 * 1. Speed data (SPEED:eth0=...)
 * 2. NAS data (USED=...,TOTAL=...,STATUS=...)
 * 3. System status (CPU=...,MemUsed=...,MemTotal=...,Temp=...,Uptime=...)
 * 4. Interface IP addresses (eth0=...,eth1=...,wifi0=...,fqdn=...)
 */
static void uart_callback(const char *line)
{
    // 1. SPEED data (format: SPEED:eth0=7500,wifi0=2500)
    if (strncmp(line, "SPEED:", 6) == 0) {
        char line_copy[UART_NAS_BUF_SIZE];
        strncpy(line_copy, line + 6, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy)-1] = '\0';

        char *token = strtok(line_copy, ",");
        while (token != NULL) {
            char iface[32];
            int speed;
            if (uart_parse_speed(token, iface, sizeof(iface), &speed)) {
                settings_screen_update_speed(iface, speed);
                ESP_LOGI("UART", "Speed %s = %d", iface, speed);
            } else {
                ESP_LOGW("UART", "Failed to parse speed token: %s", token);
            }
            token = strtok(NULL, ",");
        }
        return;
    }

    // 2. NAS data (format: USED=120.5,TOTAL=500,STATUS=OK)
    nas_data_t nas_data;
    if (uart_parse_line(line, &nas_data)) {
        nas_screen_update(&nas_data);
        ESP_LOGI("UART", "Updated NAS UI: used=%.1f, total=%.1f, status=%s",
                 nas_data.used, nas_data.total, nas_data.status);
        return;
    }

    // 3. System status (format: CPU=45,MemUsed=2.5,MemTotal=8,Temp=65,Uptime=2.3)
    int cpu = -1, temp = -1;
    float mem_used = -1, mem_total = -1, uptime = -1;
    if (strstr(line, "CPU=") && strstr(line, "MemUsed=") && strstr(line, "MemTotal=") &&
        strstr(line, "Temp=") && strstr(line, "Uptime=")) {
        if (sscanf(line, "CPU=%d,MemUsed=%f,MemTotal=%f,Temp=%d,Uptime=%f",
                   &cpu, &mem_used, &mem_total, &temp, &uptime) == 5) {
            system_screen_update(cpu, mem_used, mem_total, temp, uptime);
            ESP_LOGI("UART", "System: CPU=%d%%, Mem=%.1f/%.1f GB, Temp=%d°C, Uptime=%.1f weeks",
                     cpu, mem_used, mem_total, temp, uptime);
            return;
        }
    }

    // 4. Interface IP addresses (format: eth0=192.168.1.100,eth1=...,wifi0=...,fqdn=...)
    char line_copy[UART_NAS_BUF_SIZE];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy)-1] = '\0';

    char *token = strtok(line_copy, ",");
    while (token != NULL) {
        char iface[32];
        char ip[64];
        if (uart_parse_interface(token, iface, sizeof(iface), ip, sizeof(ip))) {
            settings_screen_update_interface(iface, ip);
            ESP_LOGI("UART", "Interface %s = %s", iface, ip);
        } else {
            ESP_LOGW("UART", "Failed to parse token: %s", token);
        }
        token = strtok(NULL, ",");
    }
}

/**
 * @brief Initialise LVGL and register the display driver.
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
static esp_err_t app_lvgl_init(void)
{
    lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 1024 * 10,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    s_disp_cfg = (lvgl_port_display_cfg_t){
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = { .buff_dma = true, .swap_bytes = true }
    };

    lv_disp_t *disp = lvgl_port_add_disp(&s_disp_cfg);
    if (!disp) return ESP_FAIL;
    lv_disp_set_default(disp);
    return ESP_OK;
}

/**
 * @brief Main application entry point.
 */
void app_main(void)
{
    // Initialise NVS (required for some ESP‑IDF components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Start");

    // Initialise SPI bus for the display
    bsp_spi_init();

    // Initialise the display (JD9853)
    bsp_display_init(&io_handle, &panel_handle,
                     EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT);

    // Hardware orientation adjustments (landscape mode)
    esp_lcd_panel_set_gap(panel_handle, 0, 0);
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_set_gap(panel_handle, 0, 34);
    esp_lcd_panel_mirror(panel_handle, true, false);

    // Turn on backlight via GPIO (simple on/off)
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << EXAMPLE_PIN_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(EXAMPLE_PIN_LCD_BL, 1);
    ESP_LOGI(TAG, "Backlight ON");

    // Initialise LVGL
    ESP_ERROR_CHECK(app_lvgl_init());

    // Create the three screens
    nas_screen = nas_screen_create();
    settings_screen = settings_screen_create();
    system_screen = system_screen_create();

    // Start with the NAS screen
    current_screen = nas_screen;
    lv_scr_load(current_screen);
    last_switch_time = esp_timer_get_time();

    // Initialise touch (GPIO21 interrupt)
    gpio_touch_init();

    // Initialise UART for receiving data from the NAS
    bsp_uart_init();
    bsp_uart_register_callback(uart_callback);

    // Main event loop
    while (1) {
        // Advance LVGL tick and process LVGL tasks
        lv_tick_inc(5);
        lv_timer_handler();

        // Poll UART (non‑blocking)
        bsp_uart_poll();

        // Handle touch interrupt with debounce
        if (touch_interrupt_pending) {
            uint64_t now = esp_timer_get_time();
            if ((now - last_touch_time) > TOUCH_DEBOUNCE_US) {
                last_touch_time = now;

                // Cycle through screens: NAS → Settings → System → NAS
                if (current_screen == nas_screen) {
                    current_screen = settings_screen;
                } else if (current_screen == settings_screen) {
                    current_screen = system_screen;
                } else {
                    current_screen = nas_screen;
                }
                lv_scr_load(current_screen);
                ESP_LOGI(TAG, "Touch switched to screen %d",
                         current_screen == nas_screen ? 1 :
                         (current_screen == settings_screen ? 2 : 3));
                // Reset the timer for automatic switching
                last_switch_time = now;
            }
            touch_interrupt_pending = false;
        }

        // Automatic screen switching every 10 seconds
        uint64_t now = esp_timer_get_time();
        if (now - last_switch_time >= switch_interval_us) {
            last_switch_time = now;

            // Cycle through screens: NAS → Settings → System → NAS
            if (current_screen == nas_screen) {
                current_screen = settings_screen;
            } else if (current_screen == settings_screen) {
                current_screen = system_screen;
            } else {
                current_screen = nas_screen;
            }
            lv_scr_load(current_screen);
            ESP_LOGI(TAG, "Timer switched to screen %d",
                     current_screen == nas_screen ? 1 :
                     (current_screen == settings_screen ? 2 : 3));
        }

        // Yield CPU for 5 ms to let other tasks run
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}