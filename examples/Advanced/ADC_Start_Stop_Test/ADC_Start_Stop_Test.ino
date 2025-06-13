#include <AdvancedADC.h>

// Example demonstrating proper start/stop/end usage
// This example shows how to:
// 1. Initialize the ADC with begin()
// 2. Start and stop conversions multiple times
// 3. Properly release resources with end()

AdvancedADC adc(1, A0); // Use ADC1 with pin A0

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }

    Serial.println("ADC Start/Stop/End Test");
    Serial.println("=====================");

    // Initialize ADC: resolution, sample rate, samples per channel, queue depth
    if (!adc.begin(AN_RESOLUTION_12, 1000, 64, 8, false)) {  // Don't start immediately
        Serial.println("Failed to initialize ADC!");
        while (1) {
            delay(100);
        }
    }
    Serial.println("ADC initialized successfully");

    // Test multiple start/stop cycles
    testStartStopCycles();

    // Test end() and reinitialize
    testEndAndReinitialize();

    Serial.println("All tests completed successfully!");
}

void testStartStopCycles() {
    Serial.println("\nTesting start/stop cycles...");

    for (int cycle = 1; cycle <= 3; cycle++) {
        Serial.print("Cycle "); Serial.print(cycle); Serial.println(":");

        // Start conversion
        if (!adc.start(1000)) {
            Serial.println("  ERROR: Failed to start ADC!");
            return;
        }
        Serial.println("  Started ADC conversion");

        // Read some samples
        for (int i = 0; i < 5; i++) {
            while (!adc.available()) {
                delay(1);
            }
            SampleBuffer buf = adc.read();
            Serial.print("    Sample "); Serial.print(i);
            Serial.print(": "); Serial.println(buf[0]);
            buf.release();
        }

        // Stop conversion
        if (!adc.stop()) {
            Serial.println("  ERROR: Failed to stop ADC!");
            return;
        }
        Serial.println("  Stopped ADC conversion");

        delay(100); // Brief pause between cycles
    }
    Serial.println("Start/stop cycles completed successfully!");
}

void testEndAndReinitialize() {
    Serial.println("\nTesting end() and reinitialize...");

    // End current ADC session (releases all resources)
    if (!adc.end()) {
        Serial.println("ERROR: Failed to end ADC!");
        return;
    }
    Serial.println("ADC ended successfully");

    // Try to start without begin() - should fail
    if (adc.start(1000)) {
        Serial.println("ERROR: start() should have failed after end()!");
        return;
    }
    Serial.println("Correctly failed to start after end() - as expected");

    // Reinitialize with different parameters
    if (!adc.begin(AN_RESOLUTION_14, 2000, 32, 4, false)) {
        Serial.println("ERROR: Failed to reinitialize ADC!");
        return;
    }
    Serial.println("ADC reinitialized with new parameters");

    // Start and take a few samples to verify it works
    if (!adc.start(2000)) {
        Serial.println("ERROR: Failed to start after reinitialize!");
        return;
    }
    Serial.println("ADC started after reinitialize");

    // Read a few samples to verify everything works
    for (int i = 0; i < 3; i++) {
        while (!adc.available()) {
            delay(1);
        }
        SampleBuffer buf = adc.read();
        Serial.print("  Reinitialized sample "); Serial.print(i);
        Serial.print(": "); Serial.println(buf[0]);
        buf.release();
    }

    Serial.println("End and reinitialize test completed successfully!");
}

void loop() {
    // Main loop - just blink to show the system is running
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > 1000) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
        lastBlink = millis();
    }
}
