#pragma once
#include <stdbool.h>

// XIAO ESP32-C3 Pin Definitions
#define PIN_PIR     2   // D0
#define PIN_BUZZER  3   // D1
#define PIN_LED     4   // D2

void device_init(void);
void device_set_led(bool state);
void device_set_buzzer(bool state);
bool device_read_pir(void);