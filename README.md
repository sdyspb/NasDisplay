# User Manual: ESP32-C6 Display for NAS Monitoring
## 1. Purpose
The device based on the ESP32-C6 with a 1.47" colour display (172×320, landscape orientation) is designed to display key parameters of a NAS (Network Attached Storage) and server system information. The interface is built with the LVGL graphics library and provides three screens:

**NAS Storage** – used/free space (GB), status, usage bar.
**Network Interfaces** – IP addresses of network interfaces (eth0, eth1, wifi0), data transfer speeds (arc meters for eth0 and wifi0), and the device name (FQDN).
**System Status** – CPU load (%), RAM usage, CPU temperature, uptime in days, and a three‑segment meter.

Data is received via UART (RS‑232) from the NAS server (e.g., an OMV plugin). Screen switching occurs automatically every 10 seconds or by touch (planned for future implementation).

## 2. Hardware Connection
### 2.1. Power
- Supply voltage: 5 V via the USB‑C connector of the ESP32-C6-DevKitM-1 board.
- Current consumption: up to 300 mA (including display backlight).

### 2.2. UART (Communication with NAS)
Signal	ESP32 Pin	Description
TX	GPIO16	Data from ESP32 to NAS (not used)
RX	GPIO17	Data from NAS to ESP32 (connect to NAS TX)
GND	GND	Common ground
Baud rate: 115200, 8 data bits, 1 stop bit, no parity.

### 2.3. Display
The display is connected via SPI (pins defined in bsp_config.h). All necessary connections are provided on the board.

### 2.4. Touch (future support)
The board includes a GPIO21 pin for touch interrupt, but the touch controller driver is not yet activated.

## 3. Firmware Flashing and Setup
### 3.1. Requirements
- PlatformIO (VS Code) or ESP‑IDF.
- Git installed (for cloning the repository).

### 3.2. Building the Project
- Clone the repository:

```bash
git clone https://github.com/your-repo/nas-display.git
cd nas-display
```
- Open the folder in VS Code with the PlatformIO extension installed.
- Select the build environment: esp32-c6-devkitm-1.
- Build the project: press Build (Ctrl+Alt+B) or run:

```bash
pio run
```
**Flash the device:**
- Connect the board via USB‑C to your computer.
- Press Upload (Ctrl+Alt+U).
- If necessary, press the BOOT button on the board during connection.

### 3.3. Testing
After flashing, the backlight turns on and the initial NAS Storage screen appears. The screen switches every 10 seconds. To test UART, send a test string (e.g., via terminal):

```NAS command
USED=120.5,TOTAL=500,STATUS=OK
```
Data should update on the first screen.

## 4. Future Plans
- Touch screen support – add touch input for screen switching and menu interaction.
- Wi‑Fi – obtain data wirelessly (HTTP, MQTT) to reduce reliance on wired UART.
- Settings screen – configure brightness, switching interval, Wi‑Fi parameters.

## 5. USB vs. UART: Limitations
- USB (used for debugging)
- Limited cable length (typically 3–5 metres).
- Requires drivers on the host machine.
- Used for programming, not for continuous data exchange.
- UART (used for NAS communication)
- Allows long‑distance communication (up to tens of metres) with line drivers (RS‑232/RS‑485).
- Simple setup on the NAS side.
- Stable in industrial environments.
In the current implementation, USB is used only for firmware upload and monitoring. Permanent data exchange with the NAS is done via dedicated UART.

## 6. Project Tree (PlatformIO + VS Code)
```Project Structure
nas-display/
├── .pio/                     (build temporary files)
├── include/                  (header files – moved to src)
├── lib/                      (not used; libraries via lib_deps)
├── src/
│   ├── main.c
│   ├── bsp_config.h
│   ├── bsp_display.c/.h
│   ├── bsp_spi.c/.h
│   ├── bsp_uart.c/.h
│   ├── esp_lvgl_port.c/.h
│   ├── esp_lcd_jd9853.c/.h
│   ├── lv_conf.h
│   ├── uart/
│   │   ├── uart_parser.h
│   │   └── uart_parser.c
│   └── ui/
│       ├── ui_common.h
│       ├── ui_common.c
│       ├── nas_screen.h
│       ├── nas_screen.c
│       ├── settings_screen.h
│       ├── settings_screen.c
│       ├── system_screen.h
│       └── system_screen.c
├── platformio.ini
├── CMakeLists.txt
└── README.md
```
Key files:

