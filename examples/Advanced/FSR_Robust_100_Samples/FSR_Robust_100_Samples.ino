/*
  Versión simplificada y robusta para obtener 100+ muestras por canal
  Compatible con PlatformIO y Arduino IDE

  Configuración:
  - ADC1: 3 canales (LEFT_CENTER, RIGHT_CENTER, RIGHT_BOTTOM)
  - ADC3: 3 canales (LEFT_TOP, RIGHT_TOP, LEFT_BOTTOM)
  - Mínimo 100 muestras por canal por ciclo
*/

#include <AdvancedADC.h>

// FSRs addresses
#define ANALOG_PORT_FSR_LEFT_TOP PC_2C
#define ANALOG_PORT_FSR_LEFT_CENTER PA_0
#define ANALOG_PORT_FSR_LEFT_BOTTOM PC_0
#define ANALOG_PORT_FSR_RIGHT_TOP PC_3C
#define ANALOG_PORT_FSR_RIGHT_CENTER PA_1C
#define ANALOG_PORT_FSR_RIGHT_BOTTOM PA_0C

// Configuración de ADCs
AdvancedADC adc1(1, ANALOG_PORT_FSR_LEFT_CENTER, ANALOG_PORT_FSR_RIGHT_CENTER, ANALOG_PORT_FSR_RIGHT_BOTTOM);
AdvancedADC adc3(3, ANALOG_PORT_FSR_LEFT_TOP, ANALOG_PORT_FSR_RIGHT_TOP, ANALOG_PORT_FSR_LEFT_BOTTOM);

// Configuración de muestreo
const uint32_t SAMPLE_RATE = 2000;
const size_t SAMPLES_PER_BUFFER = 50;
const size_t NUM_BUFFERS = 8;
const size_t MIN_SAMPLES_REQUIRED = 100;

// Almacenamiento de muestras
Sample adc1_data[3][MIN_SAMPLES_REQUIRED + 50];
Sample adc3_data[3][MIN_SAMPLES_REQUIRED + 50];
size_t adc1_count[3];
size_t adc3_count[3];

// Variables de control
bool capturing = false;
unsigned long capture_start = 0;

