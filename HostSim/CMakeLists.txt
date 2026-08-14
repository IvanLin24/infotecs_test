cmake_minimum_required(VERSION 3.16)
project(spi_test_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(spi_test_sim
    main.c
    ../App/Src/crc16.c
    ../App/Src/protocol.c
    ../App/Src/exchange.c
)

target_include_directories(spi_test_sim PRIVATE
    ../App/Inc
)
