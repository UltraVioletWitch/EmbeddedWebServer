add_library(W5500 W5500.c)
target_link_libraries(W5500 PUBLIC pico_stdlib hardware_spi)
