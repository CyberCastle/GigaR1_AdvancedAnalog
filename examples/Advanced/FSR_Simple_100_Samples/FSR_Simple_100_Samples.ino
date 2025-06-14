/*
  Ejemplo simplificado para obtener 100+ muestras por canal en ciclos start/stop
  Configuración específica para los FSRs definidos
*/

#include <AdvancedADC.h>

// FSRs addresses
#define ANALOG_PORT_FSR_LEFT_TOP PC_2C
#define ANALOG_PORT_FSR_LEFT_CENTER PA_0
#define ANALOG_PORT_FSR_LEFT_BOTTOM PC_0
#define ANALOG_PORT_FSR_RIGHT_TOP PC_3C
#define ANALOG_PORT_FSR_RIGHT_CENTER PA_1C
#define ANALOG_PORT_FSR_RIGHT_BOTTOM PA_0C

// Configuración de ADCs según tu definición
AdvancedADC adc1(1, ANALOG_PORT_FSR_LEFT_CENTER, ANALOG_PORT_FSR_RIGHT_CENTER, ANALOG_PORT_FSR_RIGHT_BOTTOM);
AdvancedADC adc3(3, ANALOG_PORT_FSR_LEFT_TOP, ANALOG_PORT_FSR_RIGHT_TOP, ANALOG_PORT_FSR_LEFT_BOTTOM);

// Parámetros de configuración
const uint32_t SAMPLE_RATE = 2000;        // 2kHz para obtener muestras rápidamente
const size_t SAMPLES_PER_BUFFER = 50;     // 50 muestras por buffer por canal
const size_t NUM_BUFFERS = 8;             // 8 buffers en el pool
const size_t MIN_SAMPLES_REQUIRED = 100;  // Mínimo 100 muestras por canal

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("FSR Sampling: 100+ samples per channel per cycle");
    Serial.println("==============================================");

    // Configurar ADC1 - 3 canales
    if (!adc1.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: Failed to initialize ADC1!");
        while (1) delay(100);
    }
    Serial.println("ADC1 configured: 3 channels");

    // Configurar ADC3 - 3 canales
    if (!adc3.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: Failed to initialize ADC3!");
        while (1) delay(100);
    }
    Serial.println("ADC3 configured: 3 channels");

    Serial.println("Ready to start sampling cycles...");
    delay(1000);
}

void printResults(Sample samples[][MIN_SAMPLES_REQUIRED + 50], size_t counts[],
                 const char* adc_name, size_t num_channels) {
    const char* channel_names[] = {"LEFT_CENTER/LEFT_TOP", "RIGHT_CENTER/RIGHT_TOP", "RIGHT_BOTTOM/LEFT_BOTTOM"};

    Serial.print(adc_name);
    Serial.println(" Results:");

    for (size_t ch = 0; ch < num_channels; ch++) {
        Serial.print("  Channel ");
        Serial.print(ch);
        Serial.print(" (");
        Serial.print(channel_names[ch]);
        Serial.print("): ");
        Serial.print(counts[ch]);
        Serial.print(" samples");

        if (counts[ch] >= MIN_SAMPLES_REQUIRED) {
            // Calcular estadísticas básicas
            uint32_t sum = 0;
            uint16_t min_val = samples[ch][0];
            uint16_t max_val = samples[ch][0];

            for (size_t i = 0; i < counts[ch]; i++) {
                sum += samples[ch][i];
                if (samples[ch][i] < min_val) min_val = samples[ch][i];
                if (samples[ch][i] > max_val) max_val = samples[ch][i];
            }

            float average = (float)sum / counts[ch];

            Serial.print(" - Avg: ");
            Serial.print(average, 1);
            Serial.print(", Min: ");
            Serial.print(min_val);
            Serial.print(", Max: ");
            Serial.print(max_val);
            Serial.println(" ✓");
        } else {
            Serial.println(" ✗ INSUFFICIENT SAMPLES!");
        }
    }
}

void applyDigitalFilters(Sample samples[][MIN_SAMPLES_REQUIRED + 50], size_t counts[],
                        const char* adc_name, size_t num_channels) {
    Serial.print(adc_name);
    Serial.println(" Digital Filter Results:");

    for (size_t ch = 0; ch < num_channels; ch++) {
        if (counts[ch] >= MIN_SAMPLES_REQUIRED) {
            // Filtro de media móvil simple
            float moving_avg = 0.0;
            for (size_t i = 0; i < counts[ch]; i++) {
                moving_avg += samples[ch][i];
            }
            moving_avg /= counts[ch];

            // Filtro paso bajo IIR simple (simula filtrado en tiempo real)
            float filtered_value = samples[ch][0];
            float alpha = 0.1; // Factor de suavizado

            for (size_t i = 1; i < counts[ch]; i++) {
                filtered_value = alpha * samples[ch][i] + (1.0 - alpha) * filtered_value;
            }

            // Calcular desviación estándar
            float variance = 0.0;
            for (size_t i = 0; i < counts[ch]; i++) {
                float diff = samples[ch][i] - moving_avg;
                variance += diff * diff;
            }
            float std_dev = sqrt(variance / counts[ch]);

            Serial.print("  Ch");
            Serial.print(ch);
            Serial.print(" - Moving Avg: ");
            Serial.print(moving_avg, 2);
            Serial.print(", IIR Filtered: ");
            Serial.print(filtered_value, 2);
            Serial.print(", Std Dev: ");
            Serial.println(std_dev, 2);
        }
    }
}

