#include <AdvancedADC.h>

// Sample PC_2C, PC_3C and PC_0 on ADC3
// Sample PA_0, PA_1C and PA_0C on ADC1

AdvancedADC adc3(PC_2C, PC_3C, PC_0);
AdvancedADC adc1(PA_0, PA_1C, PA_0C);

uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);

    adc3.setADC(3); // Manually select ADC3
    if (!adc3.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start ADC3 acquisition!");
        while (1);
    }

    adc1.setADC(1); // Manually select ADC1
    if (!adc1.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start ADC1 acquisition!");
        while (1);
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
