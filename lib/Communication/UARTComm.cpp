// lib/Communication/UARTComm.cpp
#include "UARTComm.h"

UARTComm::UARTComm(HardwareSerial &serial, uint32_t baudRate)
    : _serial(serial), _baudRate(baudRate), _lastReceived(0) {}

void UARTComm::begin()
{
    // Serial port already initialized in main code
    _lastReceived = millis();
    DEBUG_PRINTLN("UART Communication initialized");
}

bool UARTComm::available()
{
    return _serial.available() > 0;
}

void UARTComm::sendMessage(const JsonDocument &doc)
{
    String output;
    serializeJson(doc, output);

    // Send with newline delimiter
    _serial.println(output);
    _serial.flush();
}

StaticJsonDocument<512> UARTComm::receiveMessage()
{
    StaticJsonDocument<512> doc;

    if (!_serial.available())
    {
        return doc;
    }

    String message = readLine();

    if (message.length() == 0)
    {
        return doc;
    }

    // Parse JSON
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        DEBUG_PRINT("JSON parse error: ");
        DEBUG_PRINTLN(error.c_str());
        doc.clear();
        return doc;
    }

    _lastReceived = millis();
    return doc;
}

String UARTComm::readLine()
{
    String line = "";
    unsigned long startTime = millis();
    const unsigned long timeout = 100; // 100ms timeout

    while (millis() - startTime < timeout)
    {
        if (_serial.available())
        {
            char c = _serial.read();

            if (c == '\n')
            {
                break;
            }

            if (c != '\r')
            { // Ignore carriage return
                line += c;
            }
        }
    }

    return line;
}

unsigned long UARTComm::getLastReceived()
{
    return _lastReceived;
}