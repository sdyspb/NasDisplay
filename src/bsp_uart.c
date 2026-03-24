#include "bsp_uart.h"
#include "bsp_config.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "bsp_uart";
static uart_nas_callback_t s_callback = NULL;
static char s_buffer[UART_NAS_BUF_SIZE];
static int s_idx = 0;

void bsp_uart_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate = UART_NAS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NAS_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_driver_install(UART_NAS_PORT, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(UART_NAS_PORT, UART_NAS_TX_PIN, UART_NAS_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "UART initialized (polling mode)");
}

void bsp_uart_register_callback(uart_nas_callback_t callback)
{
    s_callback = callback;
}

void bsp_uart_poll(void)
{
    uint8_t c;
    int len = uart_read_bytes(UART_NAS_PORT, &c, 1, 0); // not blocking
    if (len > 0) {
        if (c == '\n' || c == '\r') {
            if (s_idx > 0) {
                s_buffer[s_idx] = '\0';
                if (s_callback) s_callback(s_buffer);
                s_idx = 0;
            }
        } else if (s_idx < sizeof(s_buffer) - 1) {
            s_buffer[s_idx++] = c;
        }
    }
}