void collectSamplesFromBuffer(SampleBuffer &buf, Sample samples[][MIN_SAMPLES_REQUIRED + 50],
                             size_t counts[], size_t num_channels) {
    size_t buffer_size = buf.size();
    size_t samples_per_channel = buffer_size / num_channels;

    // Las muestras están intercaladas: [Ch0, Ch1, Ch2, Ch0, Ch1, Ch2, ...]
    for (size_t sample = 0; sample < samples_per_channel; sample++) {
        for (size_t ch = 0; ch < num_channels; ch++) {
            if (counts[ch] < MIN_SAMPLES_REQUIRED + 50) {
                size_t buffer_index = sample * num_channels + ch;
                if (buffer_index < buffer_size) {
                    samples[ch][counts[ch]] = buf[buffer_index];
                    counts[ch]++;
                }
            }
        }
    }
}


void performCaptureCycle() {
    // Arrays para almacenar muestras de cada canal
    Sample adc1_samples[3][MIN_SAMPLES_REQUIRED + 50]; // +50 buffer extra
    Sample adc3_samples[3][MIN_SAMPLES_REQUIRED + 50];
    size_t adc1_counts[3] = {0, 0, 0};
    size_t adc3_counts[3] = {0, 0, 0};

    // Limpiar buffers previos
    adc1.clear();
    adc3.clear();

    // FASE 1: INICIAR CONVERSIÓN
    Serial.println("Phase 1: Starting ADC conversion...");

    if (!adc1.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC1");
        return;
    }

    if (!adc3.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC3");
        adc1.stop();
        return;
    }

    Serial.println("Both ADCs started successfully");

    // FASE 2: RECOLECTAR MUESTRAS
    Serial.println("Phase 2: Collecting samples...");

    unsigned long start_time = millis();
    bool enough_samples = false;

    while (!enough_samples && (millis() - start_time < 1000)) { // Timeout de 1s
        // Recolectar de ADC1
        while (adc1.available()) {
            SampleBuffer buf = adc1.read();
            collectSamplesFromBuffer(buf, adc1_samples, adc1_counts, 3);
            buf.release();
        }

        // Recolectar de ADC3
        while (adc3.available()) {
            SampleBuffer buf = adc3.read();
            collectSamplesFromBuffer(buf, adc3_samples, adc3_counts, 3);
            buf.release();
        }

        // Verificar si tenemos suficientes muestras en todos los canales
        enough_samples = true;
        for (int ch = 0; ch < 3; ch++) {
            if (adc1_counts[ch] < MIN_SAMPLES_REQUIRED ||
                adc3_counts[ch] < MIN_SAMPLES_REQUIRED) {
                enough_samples = false;
                break;
            }
        }

        // Pequeña pausa para no saturar el CPU
        delayMicroseconds(100);
    }

    // FASE 3: DETENER CONVERSIÓN
    Serial.println("Phase 3: Stopping ADC conversion...");

    adc1.stop();
    adc3.stop();

    // Recoger buffers restantes
    while (adc1.available()) {
        SampleBuffer buf = adc1.read();
        collectSamplesFromBuffer(buf, adc1_samples, adc1_counts, 3);
        buf.release();
    }

    while (adc3.available()) {
        SampleBuffer buf = adc3.read();
        collectSamplesFromBuffer(buf, adc3_samples, adc3_counts, 3);
        buf.release();
    }

    // FASE 4: PROCESAR Y MOSTRAR RESULTADOS
    Serial.println("Phase 4: Processing results...");

    unsigned long capture_time = millis() - start_time;
    Serial.print("Capture completed in: ");
    Serial.print(capture_time);
    Serial.println(" ms");

    printResults(adc1_samples, adc1_counts, "ADC1", 3);
    printResults(adc3_samples, adc3_counts, "ADC3", 3);

    // FASE 5: APLICAR FILTROS DIGITALES
    Serial.println("Phase 5: Applying digital filters...");
    applyDigitalFilters(adc1_samples, adc1_counts, "ADC1", 3);
    applyDigitalFilters(adc3_samples, adc3_counts, "ADC3", 3);
}





void loop() {
    Serial.println("\n--- Starting new capture cycle ---");

    // Ejecutar un ciclo completo de captura
    performCaptureCycle();

    // Esperar antes del próximo ciclo
    delay(2000);
}
