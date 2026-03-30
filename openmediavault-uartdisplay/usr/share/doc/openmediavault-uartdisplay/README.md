# UART Display Service for OpenMediaVault

A service for displaying NAS status on an embedded UART display.

## Features

- **Storage Monitoring**: Monitors mdadm RAID arrays and btrfs multi-device filesystems
- **Real-time Statistics**: Displays used/total space and array status
- **System Statistics**: CPU load, memory usage, temperature, uptime
- **Network Information**: IP addresses and interface traffic speed
- **Configurable Update Interval**: Default 10 seconds (5-60 seconds)

## Installation

### From Package

```bash
# Install the package
dpkg -i openmediavault-uartdisplay_1.0.0_all.deb

# Resolve dependencies
apt-get install -f -y

# Enable and start the service
systemctl enable uartdisplay
systemctl start uartdisplay
```

### Build from Source

```bash
# Install build dependencies
apt-get install -y debhelper python3 python3-serial dh-python

# Build the package
dpkg-buildpackage -us -uc

# Install the built package
dpkg -i ../openmediavault-uartdisplay_1.0.0_all.deb
```

## Configuration

### Configuration File

Edit `/etc/openmediavault/uartdisplay.conf`:

```ini
[uart]
uart_device = /dev/ttyUSB0
uart_baudrate = 115200
uart_timeout = 1

[display]
update_interval = 10
mount_point = /srv
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `uart_device` | UART device path | `/dev/ttyUSB0` |
| `uart_baudrate` | Serial speed | `115200` |
| `uart_timeout` | Read timeout (seconds) | `1` |
| `update_interval` | Display refresh rate (seconds) | `10` |
| `mount_point` | Directory to monitor | `/srv` |

## Message Format

The service sends the following messages to the UART display:

### 1. Array Status
```
USED=82.6,TOTAL=5589.1,STATUS=OK
```

### 2. System Statistics
```
CPU=5,MemUsed=1.0,MemTotal=15.2,Temp=45,Uptime=0.8
```

### 3. Network IP Addresses
```
eth0=192.168.1.100,eth1=192.168.1.101,fqdn=6unas.internal
```

### 4. Network Speed (only with active traffic)
```
SPEED:eth0=8900,wifi0=2500
```

## Supported Hardware

- **UART Adapters**: USB-to-serial (FTDI, CP210x, CH34x), GPIO UART
- **Displays**: Any UART-compatible display (OLED, LCD, E-Paper)
- **Baud Rates**: 9600 to 921600

## Troubleshooting

### Check Service Status

```bash
systemctl status uartdisplay
journalctl -u uartdisplay.service -f
```

### Test UART Connection

```bash
# Check if device exists
ls -la /dev/ttyUSB0

# Test serial communication
echo "TEST" > /dev/ttyUSB0
```

### Common Issues

1. **Permission denied**: Add user to `dialout` group
   ```bash
   usermod -aG dialout $USER
   ```

2. **No array detected**: Verify mount point and array type (mdadm/btrfs)

3. **Display shows garbage**: Check baud rate matches display settings

## License

GPL-3+

## Author

PixelNAS <sdyspb@pixelnas.com>

## Repository

https://github.com/sdyspb/NasDisplay
