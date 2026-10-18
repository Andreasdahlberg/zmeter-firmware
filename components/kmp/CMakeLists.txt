idf_component_register(SRCS "kmp_protocol.c" "kmp_uart.c" "kmp_client.c"
    REQUIRES esp_driver_uart
    INCLUDE_DIRS "include")
