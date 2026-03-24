#include "bsp_spi.h"
#include "bsp_config.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "bsp_spi";

void bsp_spi_init(void)
{
    ESP_LOGI(TAG, "Init SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_SCLK,
        .mosi_io_num = EXAMPLE_PIN_MOSI,
        .miso_io_num = EXAMPLE_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
}