#include "LoraHandler.h"
#include "Settings.h"

#ifdef LORA_ENABLED

extern JsonObject liveData;
extern void writeLog(const char *format, ...);
extern bool sendtoMQTT(); // defined in main.cpp

LoraHandler::LoraHandler() {
    _mode = MODE_OFF;
    _lastSendTime = 0;
    _frequency = 433E6; // Default to 433MHz
}

bool LoraHandler::begin(long frequency) {
    _frequency = frequency;

    // Set pins
    LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);

    if (!LoRa.begin(_frequency)) {
        writeLog("Starting LoRa failed!");
        return false;
    }

    // LoRa.setSyncWord(0xF3); // Optional: ranges from 0-0xFF, default 0x12
    writeLog("LoRa Init OK at frequency %ld", _frequency);
    return true;
}

void LoraHandler::setMode(LoraMode mode) {
    _mode = mode;
    if (_mode == MODE_RECEIVER) {
        LoRa.receive(); // Put into receive mode
    }
}

LoraHandler::LoraMode LoraHandler::getMode() {
    return _mode;
}

void LoraHandler::loop() {
    if (_mode == MODE_RECEIVER) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            onReceive(packetSize);
        }
    }
}

void LoraHandler::sendData(JsonObject& data) {
    if (_mode != MODE_SENDER) return;

    String jsonString;
    serializeJson(data, jsonString);

    LoRa.beginPacket();
    LoRa.print(jsonString);
    LoRa.endPacket();

    writeLog("Sent LoRa packet: %s", jsonString.c_str());
    _lastSendTime = millis();
}

void LoraHandler::onReceive(int packetSize) {
    if (packetSize == 0) return;

    String packet = "";
    while (LoRa.available()) {
        packet += (char)LoRa.read();
    }

    writeLog("Received LoRa packet: RSSI %d", LoRa.packetRssi());
    processReceivedPacket(packet);
}

void LoraHandler::processReceivedPacket(String packet) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, packet);

    if (error) {
        writeLog("deserializeJson() failed: %s", error.c_str());
        return;
    }

    JsonObject root = doc.as<JsonObject>();

    // Merge received data into liveData
    for (JsonPair kv : root) {
        liveData[kv.key()] = kv.value();
    }

    // Trigger MQTT send on the receiver node
    sendtoMQTT();
}

#endif // LORA_ENABLED
