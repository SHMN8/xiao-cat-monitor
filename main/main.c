#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <driver/gpio.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_ota.h>
#include <app_wifi.h>

static const char *TAG = "LITTER_MONITOR";

// --- Hardware Pins ---
#define PIN_PIR         2  // D0
#define PIN_BUZZER      3  // D1
#define PIN_RED_LED     4  // D2
#define PIN_WHITE_LED   5  // D3

// --- Logic Constants ---
#define MAX_VISITS_BEFORE_DIRTY 30
#define NVS_NAMESPACE "storage"
#define NVS_KEY_VISITS "visits"

// --- Global State ---
bool g_alarm_armed = true;
bool g_white_led_manual = false;
int  g_visit_count = 0;
esp_rmaker_device_t *monitor_device = NULL;

// --- Helper: NVS Storage ---
void save_count_to_nvs(int count) {
    nvs_handle_t my_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_i32(my_handle, NVS_KEY_VISITS, count);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

int load_count_from_nvs() {
    nvs_handle_t my_handle;
    int32_t saved_count = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_i32(my_handle, NVS_KEY_VISITS, &saved_count);
        nvs_close(my_handle);
    }
    return (int)saved_count;
}

// --- VISUAL UI LOGIC ENGINE ---
void update_app_status() {
    if (!monitor_device) return;

    int visits_left = MAX_VISITS_BEFORE_DIRTY - g_visit_count;
    
    // 1. Visit Count
    char count_ui_str[32];
    const char *color_icon;

    if (g_visit_count < 10) {
        color_icon = "🟢"; // Clean
    } else if (g_visit_count < 25) {
        color_icon = "🟡"; // Warning
    } else {
        color_icon = "🔴"; // Dirty
    }
    
    snprintf(count_ui_str, sizeof(count_ui_str), "%s %d Visits", color_icon, g_visit_count);

    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_name(monitor_device, "Visit Count"),
        esp_rmaker_str(count_ui_str)
    );

    // 2. Countdown & Cleanliness Status
    char countdown_str[100];
    char status_note[64];
    int days_left = (visits_left < 0) ? 0 : (visits_left / 5); 

    if (visits_left > 15) {
        snprintf(status_note, sizeof(status_note), "Still Clean");
        snprintf(countdown_str, sizeof(countdown_str), "🗓️ %d Days Left\n(%s)", days_left, status_note);
    } else if (visits_left > 0) {
        snprintf(status_note, sizeof(status_note), "Cleaning required Soon");
        snprintf(countdown_str, sizeof(countdown_str), "⏳ %d Day(s) Left\n(%s)", days_left, status_note);
    } else {
        snprintf(status_note, sizeof(status_note), "Its Dirty, CHANGE NOW!");
        snprintf(countdown_str, sizeof(countdown_str), "🚨 CAPACITY FULL\n(%s)", status_note);
    }

    // Update Params
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_name(monitor_device, "Countdown"),
        esp_rmaker_str(countdown_str)
    );
    
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_name(monitor_device, "Cleanliness"),
        esp_rmaker_str(status_note)
    );
}

// --- Hardware Init ---
void setup_hardware() {
    gpio_reset_pin(PIN_PIR); gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);
    gpio_reset_pin(PIN_BUZZER); gpio_set_direction(PIN_BUZZER, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_RED_LED); gpio_set_direction(PIN_RED_LED, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_WHITE_LED); gpio_set_direction(PIN_WHITE_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BUZZER, 0); gpio_set_level(PIN_RED_LED, 0); gpio_set_level(PIN_WHITE_LED, 0);
}

