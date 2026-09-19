// diag.cpp
// Command-line diagnostic tool for the ultrasound sensor.

#include "modbus_sensor.hpp"  // Our wrapper class from above
#include <cstdlib>            // For std::exit-related utilities
#include <cstring>            // For C-string functions
#include <getopt.h>           // For parsing long command-line options
#include <iomanip>            // For std::hex / std::dec output formatting
#include <iostream>           // For std::cout / std::cerr
#include <string>             // For std::string

// ============================================================================
// Config struct
// ============================================================================
// This struct holds every setting the user can change from the command line.
// Using a struct keeps related variables together.
// ============================================================================
struct Config {
    std::string port = "/dev/serial0";  // Serial port; serial0 is the Pi symlink
    int baud = 115200;                  // Baud rate
    char parity = 'N';                  // Parity: None/Even/Odd
    uint8_t slave = 1;                  // Modbus slave ID
    int timeout_ms = 500;               // How long to wait for a reply

    bool read_dist = false;             // Should we read the distance register?
    uint16_t read_reg = 0xFFFF;         // Register to read (0xFFFF means "not set")
    uint16_t write_reg = 0xFFFF;        // Register to write (0xFFFF means "not set")
    uint16_t write_val = 0;             // Value to write
};

// ============================================================================
// print_usage
// ============================================================================
// Prints help text when the user runs the program wrong or with --help.
// 'prog' is the program name (argv[0]).
// ============================================================================
static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options]\n"
              << "  -p, --port     serial port (default /dev/serial0)\n"
              << "  -b, --baud     baud rate (default 115200)\n"
              << "  -s, --slave    Modbus slave id (default 1)\n"
              << "  -d, --distance read real-time distance from register 0x0101\n"
              << "  -r, --read     <addr> read one holding register (hex OK)\n"
              << "  -w, --write    <addr>:<value> write holding register (hex OK)\n"
              << "  -t, --timeout  response timeout ms (default 500)\n";
}

// ============================================================================
// parse_hex_or_dec
// ============================================================================
// Helper that converts a string to an integer.
// Accepts decimal ("257") or hex ("0x0101").
// ============================================================================
static int parse_hex_or_dec(const std::string& s) {
    // If the string starts with "0x" or "0X", parse it as base-16 (hex).
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        return std::stoi(s, nullptr, 16);
    }

    // Otherwise parse it as base-10 (decimal).
    return std::stoi(s);
}

// ============================================================================
// main
// ============================================================================
// argc = argument count (including program name)
// argv = argument vector (array of C-strings)
// ============================================================================
int main(int argc, char** argv) {

    Config cfg;  // Create one Config object with all default values.

    // ------------------------------------------------------------------------
    // Set up the long options table for getopt_long.
    // Each row is: { "name", has_arg, flag, val }
    // has_arg: no_argument, required_argument, or optional_argument
    // ------------------------------------------------------------------------
    static struct option long_opts[] = {
        {"port",     required_argument, nullptr, 'p'},
        {"baud",     required_argument, nullptr, 'b'},
        {"slave",    required_argument, nullptr, 's'},
        {"distance", no_argument,       nullptr, 'd'},
        {"read",     required_argument, nullptr, 'r'},
        {"write",    required_argument, nullptr, 'w'},
        {"timeout",  required_argument, nullptr, 't'},
        {"help",     no_argument,       nullptr, 'h'},
        {nullptr,    0,                 nullptr,  0 }   // Marks end of table
    };

    int opt;  // Will hold the current option character.

    // getopt_long loops through argv and returns -1 when finished.
    while ((opt = getopt_long(argc, argv, "p:b:s:dr:w:t:h", long_opts, nullptr)) != -1) {

        // A switch decides what to do for each option.
        switch (opt) {

            case 'p':  // --port or -p
                cfg.port = optarg;  // optarg is the string after the option
                break;

            case 'b':  // --baud or -b
                cfg.baud = std::stoi(optarg);  // Convert string to int
                break;

            case 's':  // --slave or -s
                cfg.slave = static_cast<uint8_t>(std::stoi(optarg));
                break;

            case 'd':  // --distance or -d
                cfg.read_dist = true;
                break;

            case 'r':  // --read or -r
                cfg.read_reg = static_cast<uint16_t>(parse_hex_or_dec(optarg));
                break;

            case 'w': {  // --write or -w
                // The write argument looks like "0x0201:6"
                std::string arg(optarg);

                // Find the ':' separator.
                auto pos = arg.find(':');
                if (pos == std::string::npos) {
                    std::cerr << "write format is <addr>:<value>\n";
                    return 1;  // Return non-zero means error
                }

                // Part before ':' is the register address.
                cfg.write_reg = static_cast<uint16_t>(
                    parse_hex_or_dec(arg.substr(0, pos)));

                // Part after ':' is the value to write.
                cfg.write_val = static_cast<uint16_t>(
                    parse_hex_or_dec(arg.substr(pos + 1)));
                break;
            }

            case 't':  // --timeout or -t
                cfg.timeout_ms = std::stoi(optarg);
                break;

            case 'h':  // --help or -h
                print_usage(argv[0]);
                return 0;  // Zero means success

            default:  // Unknown option
                print_usage(argv[0]);
                return 1;
        }
    }

    // ------------------------------------------------------------------------
    // Sanity check: the user must ask for at least one operation.
    // 0xFFFF is our "not set" sentinel value.
    // ------------------------------------------------------------------------
    if (!cfg.read_dist && cfg.read_reg == 0xFFFF && cfg.write_reg == 0xFFFF) {
        print_usage(argv[0]);
        return 1;
    }

    // ------------------------------------------------------------------------
    // Try to connect to the sensor and perform the requested operations.
    // If anything throws, we catch it and print an error.
    // ------------------------------------------------------------------------
    try {
        // Create the sensor object. The constructor opens the serial port.
        ModbusSensor sensor(cfg.port, cfg.baud, cfg.parity, 8, 1,
                            cfg.slave, cfg.timeout_ms);

        // If the user asked for distance, read register 0x0101.
        if (cfg.read_dist) {
            sensor.flush();                 // Throw away stale bytes
            uint16_t d = sensor.readHolding(0x0101);  // Read distance register
            std::cout << "distance=" << d << " mm\n";
        }

        // If the user asked to read a specific register, do it.
        if (cfg.read_reg != 0xFFFF) {
            uint16_t v = sensor.readHolding(cfg.read_reg);

            // Print in both hex and decimal.
            std::cout << "reg[0x" << std::hex << cfg.read_reg
                      << "]=0x" << v
                      << " (" << std::dec << v << ")\n";
        }

        // If the user asked to write a register, do it.
        if (cfg.write_reg != 0xFFFF) {
            sensor.writeHolding(cfg.write_reg, cfg.write_val);
            std::cout << "wrote 0x" << std::hex << cfg.write_val
                      << " to 0x" << cfg.write_reg << "\n";
        }

    } catch (const std::exception& e) {
        // std::exception is the base class for all standard errors.
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;  // Success
}
