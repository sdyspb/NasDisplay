/**
 * @file bsp_config.h
 * @brief Board support package configuration – pin assignments and display settings.
 *
 * This file contains all hardware-specific definitions for the ESP32-C6 board
 * with the Waveshare 1.47" JD9853 display. Modify these values to match your
 * hardware setup.
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include "driver/gpio.h"
#include "driver/uart.h"

//=============================================================================
// SPI (Display)
//=============================================================================
#define EXAMPLE_SPI_HOST       SPI2_HOST                // SPI controller to use
#define EXAMPLE_PIN_MISO       GPIO_NUM_3               // Master In Slave Out (not used, but defined)
#define EXAMPLE_PIN_MOSI       GPIO_NUM_2               // Master Out Slave In
#define EXAMPLE_PIN_SCLK       GPIO_NUM_1               // Serial Clock

//=============================================================================
// Display (JD9853)
//=============================================================================
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)   // SPI clock frequency (20 MHz)
#define EXAMPLE_PIN_LCD_CS    GPIO_NUM_14               // Chip Select
#define EXAMPLE_PIN_LCD_DC    GPIO_NUM_15               // Data/Command
#define EXAMPLE_PIN_LCD_RST   GPIO_NUM_22               // Reset
#define EXAMPLE_PIN_LCD_BL    GPIO_NUM_23               // Backlight

// Backlight PWM (LEDC) settings
#define LCD_BL_LEDC_TIMER      LEDC_TIMER_0
#define LCD_BL_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LCD_BL_LEDC_CHANNEL    LEDC_CHANNEL_0
#define LCD_BL_LEDC_DUTY_RES   LEDC_TIMER_10_BIT        // 10‑bit resolution → 1024 steps
#define LCD_BL_LEDC_DUTY       (1024)                   // Max duty
#define LCD_BL_LEDC_FREQUENCY  (5000)                   // 5 kHz

//=============================================================================
// I2C (reserved for touch / sensors)
//=============================================================================
#define EXAMPLE_PIN_I2C_SDA    GPIO_NUM_18
#define EXAMPLE_PIN_I2C_SCL    GPIO_NUM_19
#define I2C_PORT_NUM           0

//=============================================================================
// Touch (GPIO interrupt)
//=============================================================================
#define EXAMPLE_PIN_TP_INT     GPIO_NUM_21
#define EXAMPLE_PIN_TP_RST     GPIO_NUM_20

//=============================================================================
// SD Card (not used, but defined for compatibility)
//=============================================================================
#define EXAMPLE_PIN_SD_CS      GPIO_NUM_4

//=============================================================================
// Battery voltage measurement (not used)
//=============================================================================
#define EXAMPLE_ADC_UNIT       ADC_UNIT_1
#define EXAMPLE_BATTERY_ADC_CHANNEL ADC_CHANNEL_0
#define EXAMPLE_ADC_ATTEN      ADC_ATTEN_DB_12

//=============================================================================
// Wi‑Fi (not used)
//=============================================================================
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

//=============================================================================
// UART (communication with NAS)
//=============================================================================
#define UART_NAS_PORT          UART_NUM_1
#define UART_NAS_TX_PIN        GPIO_NUM_16
#define UART_NAS_RX_PIN        GPIO_NUM_17
#define UART_NAS_BAUD          115200
#define UART_NAS_BUF_SIZE      256

//=============================================================================
// LVGL display dimensions (after rotation)
//=============================================================================
#define EXAMPLE_DISPLAY_ROTATION 90
#if EXAMPLE_DISPLAY_ROTATION == 90 || EXAMPLE_DISPLAY_ROTATION == 270
#define EXAMPLE_LCD_H_RES (320)   // width in landscape
#define EXAMPLE_LCD_V_RES (172)   // height in landscape
#else
#define EXAMPLE_LCD_H_RES (172)
#define EXAMPLE_LCD_V_RES (320)
#endif

//=============================================================================
// LVGL draw buffer
//=============================================================================
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (172)   // full screen height – use whole‑screen buffering
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)     // double buffering (improves smoothness)

//=============================================================================
// UI elements dimensions
//=============================================================================
#define NAS_USAGE_BAR_WIDTH (320)            // storage bar width (full screen width)
#define METER_SIZE (130)                    // size of circular speed meter (pixels)
#define METER_ARC1_COLOR lv_color_hex(0xFF0000) // red arc (eth0)
#define METER_ARC2_COLOR lv_color_hex(0x0088FF) // blue arc (wifi0)

//=============================================================================
// Touch debounce time (microseconds)
//=============================================================================
#define TOUCH_DEBOUNCE_US (150000)           // 150 ms – prevents multiple triggers

#endif // BSP_CONFIG_H