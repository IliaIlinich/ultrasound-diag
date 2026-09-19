# CMakeLists.txt

# Minimum required CMake version.
cmake_minimum_required(VERSION 3.10)

# Project name and the language we use (C++).
project(ultrasound CXX)

# Use C++17 standard.
set(CMAKE_CXX_STANDARD 17)

# Find pkg-config, a helper that knows where libmodbus is installed.
find_package(PkgConfig REQUIRED)

# Use pkg-config to get compiler flags and library paths for libmodbus.
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

# Build the diagnostic tool from diag.cpp.
add_executable(ultrasound-diag diag.cpp)

# Tell the compiler where libmodbus headers live.
target_include_directories(ultrasound-diag PRIVATE ${LIBMODBUS_INCLUDE_DIRS})

# Link the diagnostic tool against libmodbus.
target_link_libraries(ultrasound-diag ${LIBMODBUS_LIBRARIES})

# Build the motion-detector service from mdetector.cpp.
add_executable(mdetector mdetector.cpp)

# Same include/link steps for the service.
target_include_directories(mdetector PRIVATE ${LIBMODBUS_INCLUDE_DIRS})
target_link_libraries(mdetector ${LIBMODBUS_LIBRARIES})
