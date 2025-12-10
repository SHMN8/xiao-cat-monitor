idf_component_register(SRCS "app_wifi.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi esp_event nvs_flash network_provisioning esp_rainmaker)