// --- RainMaker Callback ---
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);

    if (strcmp(param_name, "Motion Detection system") == 0) {
        g_alarm_armed = val.val.b;
    } 
    else if (strcmp(param_name, "White Light") == 0) {
        g_white_led_manual = val.val.b;
        gpio_set_level(PIN_WHITE_LED, g_white_led_manual);
    }
    else if (strcmp(param_name, "Reset Counter") == 0) {
        if (val.val.b == true) { 
            g_visit_count = 0;
            save_count_to_nvs(0);
            update_app_status();
            esp_rmaker_param_update_and_report(param, esp_rmaker_bool(false)); 
        }
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

// --- Logic Task ---
void monitor_task(void *pvParameters) {
    bool motion_active = false;
    bool prev_motion = false;
    int blink_timer = 0;

    while (1) {
        motion_active = gpio_get_level(PIN_PIR);

        // Entry Detection
        if (motion_active && !prev_motion) {
            ESP_LOGI(TAG, "Cat Entry Detected!");
            g_visit_count++;
            save_count_to_nvs(g_visit_count);
            update_app_status(); 
        }

        // Output Logic
        if (motion_active) {
            gpio_set_level(PIN_WHITE_LED, 1);
            if (g_alarm_armed) {
                if (blink_timer % 2 == 0) {
                    gpio_set_level(PIN_RED_LED, 1); gpio_set_level(PIN_BUZZER, 1);
                } else {
                    gpio_set_level(PIN_RED_LED, 0); gpio_set_level(PIN_BUZZER, 0);
                }
                blink_timer++;
            }
        } else {
            if (!g_white_led_manual) gpio_set_level(PIN_WHITE_LED, 0); 
            gpio_set_level(PIN_RED_LED, 0);
            gpio_set_level(PIN_BUZZER, 0);
            blink_timer = 0;
        }
        prev_motion = motion_active;
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void app_main() {
    esp_log_level_set("*", ESP_LOG_WARN);
    
    // Init Order
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase(); nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    setup_hardware();
    g_visit_count = load_count_from_nvs();

    // 4. Init RainMaker
    esp_rmaker_config_t rmaker_cfg = { .enable_time_sync = false };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rmaker_cfg, "Cat Litter Box - Predictive Analysis", "Cat_Box_Detection");
    
    // 5. Build UI Device
    monitor_device = esp_rmaker_device_create("Cat Litter Box - Predictive Analysis", NULL, NULL);

    // Param 1: Visit Count
    esp_rmaker_param_t *count_param = esp_rmaker_param_create("Visit Count", ESP_RMAKER_PARAM_NAME, esp_rmaker_str("Initializing..."), PROP_FLAG_READ);
    esp_rmaker_device_add_param(monitor_device, count_param);

    // Param 2: Countdown
    esp_rmaker_param_t *countdown_param = esp_rmaker_param_create("Countdown", ESP_RMAKER_PARAM_NAME, esp_rmaker_str("Calculating..."), PROP_FLAG_READ);
    esp_rmaker_device_add_param(monitor_device, countdown_param);

    // Param 3: Cleanliness Status
    esp_rmaker_param_t *status_param = esp_rmaker_param_create("Cleanliness", ESP_RMAKER_PARAM_NAME, esp_rmaker_str("Initializing..."), PROP_FLAG_READ);
    esp_rmaker_param_add_ui_type(status_param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(monitor_device, status_param);

    // Param 4: Reset
    esp_rmaker_param_t *reset_param = esp_rmaker_param_create("Reset Counter", ESP_RMAKER_PARAM_POWER, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(reset_param, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(monitor_device, reset_param);

    // Param 5 & 6: Controls
    esp_rmaker_device_add_param(monitor_device, esp_rmaker_power_param_create("White Light", false));
    esp_rmaker_device_add_param(monitor_device, esp_rmaker_power_param_create("Motion Detection system", true));

    esp_rmaker_device_add_cb(monitor_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, monitor_device);
    
    esp_rmaker_ota_enable_default(); 
    esp_rmaker_start();
    app_wifi_start(NULL);

    update_app_status(); 
    xTaskCreate(monitor_task, "monitor_task", 4096, NULL, 5, NULL);
}
