idf_component_register(
    SRC_DIRS  "."
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_driver_uart esp_driver_gpio esp_driver_spi
                  esp_timer ieee802154 led_strip
                  esp-zigbee-lib esp-zboss-lib
)
