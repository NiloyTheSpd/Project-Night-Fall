/**
 * ESP32-CAM DIAGNOSTIC TEST SKETCH
 * ================================
 * This sketch tests ONLY basic functionality:
 * - Serial output (GPIO1=TX, GPIO3=RX)
 * - Built-in LED blink (GPIO4)
 * 
 * NO WiFi, NO WebSocket, NO Camera, NO SD Card
 * If this doesn't work, it's a hardware/bootmode issue.
 * 
 * IMPORTANT: Ensure GPIO0 is NOT connected to GND during boot!
 * GPIO0 to GND = Flash mode (no code execution)
 * GPIO0 floating = Run mode (normal execution)
 */

#include <Arduino.h>

// ESP32-CAM has built-in white LED on GPIO4 (also camera flash)
#define LED_BUILTIN_CAM 4

// Use explicit UART pins for ESP32-CAM
// GPIO1 = TX (to FTDI RX)
// GPIO3 = RX (from FTDI TX)
#define UART_TX_PIN 1
#define UART_RX_PIN 3

void setup()
{
    // ========================================
    // STAGE 0: Configure LED immediately
    // If LED works, we know the chip is running
    // ========================================
    pinMode(LED_BUILTIN_CAM, OUTPUT);
    digitalWrite(LED_BUILTIN_CAM, HIGH);  // Turn ON LED immediately
    
    // ========================================
    // STAGE 1: Initialize Serial with explicit pins
    // ========================================
    // ESP32-CAM uses CH340 or CP2102 USB-Serial chip
    // Communication happens on GPIO1 (TX) and GPIO3 (RX)
    Serial.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    // ========================================
    // STAGE 2: Extended delay for USB-Serial chip
    // ESP32-CAM needs longer delay than regular ESP32
    // ========================================
    delay(2000);
    
    // ========================================
    // STAGE 3: Flush any bootloader garbage
    // ========================================
    while (Serial.available()) {
        Serial.read();
    }
    Serial.flush();  // Wait for TX buffer to empty
    
    // ========================================
    // STAGE 4: Print boot message
    // ========================================
    Serial.println();
    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32-CAM DIAGNOSTIC TEST");
    Serial.println("========================================");
    Serial.println("[OK] Serial initialized");
    Serial.printf("[OK] Baud rate: 115200\n");
    Serial.printf("[OK] TX pin: GPIO%d\n", UART_TX_PIN);
    Serial.printf("[OK] RX pin: GPIO%d\n", UART_RX_PIN);
    Serial.printf("[OK] Boot time: %lu ms\n", millis());
    Serial.println();
    Serial.println("If you see this message, serial is working!");
    Serial.println("LED on GPIO4 will now blink every second.");
    Serial.println("========================================");
    Serial.println();
    
    // Turn off LED after boot message
    digitalWrite(LED_BUILTIN_CAM, LOW);
}

void loop()
{
    static unsigned long counter = 0;
    static unsigned long lastBlink = 0;
    unsigned long now = millis();
    
    // Blink LED every second
    if (now - lastBlink >= 1000) {
        lastBlink = now;
        
        // Toggle LED
        static bool ledState = false;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN_CAM, ledState);
        
        // Print status
        Serial.printf("[%lu] PING #%lu | LED: %s | Uptime: %lu sec\n",
                      now,
                      counter++,
                      ledState ? "ON" : "OFF",
                      now / 1000);
    }
    
    // Echo any received characters (for testing RX)
    if (Serial.available()) {
        char c = Serial.read();
        Serial.printf("[RX] Received: '%c' (0x%02X)\n", c, c);
    }
}
