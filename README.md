### Ultrasound-diag

It's a simple tool to diagnose an ultrasound sensor

For now it can just read the distance from an ultrasound sensor. If you have it connected to a different port, please fix the definition of PORT in code.

To use just add -d when launching the binary
```Bash
  ./sensor_diag -d
```

If you want to compile it, you'd need the libmodbus-dev library.
```Bash
  sudo apt-get install libmodbus-dev
```
