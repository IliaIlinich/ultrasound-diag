cmake_minimum_required(VERSION 3.10)
project(ultrasound-diag CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

add_executable(ultrasound-diag diag.cpp)
target_include_directories(ultrasound-diag PRIVATE ${LIBMODBUS_INCLUDE_DIRS})
target_link_libraries(ultrasound-diag ${LIBMODBUS_LIBRARIES})
