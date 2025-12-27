/**
 * MINIMAL ESP32-CAM SERIAL TEST
 * Tests ONLY serial output - NO WiFi, NO WebSocket, NO libraries
 */

#include <Arduino.h>

#define SERIAL_BAUD_RATE 115200

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(2000); // Critical: ESP32-CAM needs extended delay for serial chip stabilization

    // Flush bootloader garbage from buffer
    while (Serial.available())
        Serial.read();

    Serial.println("\n\n====================================");
    Serial.println("ESP32-CAM MINIMAL SERIAL TEST");
    Serial.println("====================================");
    Serial.printf("Baud rate: %d\n", SERIAL_BAUD_RATE);
    Serial.printf("Boot time (millis): %lu\n", millis());
    Serial.println("Serial is working!");
    Serial.println("Starting loop...\n");
}

void loop()
{
    static unsigned long counter = 0;
    Serial.printf("PING #%lu at %lu ms\n", counter++, millis());
    delay(1000);
}
