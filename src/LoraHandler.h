#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef LORA_ENABLED
#include <SPI.h>
#include <LoRa.h>

// Default pins if not defined in platformio.ini
#ifndef CS_PIN
#define CS_PIN 5
#endif
#ifndef RST_PIN
#define RST_PIN 14
#endif
#ifndef IRQ_PIN
#define IRQ_PIN 2
#endif

class LoraHandler
{
public:
    enum LoraMode {
        MODE_OFF = 0,
        MODE_SENDER = 1,
        MODE_RECEIVER = 2
    };

    LoraHandler();
    bool begin(long frequency = 433E6);
    void loop();
    void sendData(JsonObject& data);
    void setMode(LoraMode mode);
    LoraMode getMode();

private:
    LoraMode _mode;
    long _frequency;
    unsigned long _lastSendTime;

    void onReceive(int packetSize);
    void processReceivedPacket(String packet);
};

#endif // LORA_ENABLED
#endif // LORA_HANDLER_H
