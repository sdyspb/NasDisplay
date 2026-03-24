idf_component_register(SRCS
    main.c
    bsp_display.c
    bsp_spi.c
    esp_lvgl_port.c
    esp_lcd_jd9853.c
    bsp_uart.c
    uart/uart_parser.c
    ui/nas_screen.c
    ui/settings_screen.c
    ui/system_screen.c
    ui/ui_common.c
    INCLUDE_DIRS "." "uart" "ui"
    REQUIRES driver esp_adc nvs_flash json
    PRIV_REQUIRES esp_lcd
)