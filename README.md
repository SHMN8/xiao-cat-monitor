# 🐱 Smart Cat Litter Monitor (ESP32-C3)

A predictive maintenance IoT system for cat litter boxes using the **Seeed Studio XIAO ESP32-C3**. It tracks usage via motion detection, calculates cleanliness based on frequency, and alerts you via **ESP RainMaker** when cleaning is required.

## 🌟 Features
* **Motion Detection:** PIR sensor detects cat entry.
* **Predictive Analysis:** Algorithms calculate "Days Left" until cleaning is needed based on usage capacity (3 inch of littersand/20L).
* **Smart Alerts:**
    * 🟢 "Still Clean"
    * 🟡 "Cleaning Required Soon"
    * 🚨 "IT'S DIRTY! CHANGE NOW!"
* **Hardware Feedback:**
    * White Night Light (Auto-on with motion).
    * Red LED Blink + Buzzer (Audio/Visual Alarm).
* **Cloud Dashboard:** Real-time tracking and resets via ESP RainMaker App.

## 🛠️ Hardware Required
* **Microcontroller:** Seeed Studio XIAO ESP32-C3
* **Sensors:** PIR Motion Sensor (HC-SR501 or AM312)
* **Indicators:**
    * Active Buzzer (5V)
    * Red LED (Alarm)
    * White LED (Night Light)
* **Power:** USB-C (5V)

## 🔌 Wiring
| Component | XIAO Pin | GPIO |
| :--- | :--- | :--- |
| **PIR Sensor**    | D0 | GPIO 2 |
| **Buzzer**        | D1 | GPIO 3 |
| **Red LED**       | D2 | GPIO 4 |
| **White LED**     | D3 | GPIO 5 |

## 🚀 Installation
1.  **Install ESP-IDF:** This project requires ESP-IDF v5.0+.
2.  **Clone the Repo:**
    ```bash
    git clone [https://github.com/SHMN8/xiao-cat-monitor.git](https://github.com/SHMN8/xiao-cat-monitor.git)
    ```
3.  **Build & Flash:**
    ```bash
    idf.py set-target esp32c3
    idf.py build
    idf.py -p PORT flash monitor
    ```
4.  **Provisioning:**
    * Download the **ESP RainMaker** app.
    * Scan the QR code in the terminal (or pair via BLE).
