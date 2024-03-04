add_executable(spi_mcp3002_adc
        main.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(spi_mcp3002_adc pico_stdlib hardware_adc hardware_spi)

# From  ~/pico/pico-examples/hello_world/usb/CMakeLists.txt
# enable usb output, disable uart output
pico_enable_stdio_usb(spi_mcp3002_adc 1)
pico_enable_stdio_uart(spi_mcp3002_adc 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_mcp3002_adc)