#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>
#include <esp_rmaker_core.h>

#include "app_wifi.h"

static const char *TAG = "app_wifi";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "R_"; // Short prefix for BLE
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

void app_wifi_start(void *ctx)
{
    
    wifi_event_group = xEventGroupCreate();
    
    // Register handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    network_prov_mgr_config_t config = {
        .scheme = network_prov_scheme_ble,
        .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(network_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(network_prov_mgr_is_wifi_provisioned(&provisioned));

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning...");
        
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));
        
        network_prov_security_t security = NETWORK_PROV_SECURITY_1;
        const char *pop = "password"; 
        
        ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(security, pop, service_name, NULL));
        
        ESP_LOGW(TAG, "---------------------------------------------------");
        ESP_LOGW(TAG, "PROVISIONING REQUIRED");
        ESP_LOGW(TAG, "1. Open ESP RainMaker App");
        ESP_LOGW(TAG, "2. Add Device -> Pair via BLE");
        ESP_LOGW(TAG, "3. Name: %s", service_name);
        ESP_LOGW(TAG, "4. POP: %s", pop);
        ESP_LOGW(TAG, "---------------------------------------------------");

    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi...");
        network_prov_mgr_deinit(); 
        esp_wifi_start(); // Start Wi-Fi now that we know we have credentials
    }
    
    // Wait for IP address
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
}
