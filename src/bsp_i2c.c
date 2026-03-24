#include "bsp_i2c.h"
#include "bsp_config.h"

i2c_master_bus_handle_t bsp_i2c_init(void)
{
    i2c_master_bus_handle_t bus;
    i2c_master_bus_config_t i2c_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = (i2c_port_num_t)I2C_PORT_NUM,
        .scl_io_num = EXAMPLE_PIN_I2C_SCL,
        .sda_io_num = EXAMPLE_PIN_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_cfg, &bus));
    return bus;
}