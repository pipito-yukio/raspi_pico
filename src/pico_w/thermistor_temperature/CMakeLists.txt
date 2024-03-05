# From ~/pico/pico-examples/adc/hello_adc/CMakeLists.txt
add_executable(thermistor_temper
        main.c
        )

target_link_libraries(thermistor_temper pico_stdlib hardware_adc)

# From  ~/pico/pico-examples/hello_world/usb/CMakeLists.txt
# enable usb output, disable uart output
pico_enable_stdio_usb(thermistor_temper 1)
pico_enable_stdio_uart(thermistor_temper 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(thermistor_temper)
