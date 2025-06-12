#include <AdvancedADC.h>

// Example that demonstrates the analogRead() helper with debug output

AdvancedADC adc(1, A0); // Use ADC1 with pin A0

void setup() {
    Serial.begin(9600);
    while (!Serial) {
    }

    // Start ADC with: resolution, sample rate, samples per channel, queue depth
    if (!adc.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1)
            ;
    }
}

void loop() {
    // analogRead(channel) prints debug information and returns the sample
    adc.analogRead(0);
    delay(100);
}