platformio.ini – build environment settings, dependencies (LVGL).
src/main.c – main logic, initialisation, UART handling, screen switching.
src/ui/ – implementation of the three screens.
src/uart/uart_parser.c – parsing of incoming strings.

## 7. Important Runtime Settings
### 7.1. Watchdog Timer (WDT)
To avoid unexpected resets, the following options should be set in the sdkconfig (or via idf.py menuconfig):

```text
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n
```
This disables checking the IDLE task and sets a generous timeout (10 seconds), which is essential for LVGL’s lengthy rendering operations.

### 7.2. Display Buffering
In bsp_config.h, the draw buffer is configured to cover the whole screen (EXAMPLE_LCD_DRAW_BUFF_HEIGHT = 172) and double buffering is enabled (EXAMPLE_LCD_DRAW_BUFF_DOUBLE = 1). This ensures that the entire display is updated in one go, preventing tearing and “snow” artefacts. Double buffering consumes additional RAM (~220 KB), but it is necessary for smooth rendering on the 172×320 screen.

## 8. Configuration Parameters
### 8.1. src/bsp_config.h
``` Parameter	/ Description	/ Default
EXAMPLE_LCD_PIXEL_CLOCK_HZ	SPI clock frequency for the display	20 MHz (20 000 000)
EXAMPLE_DISPLAY_ROTATION	Orientation of the screen (0, 90, 180, 270)	90 (landscape)
EXAMPLE_LCD_DRAW_BUFF_HEIGHT	Height of the LVGL draw buffer (pixels). Set to full screen height (172) to use double‑buffering.	172
EXAMPLE_LCD_DRAW_BUFF_DOUBLE	Enable double buffering (1 = yes, 0 = no)	1
UART_NAS_BAUD	UART baud rate for NAS communication	115200
TOUCH_DEBOUNCE_US	Debounce time for touch interrupt (microseconds)	150000 (150 ms)
NAS_USAGE_BAR_WIDTH	Width of the storage usage bar on the NAS screen (pixels)	320
METER_SIZE	Size (width and height) of the circular speed meter on the Settings screen (pixels)	130
METER_ARC1_COLOR	Colour of the first arc (eth0)	lv_color_hex(0xFF0000) (red)
METER_ARC2_COLOR	Colour of the second arc (wifi0)	lv_color_hex(0x0088FF) (blue)
```
### 8.2. src/lv_conf.h
``` Parameter	/ Description	/ Default
LV_COLOR_16_SWAP	Byte order for 16‑bit colours. Must be set to 1 for the JD9853 display.	1
LV_MEM_SIZE	Size of the static memory pool used by LVGL (if LV_MEM_CUSTOM = 0).	48 × 1024 (48 KB)
LV_FONT_MONTSERRAT_18	Enable Montserrat 18 font for content.	1 (enabled)
LV_USE_BAR	Enable progress bar widget.	1
LV_USE_METER	Enable meter (gauge) widget.	1
```
### 8.3. src/ui/ui_common.h
``` Macro	/ Description	/ Default
CONTENT_FONT	Font used for all text labels on the screens (except headers).	&lv_font_montserrat_18
CONTENT_ROW_PAD	Vertical spacing between rows of text (pixels).	8
```
### 8.4. Adjusting Screen Switching Interval
The automatic screen switching interval is defined in src/main.c as:

```c
static const uint64_t switch_interval_us = 10 * 1000 * 1000; // 10 seconds
```
Change this value (in microseconds) to alter the delay.

### 8.5. Enabling Floating‑Point Support for snprintf
If you see f instead of decimal numbers, the C library does not support floating‑point formatting. To enable it, run idf.py menuconfig (or pio run -t menuconfig) and navigate to:

``` text
Component config → C Library → Enable floating point printf
```
Set it to yes, then rebuild.

