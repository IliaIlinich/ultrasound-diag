# Ultrasound Diagnostic Tool

A command-line utility for reading and configuring an ultrasound distance sensor over Modbus-RTU.

## Features

- Read live distance from the sensor
- Read any Modbus holding register
- Write values to configuration registers
- Configurable serial port, baud rate, slave ID, and timeout

## Requirements

- Linux (tested on Raspberry Pi OS)
- `libmodbus`
- `cmake`, `pkg-config`, `g++`

Install dependencies:

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

The binary `ultrasound-diag` will be created in `build/`.

## Usage

> Stop any service using the serial port first

### Read distance

```bash
./ultrasound-diag --port /dev/serial0 -d
```

### Read a register

```bash
./ultrasound-diag --port /dev/serial0 -r 0x0200
```

### Write a register

```bash
./ultrasound-diag --port /dev/serial0 -w 0x0200:2
```

### Change connection settings

```bash
./ultrasound-diag --port /dev/ttyUSB0 --baud 9600 --slave 2 -d
```

## Options

| Option | Description | Default |
|---|---|---|
| `-p, --port` | Serial device | `/dev/serial0` |
| `-b, --baud` | Baud rate | `115200` |
| `-s, --slave` | Modbus slave ID | `1` |
| `-t, --timeout` | Response timeout in ms | `500` |
| `-d, --distance` | Read distance register `0x0101` | |
| `-r, --read` | Read a specific register | |
| `-w, --write` | Write a value to a register (`addr:value`) | |

## Typical sensor registers

| Address | Meaning | Access |
|---|---|---|
| `0x0100` | Processed distance value | Read |
| `0x0101` | Real-time distance in mm | Read |
| `0x0200` | Modbus slave address | Read/Write |
| `0x0201` | Baud-rate code | Read/Write |
| `0x0202` | Parity setting | Read/Write |
| `0x0209` | Output value type (`0` = mm, `1` = µs) | Read/Write |

After writing a configuration register, power-cycle or reconnect the sensor so the new settings take effect.

## Troubleshooting

### `Permission denied` on `/dev/serial0`

```bash
sudo usermod -a -G dialout $USER
```

### Port is busy

```bash
sudo systemctl stop mdetector-service
```

### No reply from sensor

- Check wiring and power.
- Confirm baud rate and slave ID.
- The wrapper calls `modbus_flush()` before each read to clear stale bytes.
