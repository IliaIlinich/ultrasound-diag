# Ultrasound-diag

A collection of tools for working with an ultrasound distance sensor over Modbus-RTU.

## What is included

| File / binary | Purpose |
|---|---|
| `ultrasound-diag` | Command-line diagnostic tool. Reads the distance, reads any Modbus register, and writes configuration registers. |
| `mdetector` | Background service that polls the sensor, detects motion, and writes flag/telemetry files. Replaces `motion_detector.py`. |
| `modbus_sensor.hpp` | Shared RAII wrapper around `libmodbus`. |
| `systemd/mdetector@.service` | Systemd template unit for running `mdetector` on any serial port. |

## Requirements

- Linux (tested on Raspberry Pi OS)
- `libmodbus`
- `cmake`, `pkg-config`, `g++`

Install the dependencies:

```bash
sudo apt update
sudo apt install libmodbus-dev pkg-config cmake g++
```

## Build

```bash
mkdir build
cd build
cmake ..
make
```

This produces two binaries:

- `build/ultrasound-diag`
- `build/mdetector`

## Running the diagnostic tool

> **Note:** Before using the serial port, stop any service that already owns it:

```bash
sudo systemctl stop mdetector-service
```

### Read the current distance

```bash
./ultrasound-diag --port /dev/serial0 -d
```

### Read any register

```bash
./ultrasound-diag --port /dev/serial0 -r 0x0200
```

### Write any register

```bash
./ultrasound-diag --port /dev/serial0 -w 0x0200:2
```

### Change serial settings

```bash
./ultrasound-diag --port /dev/ttyUSB0 --baud 9600 --slave 2 -d
```

### Common options

| Option | Description | Default |
|---|---|---|
| `-p, --port` | Serial device | `/dev/serial0` |
| `-b, --baud` | Baud rate | `115200` |
| `-s, --slave` | Modbus slave ID | `1` |
| `-t, --timeout` | Response timeout in milliseconds | `500` |
| `-d, --distance` | Read distance register `0x0101` | |
| `-r, --read` | Read a specific register | |
| `-w, --write` | Write a value to a register (`addr:value`) | |

## Running the motion detector service

### Manual test

```bash
./mdetector --port /dev/serial0 \
            --flag /tmp/motion.flg \
            --unflag /tmp/no-motion.flg \
            --dump /dev/shm/mdetector.txt
```

While it runs, it will:

- Poll the sensor every `--interval` ms.
- Maintain a moving average over `--average` seconds.
- Create `--flag` when motion is detected.
- Create `--unflag` when motion stops.
- Write telemetry to `--dump`.

### Service options

| Option | Description | Default |
|---|---|---|
| `--port` | Serial device | `/dev/serial0` |
| `--baud` | Baud rate | `115200` |
| `--slave` | Modbus slave ID | `1` |
| `--interval` | Poll interval in ms | `100` |
| `--average` | Averaging window in seconds | `5` |
| `--jitter` | Distance deviation that counts as motion in mm | `50` |
| `--distance` | Distance threshold in mm | `350` |
| `--flag` | Motion flag file | `/tmp/motion.flg` |
| `--unflag` | No-motion flag file | `/tmp/no-motion.flg` |
| `--dump` | Telemetry dump file | `/dev/shm/mdetector.txt` |

## Installing as a systemd service

Copy the binary and the service template:

```bash
sudo cp build/mdetector /usr/local/bin/
sudo cp systemd/mdetector@.service /etc/systemd/system/
sudo systemctl daemon-reload
```

Start an instance on a specific port, for example `/dev/serial0`:

```bash
sudo systemctl enable --now mdetector@serial0.service
```

Start another instance on a second port, for example `/dev/ttyUSB0`:

```bash
sudo systemctl enable --now mdetector@ttyUSB0.service
```

Check status:

```bash
sudo systemctl status mdetector@serial0.service
```

View logs:

```bash
sudo journalctl -u mdetector@serial0.service -f
```

## Typical sensor registers

The registers below match the A22 sensor family. Always verify against your sensor manual.

| Address | Meaning | Access |
|---|---|---|
| `0x0100` | Processed distance value | Read |
| `0x0101` | Real-time distance in mm | Read |
| `0x0200` | Modbus slave address | Read/Write |
| `0x0201` | Baud-rate code | Read/Write |
| `0x0202` | Parity setting | Read/Write |
| `0x0209` | Output value type (`0` = mm, `1` = µs) | Read/Write |

After writing a configuration register, power-cycle or reconnect the sensor so the new settings take effect.

## Project layout

```text
.
├── CMakeLists.txt
├── modbus_sensor.hpp
├── diag.cpp
├── mdetector.cpp
├── systemd/
│   ├── mdetector@.service
│   └── README.md
└── README.md
```

## Troubleshooting

### `Permission denied` on `/dev/serial0`

Add your user to the `dialout` group and log out:

```bash
sudo usermod -a -G dialout $USER
```

### Port is busy

Stop the existing service first:

```bash
sudo systemctl stop mdetector-service
```

### No reply from sensor

- Check wiring and power.
- Confirm baud rate and slave ID with `ultrasound-diag`.
- Some sensors stream auto-frames; the wrapper calls `modbus_flush()` before each read to clear stale bytes.
