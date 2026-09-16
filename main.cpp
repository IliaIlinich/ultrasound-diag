#include <cstdint>
#include <iostream>
#include <cstring>
#include <modbus.h>

#define PORT "/dev/ttyAMA0"

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
    modbus_t *ctx = modbus_new_rtu(PORT, 115200, 'N', 8, 1);
    if (ctx == nullptr) {
        cerr << "Unable to create the libmodbus context" << endl;
        return -1;
    }

    modbus_set_response_timeout(ctx, 0, 500000);

    // Set slave address to the sensor's default (0x01)
    modbus_set_slave(ctx, 1);

    // 3. Connect to the serial port
    if (modbus_connect(ctx) == -1) {
        cerr << "Connection failed: " << modbus_strerror(errno) << endl;
        modbus_free(ctx);
        return -1;
    }

    // 4. Read the Real-Time Distance Register with Retry Logic
    uint16_t tab_reg[1];
    int rc = -1;
    int max_retries = 3;

    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        // Flush the buffer to clear out any UART automatic 0xFF frames
        modbus_flush(ctx);

        // Register 0x0101 holds the real-time distance value
        rc = modbus_read_registers(ctx, 0x0101, 1, tab_reg);

        if (rc != -1) {
            break; // Success, exit the retry loop
        }

        // If it fails, the loop will flush and try again automatically
    }

    if (rc == -1) {
        std::cerr << "Modbus read failed after " << max_retries << " attempts: "
                  << modbus_strerror(errno) << std::endl;
    } else {
        std::cout << "Sensor Distance (" << PORT << "): " << tab_reg[0] << " mm" << std::endl;
    }

    // 5. Cleanup
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}