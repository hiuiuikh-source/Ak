// HWME_AI_v5.ino

#include <Arduino.h>

// Define constants
const int LED_PIN = 13;

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    static int count = 0;
    count++;

    // Error handling example
    if (count > 100) {
        Serial.println("Error: Count exceeded limit!");
        while (true); // Stop the program
    }

    // Blink the LED
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);

    // Log the count to the serial monitor
    Serial.println(count);
}