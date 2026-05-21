

if(NOT DEFINED GALOPED_MODEL)
    set(GALOPED_MODEL "none")
endif()

if(NOT DEFINED GALOPED_AXIS)
    set(GALOPED_AXIS 1)
endif()

if(NOT DEFINED GALOPED_CLIMATE)
    set(GALOPED_CLIMATE 0)
endif()

idf_component_register(
    SRC_DIRS  "."
    INCLUDE_DIRS "."
    REQUIRES
        esp-32-vid6608-rmt
    PRIV_REQUIRES
        nvs_flash esp_driver_uart esp_driver_gpio esp_driver_spi esp_driver_i2c
        esp_timer ieee802154 led_strip
        esp-zigbee-lib esp-zboss-lib
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    GALOPED_MODEL="${GALOPED_MODEL}"
    GALOPED_AXIS=${GALOPED_AXIS}
    GALOPED_CLIMATE=${GALOPED_CLIMATE}
)
