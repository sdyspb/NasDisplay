#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
UART Display Daemon for OpenMediaVault
Displays NAS array status (mdadm/btrfs) on embedded UART display
"""

import os
import sys
import time
import serial
import subprocess
import re
import logging
import logging.handlers
import configparser
import glob
import socket

DEFAULT_CONFIG = {
    'uart_device': '/dev/ttyUSB0',
    'uart_baudrate': '115200',
    'uart_timeout': '1',
    'update_interval': '10',
    'mount_point': '/srv'
}

CONFIG_FILE = '/etc/openmediavault/uartdisplay.conf'

_prev_cpu_stats = None
_prev_net_stats = {}
_real_iface_map = {}  # Map logical names to real names


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(levelname)s: %(message)s',
        handlers=[
            logging.StreamHandler(sys.stdout),
            logging.handlers.SysLogHandler(address='/dev/log')
        ]
    )
    return logging.getLogger('uartdisplayd')


def load_config(logger):
    config = DEFAULT_CONFIG.copy()
    if os.path.exists(CONFIG_FILE):
        try:
            parser = configparser.ConfigParser()
            parser.read(CONFIG_FILE)
            if 'uart' in parser:
                for key in ['uart_device', 'uart_baudrate', 'uart_timeout']:
                    if key in parser['uart']:
                        config[key] = parser['uart'][key]
            if 'display' in parser:
                for key in ['update_interval', 'mount_point']:
                    if key in parser['display']:
                        config[key] = parser['display'][key]
            logger.info(f"Loaded configuration from {CONFIG_FILE}")
        except Exception as e:
            logger.warning(f"Failed to load config: {e}, using defaults")
    else:
        logger.info(f"Config file {CONFIG_FILE} not found, using defaults")
    return config


def get_cpu_load():
    global _prev_cpu_stats
    try:
        with open('/proc/stat', 'r') as f:
            line = f.readline()
        parts = line.split()
        if parts[0] != 'cpu':
            return 0
        user, nice, system = int(parts[1]), int(parts[2]), int(parts[3])
        idle = int(parts[4])
        iowait = int(parts[5]) if len(parts) > 5 else 0
        irq = int(parts[6]) if len(parts) > 6 else 0
        softirq = int(parts[7]) if len(parts) > 7 else 0
        total = user + nice + system + idle + iowait + irq + softirq
        busy = total - idle - iowait
        if _prev_cpu_stats is None:
            _prev_cpu_stats = {'total': total, 'busy': busy}
            return 0
        total_diff = total - _prev_cpu_stats['total']
        busy_diff = busy - _prev_cpu_stats['busy']
        _prev_cpu_stats = {'total': total, 'busy': busy}
        if total_diff > 0:
            return round(busy_diff * 100 / total_diff, 0)
        return 0
    except Exception as e:
        logging.debug(f"Failed to get CPU load: {e}")
        return 0


def get_mdadm_array_info(mount_point, logger):
    try:
        result = subprocess.run(
            ['findmnt', '-n', '-o', 'SOURCE', mount_point],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode != 0 or not result.stdout.strip():
            return None
        device = result.stdout.strip()
        if not device.startswith('/dev/md'):
            return None
        array_name = os.path.basename(device)
        status = "OK"
        try:
            with open('/proc/mdstat', 'r') as f:
                mdstat_content = f.read()
            array_pattern = rf'{array_name}.*\[([0-9]+)/([0-9]+)\]'
            match = re.search(array_pattern, mdstat_content)
            if match:
                active = int(match.group(2))
                total_arr = int(match.group(1))
                if active < total_arr:
                    status = "Degraded"
                elif re.search(rf'{array_name}.*\[U.*_\]', mdstat_content):
                    status = "Degraded"
        except Exception as e:
            logger.warning(f"Failed to read mdstat: {e}")
        result = subprocess.run(
            ['df', '-B1', mount_point],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            lines = result.stdout.strip().split('\n')
            if len(lines) >= 2:
                parts = lines[1].split()
                if len(parts) >= 3:
                    total_bytes = int(parts[1])
                    used_bytes = int(parts[2])
                    total_gb = round(total_bytes / (1024**3), 1)
                    used_gb = round(used_bytes / (1024**3), 1)
                    logger.info(f"mdadm array {device}: {used_gb}/{total_gb} GB, status={status}")
                    return {'used': used_gb, 'total': total_gb, 'status': status}
    except Exception as e:
        logger.error(f"Error getting mdadm info: {e}")
    return None


def get_btrfs_array_info(mount_point, logger):
    try:
        btrfs_mount = None
        result = subprocess.run(
            ['findmnt', '-n', '-o', 'FSTYPE', mount_point],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0 and 'btrfs' in result.stdout.strip():
            btrfs_mount = mount_point
        else:
            try:
                for entry in os.listdir(mount_point):
                    candidate = os.path.join(mount_point, entry)
                    if os.path.isdir(candidate):
                        result = subprocess.run(
                            ['findmnt', '-n', '-o', 'FSTYPE', candidate],
                            capture_output=True, text=True, timeout=5
                        )
                        if result.returncode == 0 and 'btrfs' in result.stdout.strip():
                            btrfs_mount = candidate
                            logger.info(f"Found btrfs mount at {btrfs_mount}")
                            break
            except Exception as e:
                logger.debug(f"Error scanning {mount_point}: {e}")
        if not btrfs_mount:
            return None
        result = subprocess.run(
            ['btrfs', 'filesystem', 'show', btrfs_mount],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode != 0:
            logger.warning(f"btrfs filesystem show failed: {result.stderr}")
            return None
        total_bytes = 0
        device_count = 0
        degraded = False
        raid_profile = "unknown"
        for line in result.stdout.split('\n'):
            if 'Total devices' in line:
                dev_match = re.search(r'Total devices\s+(\d+)', line)
                if dev_match:
                    device_count = int(dev_match.group(1))
            size_match = re.search(r'devid\s+\d+\s+size\s+([\d.]+)([A-Za-z]+)', line)
            if size_match:
                size_value = float(size_match.group(1))
                size_unit = size_match.group(2)
                if 'TiB' in size_unit or 'tiB' in size_unit:
                    total_bytes += int(size_value * 1024**4)
                elif 'GiB' in size_unit or 'giB' in size_unit:
                    total_bytes += int(size_value * 1024**3)
                elif 'MiB' in size_unit or 'miB' in size_unit:
                    total_bytes += int(size_value * 1024**2)
                elif 'KiB' in size_unit or 'kiB' in size_unit:
                    total_bytes += int(size_value * 1024)
            if 'missing' in line.lower():
                degraded = True
        if device_count < 2:
            logger.info(f"btrfs at {btrfs_mount} has only {device_count} device(s), not an array")
            return None
        result = subprocess.run(
            ['btrfs', 'filesystem', 'df', btrfs_mount],
            capture_output=True, text=True, timeout=5
        )
        used_bytes = 0
        if result.returncode == 0:
            for line in result.stdout.split('\n'):
                if 'Data' in line:
                    if 'RAID10' in line:
                        raid_profile = "RAID10"
                    elif 'RAID1' in line:
                        raid_profile = "RAID1"
                    elif 'RAID0' in line:
                        raid_profile = "RAID0"
                    elif 'RAID5' in line:
                        raid_profile = "RAID5"
                    elif 'RAID6' in line:
                        raid_profile = "RAID6"
                    elif 'SINGLE' in line or 'single' in line:
                        raid_profile = "SINGLE"
                    match = re.search(r'used=([\d.]+)([A-Za-z]+)', line)
                    if match:
                        size_value = float(match.group(1))
                        size_unit = match.group(2)
                        if 'TiB' in size_unit or 'tiB' in size_unit:
                            used_bytes = int(size_value * 1024**4)
                        elif 'GiB' in size_unit or 'giB' in size_unit:
                            used_bytes = int(size_value * 1024**3)
                        elif 'MiB' in size_unit or 'miB' in size_unit:
                            used_bytes = int(size_value * 1024**2)
                        elif 'KiB' in size_unit or 'kiB' in size_unit:
                            used_bytes = int(size_value * 1024)
                        break
        if raid_profile == "RAID10":
            total_bytes = total_bytes // 2
        elif raid_profile == "RAID1":
            total_bytes = total_bytes // 2
        elif raid_profile == "RAID5":
            if device_count > 1:
                total_bytes = total_bytes * (device_count - 1) // device_count
        elif raid_profile == "RAID6":
            if device_count > 2:
                total_bytes = total_bytes * (device_count - 2) // device_count
        logger.info(f"btrfs RAID: {device_count} devices, profile={raid_profile}, total={total_bytes}")
        if used_bytes == 0:
            result = subprocess.run(
                ['df', '-B1', btrfs_mount],
                capture_output=True, text=True, timeout=5
            )
            if result.returncode == 0:
                lines = result.stdout.strip().split('\n')
                if len(lines) >= 2:
                    parts = lines[1].split()
                    if len(parts) >= 3:
                        used_bytes = int(parts[2])
        status = "OK" if not degraded else "Degraded"
        total_gb = round(total_bytes / (1024**3), 1)
        used_gb = round(used_bytes / (1024**3), 1)
        logger.info(f"btrfs array at {btrfs_mount}: {used_gb}/{total_gb} GB, status={status}")
        return {'used': used_gb, 'total': total_gb, 'status': status}
    except Exception as e:
        logger.error(f"Error getting btrfs info: {e}")
    return None


def get_array_info(mount_point, logger):
    info = get_mdadm_array_info(mount_point, logger)
    if info:
        info['type'] = 'mdadm'
        return info
    info = get_btrfs_array_info(mount_point, logger)
    if info:
        info['type'] = 'btrfs'
        return info
    return None


def get_cpu_temperature():
    try:
        for path in glob.glob('/sys/class/hwmon/hwmon*/name'):
            try:
                with open(path, 'r') as f:
                    sensor_name = f.read().strip()
                if sensor_name == 'coretemp':
                    hwmon_dir = os.path.dirname(path)
                    for label_path in glob.glob(os.path.join(hwmon_dir, 'temp*_label')):
                        try:
                            with open(label_path, 'r') as f:
                                label = f.read().strip()
                            if 'Package id 0' in label or 'Core' in label:
                                temp_path = label_path.replace('_label', '_input')
                                if os.path.exists(temp_path):
                                    with open(temp_path, 'r') as f:
                                        temp_val = int(f.read().strip())
                                    if temp_val > 1000:
                                        temp_val = temp_val // 1000
                                    if 0 < temp_val < 150:
                                        return temp_val
                        except:
                            continue
                    for temp_path in glob.glob(os.path.join(hwmon_dir, 'temp*_input')):
                        try:
                            with open(temp_path, 'r') as f:
                                temp_val = int(f.read().strip())
                            if temp_val > 1000:
                                temp_val = temp_val // 1000
                            if 0 < temp_val < 150:
                                return temp_val
                        except:
                            continue
            except:
                continue
        for path in glob.glob('/sys/class/thermal/thermal_zone*/temp'):
            try:
                with open(path, 'r') as f:
                    temp_val = int(f.read().strip())
                if temp_val > 1000:
                    temp_val = temp_val // 1000
                if 0 < temp_val < 150:
                    return temp_val
            except:
                continue
        return 0
    except Exception as e:
        logging.debug(f"Failed to get CPU temperature: {e}")
        return 0


def is_virtual_interface(iface):
    """Check if interface is virtual (podman, docker, veth, bridge, etc.)."""
    virtual_prefixes = ['veth', 'docker', 'podman', 'br-', 'virbr', 'lxcbr']
    for prefix in virtual_prefixes:
        if iface.startswith(prefix):
            return True
    return False


def is_wireless_interface(iface):
    """Check if interface is wireless (wifi)."""
    if os.path.exists(f'/sys/class/net/{iface}/phy80211'):
        return True
    try:
        result = subprocess.run(
            ['iw', 'dev', iface, 'info'],
            capture_output=True, text=True, timeout=2
        )
        if result.returncode == 0 and 'Interface' in result.stdout:
            return True
    except:
        pass
    if iface.startswith('wl') or iface.startswith('wlan') or iface.startswith('wlp'):
        return True
    return False


def get_network_interfaces():
    """Get network interfaces with mapped names (eth0, eth1, wifi0).
    Returns max 3 interfaces: eth0, eth1 for wired, wifi0 for wireless.
    Excludes virtual interfaces (podman, docker, veth, etc.).
    """
    global _real_iface_map
    interfaces = []
    wired = []
    wireless = []
    
    try:
        result = subprocess.run(
            ['ip', '-o', 'addr', 'show'],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            for line in result.stdout.split('\n'):
                match = re.search(r'\d+:\s+(\S+)\s+.*inet\s+(\d+\.\d+\.\d+\.\d+)', line)
                if match:
                    iface = match.group(1)
                    ip = match.group(2)
                    if not ip.startswith('127.') and iface != 'lo':
                        # Skip virtual interfaces
                        if is_virtual_interface(iface):
                            logging.debug(f"Skipping virtual interface: {iface}")
                            continue
                        if is_wireless_interface(iface):
                            wireless.append((iface, ip))
                        else:
                            wired.append((iface, ip))
    except Exception as e:
        logging.debug(f"Failed to get network interfaces: {e}")
    
    # Map to eth0, eth1, wifi0
    for i, (real_iface, ip) in enumerate(wired[:2]):
        logical_name = f'eth{i}'
        interfaces.append((logical_name, ip))
        _real_iface_map[logical_name] = real_iface
        logging.debug(f"Mapped {real_iface} -> {logical_name} ({ip})")
    
    if wireless:
        real_iface, ip = wireless[0]
        interfaces.append(('wifi0', ip))
        _real_iface_map['wifi0'] = real_iface
        logging.debug(f"Mapped {real_iface} -> wifi0 ({ip})")
    
    return interfaces[:3]


def get_fqdn():
    try:
        return socket.getfqdn()
    except:
        try:
            with open('/etc/hostname', 'r') as f:
                return f.read().strip()
        except:
            return "unknown"


def get_interface_speed(logical_name):
    """Get current interface speed in Mb/s based on actual traffic."""
    global _prev_net_stats, _real_iface_map
    
    # Get real interface name from map
    real_iface = _real_iface_map.get(logical_name, logical_name)
    
    try:
        with open('/proc/net/dev', 'r') as f:
            for line in f:
                if real_iface + ':' in line:
                    parts = line.split(':')[1].split()
                    rx_bytes = int(parts[0])
                    tx_bytes = int(parts[8])
                    total_bytes = rx_bytes + tx_bytes
                    
                    if real_iface in _prev_net_stats:
                        prev_bytes, prev_time = _prev_net_stats[real_iface]
                        time_diff = time.time() - prev_time
                        
                        if time_diff > 0 and time_diff <= 60:  # Max 60 seconds between samples
                            bytes_diff = total_bytes - prev_bytes
                            if bytes_diff >= 0:  # Handle counter reset
                                # Convert to Mb/s (bytes * 8 / 1000000 / seconds)
                                speed_mbps = round((bytes_diff * 8) / 1000000 / time_diff)
                                _prev_net_stats[real_iface] = (total_bytes, time.time())
                                return speed_mbps
                    
                    _prev_net_stats[real_iface] = (total_bytes, time.time())
                    return 0  # First sample, no speed data yet
    except Exception as e:
        logging.debug(f"Failed to get speed for {real_iface}: {e}")
    return 0


def get_system_stats(logger):
    try:
        stats = {}
        stats['cpu'] = get_cpu_load()
        try:
            meminfo = {}
            with open('/proc/meminfo', 'r') as f:
                for line in f:
                    parts = line.split()
                    if len(parts) >= 2:
                        key = parts[0].rstrip(':')
                        meminfo[key] = int(parts[1])
            total_kb = meminfo.get('MemTotal', 0)
            available_kb = meminfo.get('MemAvailable', 0)
            used_kb = total_kb - available_kb
            stats['mem_used'] = round(used_kb / (1024**2), 1)
            stats['mem_total'] = round(total_kb / (1024**2), 1)
        except Exception as e:
            logger.debug(f"Failed to get memory stats: {e}")
            stats['mem_used'] = 0
            stats['mem_total'] = 0
        stats['temp'] = get_cpu_temperature()
        try:
            with open('/proc/uptime', 'r') as f:
                uptime_seconds = float(f.read().split()[0])
            stats['uptime'] = round(uptime_seconds / 86400, 1)
        except Exception as e:
            logger.debug(f"Failed to get uptime: {e}")
            stats['uptime'] = 0
        return stats
    except Exception as e:
        logger.error(f"Error getting system stats: {e}")
    return None


def format_message(info):
    if not info:
        return "USED=0,TOTAL=0,STATUS=Unknown"
    return f"USED={info['used']},TOTAL={info['total']},STATUS={info['status']}"


def format_system_message(stats):
    if not stats:
        return "CPU=0,MemUsed=0,MemTotal=0,Temp=0,Uptime=0"
    return f"CPU={stats['cpu']},MemUsed={stats['mem_used']},MemTotal={stats['mem_total']},Temp={stats['temp']},Uptime={stats['uptime']}"


def format_network_message(logger):
    interfaces = get_network_interfaces()
    fqdn = get_fqdn()
    if not interfaces:
        return None
    parts = [f"{iface}={ip}" for iface, ip in interfaces]
    parts.append(f"fqdn={fqdn}")
    return ','.join(parts)


def format_speed_message(logger):
    """Format network speed message using logical names (eth0, wifi0)."""
    interfaces = get_network_interfaces()
    if not interfaces:
        return None
    speeds = []
    for logical_name, ip in interfaces:
        speed = get_interface_speed(logical_name)
        # Only include interfaces with actual traffic (speed > 0)
        if speed > 0:
            speeds.append(f"{logical_name}={speed}")
    if not speeds:
        return None
    return "SPEED:" + ','.join(speeds)


def send_uart_message(serial_conn, message, logger):
    try:
        serial_conn.write(f"{message}\n".encode('ascii'))
        serial_conn.flush()
        return True
    except Exception as e:
        logger.error(f"Failed to send UART message: {e}")
        return False


def main():
    logger = setup_logging()
    logger.info("UART Display Daemon starting...")
    config = load_config(logger)
    uart_device = config['uart_device']
    uart_baudrate = int(config['uart_baudrate'])
    uart_timeout = float(config['uart_timeout'])
    update_interval = int(config['update_interval'])
    mount_point = config['mount_point']
    logger.info(f"UART Device: {uart_device}, Baudrate: {uart_baudrate}")
    logger.info(f"Mount Point: {mount_point}, Update Interval: {update_interval}s")
    serial_conn = None
    # Wait a bit for first network sample
    time.sleep(1)
    while True:
        try:
            if serial_conn is None or not serial_conn.is_open:
                try:
                    serial_conn = serial.Serial(
                        port=uart_device,
                        baudrate=uart_baudrate,
                        timeout=uart_timeout
                    )
                    logger.info(f"Serial port {uart_device} opened successfully")
                except serial.SerialException as e:
                    logger.warning(f"Cannot open serial port {uart_device}: {e}")
                    serial_conn = None
                    time.sleep(update_interval)
                    continue
            info = get_array_info(mount_point, logger)
            if info:
                message = format_message(info)
                logger.info(f"Array Status: {message}")
                send_uart_message(serial_conn, message, logger)
            else:
                logger.warning(f"No mdadm/btrfs array found at {mount_point}")
                send_uart_message(serial_conn, "USED=0,TOTAL=0,STATUS=NoArray", logger)
            sys_stats = get_system_stats(logger)
            sys_message = format_system_message(sys_stats)
            logger.info(f"System Stats: {sys_message}")
            send_uart_message(serial_conn, sys_message, logger)
            net_message = format_network_message(logger)
            if net_message:
                logger.info(f"Network IPs: {net_message}")
                send_uart_message(serial_conn, net_message, logger)
            speed_message = format_speed_message(logger)
            if speed_message:
                logger.info(f"Network Speed: {speed_message}")
                send_uart_message(serial_conn, speed_message, logger)
            else:
                logger.debug("No network speed data (no traffic)")
        except KeyboardInterrupt:
            logger.info("Received interrupt, shutting down...")
            break
        except Exception as e:
            logger.error(f"Error in main loop: {e}")
        time.sleep(update_interval)
    if serial_conn and serial_conn.is_open:
        serial_conn.close()
    logger.info("UART Display Daemon stopped")


if __name__ == '__main__':
    main()
