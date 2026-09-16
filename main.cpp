#include <cstdint>
#include <iostream>
#include <cstring>
#include <modbus.h>
using namespace std;

int main(const int argc, char *argv[]) {
    bool read_distance = false;

    // 1. Parse command-line parameters
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            read_distance = true;
            break;
        }
    }

    if (!read_distance) {
        cerr << "Usage: " << argv[0] << " -d (to read distance)" << endl;
        return 1;
    }

    // 2. Initialize Modbus RTU context
    modbus_t *ctx = modbus_new_rtu("/dev/ttyAMAO", 115200, 'N', 8, 1);
    if (ctx == nullptr) {
        cerr << "Unable to create the libmodbus context" << endl;
        return -1;
    }

    // Set slave address to the sensor's default (0x01)
    modbus_set_slave(ctx, 1);

    // 3. Connect to the serial port
    if (modbus_connect(ctx) == -1) {
        cerr << "Connection failed: " << modbus_strerror(errno) << endl;
        modbus_free(ctx);
        return -1;
    }

    // 4. Read the Real-Time Distance Register
    uint16_t tab_reg[1];
    // Register 0x0101 holds the real-time distance value
    int rc = modbus_read_registers(ctx, 0x0101, 1, tab_reg);

    if (rc == -1) {
        cerr << "Modbus read error: " << modbus_strerror(errno) << endl;
    } else {
        cout << "Sensor Distance: " << tab_reg[0] << " mm" << endl;
    }

    // 5. Cleanup
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}