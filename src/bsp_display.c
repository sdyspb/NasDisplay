#include "bsp_display.h"
#include "bsp_config.h"
#include "bsp_spi.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_jd9853.h"
#include "esp_log.h"

static const char *TAG = "bsp_display";
static uint8_t g_brightness = 0;

void bsp_display_init(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel, size_t max_transfer_sz)
{
    ESP_LOGI(TAG, "Init display");
    esp_lcd_panel_io_spi_config_t io_config = JD9853_PANEL_IO_SPI_CONFIG(EXAMPLE_PIN_LCD_CS, EXAMPLE_PIN_LCD_DC, NULL, NULL);
    io_config.pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_SPI_HOST, &io_config, io));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9853(*io, &panel_config, panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(*panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel, true));
}

void bsp_display_brightness_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LCD_BL_LEDC_MODE,
        .timer_num = LCD_BL_LEDC_TIMER,
        .duty_resolution = LCD_BL_LEDC_DUTY_RES,
        .freq_hz = LCD_BL_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LCD_BL_LEDC_MODE,
        .channel = LCD_BL_LEDC_CHANNEL,
        .timer_sel = LCD_BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = EXAMPLE_PIN_LCD_BL,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void bsp_display_set_brightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    g_brightness = brightness;
    uint32_t duty = (brightness * (LCD_BL_LEDC_DUTY - 1)) / 100;
    ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty);
    ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL);
}

uint8_t bsp_display_get_brightness(void)
{
    return g_brightness;
}