// Declaraciones de funciones
void initializeData();
bool startCapture();
void stopCapture();
void collectFromADC(AdvancedADC &adc, Sample data[][MIN_SAMPLES_REQUIRED + 50], size_t count[], size_t channels);
void processResults();
void printChannelStats(Sample data[][MIN_SAMPLES_REQUIRED + 50], size_t count[], const char* name, size_t channels);
bool hasEnoughSamples();

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("FSR Sampling - 100+ samples per channel");
    Serial.println("======================================");

    // Inicializar datos
    initializeData();

    // Configurar ADC1
    if (!adc1.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: ADC1 initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("ADC1 configured (3 channels)");

    // Configurar ADC3
    if (!adc3.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: ADC3 initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("ADC3 configured (3 channels)");

    Serial.println("System ready!\n");
    delay(1000);
}

void loop() {
    if (!capturing) {
        Serial.println("--- Starting capture cycle ---");
        if (startCapture()) {
            capture_start = millis();
            capturing = true;
        } else {
            Serial.println("Failed to start capture!");
            delay(1000);
            return;
        }
    }

    // Recolectar muestras durante la captura
    if (capturing) {
        collectFromADC(adc1, adc1_data, adc1_count, 3);
        collectFromADC(adc3, adc3_data, adc3_count, 3);

        // Verificar condiciones de parada
        if (hasEnoughSamples() || (millis() - capture_start > 1000)) {
            stopCapture();
            processResults();
            capturing = false;
            delay(2000); // Pausa entre ciclos
        }
    }
}

void initializeData() {
    for (int ch = 0; ch < 3; ch++) {
        adc1_count[ch] = 0;
        adc3_count[ch] = 0;
    }
}

bool startCapture() {
    // Limpiar buffers y contadores
    adc1.clear();
    adc3.clear();
    initializeData();

    // Iniciar ADC1
    if (!adc1.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC1");
        return false;
    }

    // Iniciar ADC3
    if (!adc3.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC3");
        adc1.stop();
        return false;
    }

    Serial.println("Capture started - collecting samples...");
    return true;
}

void stopCapture() {
    Serial.println("Stopping capture...");

    // Detener ambos ADCs
    adc1.stop();
    adc3.stop();

    // Recoger buffers restantes
    collectFromADC(adc1, adc1_data, adc1_count, 3);
    collectFromADC(adc3, adc3_data, adc3_count, 3);

    unsigned long duration = millis() - capture_start;
    Serial.print("Capture completed in ");
    Serial.print(duration);
    Serial.println(" ms");
}

void collectFromADC(AdvancedADC &adc, Sample data[][MIN_SAMPLES_REQUIRED + 50], size_t count[], size_t channels) {
    while (adc.available()) {
        SampleBuffer buf = adc.read();
        size_t buf_size = buf.size();
        size_t samples_per_ch = buf_size / channels;

        // Extraer muestras intercaladas
        for (size_t s = 0; s < samples_per_ch; s++) {
            for (size_t ch = 0; ch < channels; ch++) {
                if (count[ch] < MIN_SAMPLES_REQUIRED + 50) {
                    size_t idx = s * channels + ch;
                    if (idx < buf_size) {
                        data[ch][count[ch]] = buf[idx];
                        count[ch]++;
                    }
                }
            }
        }
        buf.release();
    }
}

bool hasEnoughSamples() {
    // Verificar que todos los canales tengan suficientes muestras
    for (int ch = 0; ch < 3; ch++) {
        if (adc1_count[ch] < MIN_SAMPLES_REQUIRED || adc3_count[ch] < MIN_SAMPLES_REQUIRED) {
            return false;
        }
    }
    return true;
}

void processResults() {
    Serial.println("\n=== RESULTS ===");
    printChannelStats(adc1_data, adc1_count, "ADC1", 3);
    printChannelStats(adc3_data, adc3_count, "ADC3", 3);
    Serial.println();
}

void printChannelStats(Sample data[][MIN_SAMPLES_REQUIRED + 50], size_t count[], const char* name, size_t channels) {
    const char* ch_names[] = {"LEFT_CENTER/LEFT_TOP", "RIGHT_CENTER/RIGHT_TOP", "RIGHT_BOTTOM/LEFT_BOTTOM"};

    Serial.print(name);
    Serial.println(" Statistics:");

    for (size_t ch = 0; ch < channels; ch++) {
        Serial.print("  Ch");
        Serial.print(ch);
        Serial.print(" (");
        Serial.print(ch_names[ch]);
        Serial.print("): ");

        if (count[ch] >= MIN_SAMPLES_REQUIRED) {
            // Calcular estadísticas
            uint32_t sum = 0;
            Sample min_val = data[ch][0];
            Sample max_val = data[ch][0];

            for (size_t i = 0; i < count[ch]; i++) {
                sum += data[ch][i];
                if (data[ch][i] < min_val) min_val = data[ch][i];
                if (data[ch][i] > max_val) max_val = data[ch][i];
            }

            float avg = (float)sum / count[ch];

            // Filtro IIR simple
            float filtered = data[ch][0];
            float alpha = 0.1;
            for (size_t i = 1; i < count[ch]; i++) {
                filtered = alpha * data[ch][i] + (1.0 - alpha) * filtered;
            }

            Serial.print(count[ch]);
            Serial.print(" samples, Avg=");
            Serial.print(avg, 1);
            Serial.print(", Filtered=");
            Serial.print(filtered, 1);
            Serial.print(", Range=[");
            Serial.print(min_val);
            Serial.print("-");
            Serial.print(max_val);
            Serial.println("] ✓");
        } else {
            Serial.print("ONLY ");
            Serial.print(count[ch]);
            Serial.println(" samples - INSUFFICIENT ✗");
        }
    }
}
