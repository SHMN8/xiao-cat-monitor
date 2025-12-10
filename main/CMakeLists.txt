idf_component_register(SRCS "main.c" "device_driver.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_rainmaker app_wifi driver esp_driver_gpio)