## 9. Full Listing of src/bsp_config.h (with Comments)
```c
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
```
## 10. Message Examples & Host Interaction Recommendations
### 10.1. Message Formats
The device expects plain text messages terminated by newline (\n). Each message updates a specific part of the display.

``` Screen	Format	Example
NAS Storage	USED=... ,TOTAL=... ,STATUS=...	USED=120.5,TOTAL=500,STATUS=OK
Network Interfaces (IP)	eth0=... ,eth1=... ,wifi0=... ,fqdn=...	eth0=192.168.1.100,eth1=192.168.1.101,wifi0=192.168.1.102,fqdn=Nas6U.internal
Network Interfaces (Speed)	SPEED:eth0=... ,wifi0=...	SPEED:eth0=7500,wifi0=2500
System Status	CPU=... ,MemUsed=... ,MemTotal=... ,Temp=... ,Uptime=...	CPU=45,MemUsed=2.5,MemTotal=8,Temp=65,Uptime=2.3
```
Notes:

All values are separated by commas without spaces.
The fqdn field is displayed in yellow; IP addresses appear in green.
Uptime is given in weeks; the display converts it to days automatically.
Speeds are in the range 0–10000 (scaled to 0–100% on the meter).
The device ignores any extra spaces or unknown fields.

### 10.2. Host Interaction Recommendations
To feed data to the device, set up a script on the NAS that periodically sends the formatted strings over UART. For example, using a Python script on the NAS (e.g., via a cron job or systemd timer):

```python
#!/usr/bin/env python3
import serial
import time

# Adjust the serial port as needed (e.g., /dev/ttyUSB0)
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

def send_nas_data():
    ser.write(b'USED=120.5,TOTAL=500,STATUS=OK\n')

def send_network_data():
    ser.write(b'eth0=192.168.1.100,eth1=192.168.1.101,wifi0=192.168.1.102,fqdn=Nas6U.internal\n')

def send_speed_data():
    ser.write(b'SPEED:eth0=7500,wifi0=2500\n')

def send_system_data():
    ser.write(b'CPU=45,MemUsed=2.5,MemTotal=8,Temp=65,Uptime=2.3\n')

if __name__ == '__main__':
    while True:
        send_nas_data()
        send_network_data()
        send_speed_data()
        send_system_data()
        time.sleep(5)   # send every 5 seconds
```
- Ensure the serial device (e.g., /dev/ttyUSB0) has proper permissions.
- Use a reliable USB‑to‑UART adapter connected to the ESP32’s RX (GPIO17) and GND.
- If using a different baud rate, adjust both the script and UART_NAS_BAUD in bsp_config.h.

**Best practices:**

- Send all four message types regularly, even if values haven’t changed, to keep the display current.
- Keep the script simple and robust – use error handling for serial timeouts.
- If the device becomes unresponsive, try reducing the baud rate to 9600 (adjust both ends) to improve stability over long cables.

## 11. License and Used Open Source Projects
### 11.1. License
This project is licensed under the GNU General Public License v2.0 (GPL‑2.0)

### 11.2. Used Open Source Libraries and Components
Component	License	Link
- LVGL (graphics library)	MIT	https://github.com/lvgl/lvgl
- ESP‑IDF (Espressif framework)	Apache-2.0	https://github.com/espressif/esp-idf
- esp_lvgl_port (LVGL port for ESP)	Apache-2.0	https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port
- cJSON (JSON parser)	MIT	https://github.com/DaveGamble/cJSON
Note: All used components and libraries are open‑source software; their licenses are compatible with GPL‑2.0. The source code of this project, including all modifications, is available in the repository.

## 12. Hardware Kit Reference
The project is designed for the Waveshare ESP32-C6 Touch LCD 1.47 kit.
More details can be found at:
www.waveshare.com/wiki/ESP32-C6-Touch-LCD-1.47

## 13. Conclusion
This device visualises key NAS parameters on a compact colour display, eliminating the need to constantly open a web interface. The project is easily extensible – touch, Wi‑Fi, additional sensors, and more screens can be added.

For questions: please open an issue in the repository or contact the maintainer.

