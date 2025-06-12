#include <AdvancedADC.h>

// Sample PC_2C, PC_3C and PC_0 on ADC3
// Sample PA_0, PA_1C and PA_0C on ADC1

AdvancedADC adc3(3, PC_2C, PC_3C, PC_0); // Use ADC3 with specified pins
AdvancedADC adc1(1, PA_0, PA_1C, PA_0C); // Use ADC1 with specified pins

uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);

    // No need to manually set ADC since it's specified in constructor
    if (!adc3.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start ADC3 acquisition!");
        while (1)
            ;
    }

    // No need to manually set ADC since it's specified in constructor
    if (!adc1.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start ADC1 acquisition!");
        while (1)
            ;
    }
}

void print_adc_data(AdvancedADC &adc) {
    if (adc.available()) {
        SampleBuffer buf = adc.read();
        Serial.println(buf[0]);
        buf.release();
    }
}

void loop() {
    if (millis() - last_millis > 1) {
        print_adc_data(adc3);
        print_adc_data(adc1);
        last_millis = millis();
    }
}
