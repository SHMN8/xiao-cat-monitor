#include "device_driver.h"
#include "driver/gpio.h"

void device_init(void) {
    // Configure LED and Buzzer as Outputs
    gpio_reset_pin(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED, 0);

    gpio_reset_pin(PIN_BUZZER);
    gpio_set_direction(PIN_BUZZER, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BUZZER, 0);

    // Configure PIR as Input
    gpio_reset_pin(PIN_PIR);
    gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);
    gpio_pullup_dis(PIN_PIR);   // PIR usually drives active High/Low actively
    gpio_pulldown_en(PIN_PIR);  // Pull down to avoid floating noise
}

void device_set_led(bool state) {
    gpio_set_level(PIN_LED, state);
}

void device_set_buzzer(bool state) {
    gpio_set_level(PIN_BUZZER, state);
}

bool device_read_pir(void) {
    return gpio_get_level(PIN_PIR) == 1;
}