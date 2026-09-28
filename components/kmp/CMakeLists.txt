idf_component_register(SRCS "test_kmp_protocol.c" "kmp_protocol.c" "kmp_uart.c" "kmp_client.c"
    REQUIRES esp_driver_uart
    PRIV_REQUIRES unity
    INCLUDE_DIRS "include")
