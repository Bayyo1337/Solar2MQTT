# Solar2MQTT - LoRa Extension

This extension allows you to split the Solar2MQTT system into two nodes using LoRa radio (SX1278/RA-02 modules). This is useful if your inverter is in a location with poor WiFi coverage (e.g., a basement or shed).

## Architecture

1.  **Sender Node (Inverter Side):**
    *   Connected to the Inverter via Serial/Modbus.
    *   Reads data from the inverter.
    *   Broadcasts the data via LoRa every 60 seconds.
    *   Does *not* require a permanent WiFi connection (only for initial config).

2.  **Receiver Node (Home Side):**
    *   Connected to your Home WiFi and MQTT Broker.
    *   Listens for LoRa packets from the Sender.
    *   Forwards the received data to MQTT (Home Assistant) as if it were reading it locally.

## Hardware Setup (ESP32)

We recommend using an **ESP32** (e.g., ESP32-DevKitC or ESP32-C3) combined with an **RA-02 (SX1278)** LoRa module.

### Wiring (Default)

| RA-02 Pin | ESP32 Pin | Function |
| :--- | :--- | :--- |
| 3.3V | 3.3V | Power |
| GND | GND | Ground |
| NSS | GPIO 5 | Chip Select |
| MOSI | GPIO 23 | SPI MOSI |
| MISO | GPIO 19 | SPI MISO |
| SCK | GPIO 18 | SPI Clock |
| RST | GPIO 14 | Reset |
| DIO0 | GPIO 2 | Interrupt |

*Note: Pins can be changed in `platformio.ini` by modifying the `CS_PIN`, `RST_PIN`, and `IRQ_PIN` build flags.*

### Inverter Connection (Sender Only)

Connect your RS232/RS485 adapter to the ESP32 UART pins:
*   **RX:** GPIO 13 (Default)
*   **TX:** GPIO 17 (Default)

*Note: ESP32 uses HardwareSerial (Serial2), which is much more stable than the SoftwareSerial used on ESP8266. We default to GPIO 17 for TX because GPIO 15 is a strapping pin that can prevent booting if pulled high.*

## Configuration

1.  **Flash the Firmware:**
    *   Select the `env:esp32-lora` environment in PlatformIO.
    *   Upload to both ESP32 boards.

2.  **Configure Sender:**
    *   Connect to the WiFi AP (`Solar2MQTT-AP`).
    *   Go to **Settings**.
    *   Set **LoRa Mode** to **Sender (Inverter)**.
    *   Save and Reboot.

3.  **Configure Receiver:**
    *   Connect to the WiFi AP.
    *   Go to **Settings**.
    *   Configure your **MQTT Server**, **User**, and **Password**.
    *   Set **LoRa Mode** to **Receiver (Gateway)**.
    *   Save and Reboot.

## Troubleshooting

*   **No Data?** Check your wiring, especially the SPI pins (MISO/MOSI).
*   **Frequency:** The default frequency is **433MHz**. If you are using 868MHz or 915MHz modules, you must edit `src/LoraHandler.cpp` or add a build flag (future improvement).
*   **Range:** Ensure you have an antenna connected to the RA-02 module. Do not power it up without an antenna!
