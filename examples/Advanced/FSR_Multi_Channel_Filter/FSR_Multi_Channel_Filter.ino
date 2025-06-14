/*
  Ejemplo para capturar 100+ muestras por canal entre ciclos start/stop
  para aplicar filtros digitales en sensores FSR

  Configuración:
  - ADC1: 3 canales (LEFT_CENTER, RIGHT_CENTER, RIGHT_BOTTOM)
  - ADC3: 3 canales (LEFT_TOP, RIGHT_TOP, LEFT_BOTTOM)
  - Mínimo 100 muestras por canal por ciclo
*/

#include <AdvancedADC.h>

// FSRs addresses
#define ANALOG_PORT_FSR_LEFT_TOP PC_2C     // Default Pin A8
#define ANALOG_PORT_FSR_LEFT_CENTER PA_0   // Default Pin A7
#define ANALOG_PORT_FSR_LEFT_BOTTOM PC_0   // Default Pin A6
#define ANALOG_PORT_FSR_RIGHT_TOP PC_3C    // Default Pin A9
#define ANALOG_PORT_FSR_RIGHT_CENTER PA_1C // Default Pin A10
#define ANALOG_PORT_FSR_RIGHT_BOTTOM PA_0C // Default Pin A11

// Configuración de ADCs
AdvancedADC adc1(1, ANALOG_PORT_FSR_LEFT_CENTER, ANALOG_PORT_FSR_RIGHT_CENTER, ANALOG_PORT_FSR_RIGHT_BOTTOM);
AdvancedADC adc3(3, ANALOG_PORT_FSR_LEFT_TOP, ANALOG_PORT_FSR_RIGHT_TOP, ANALOG_PORT_FSR_LEFT_BOTTOM);

// Configuración de muestreo
const uint32_t SAMPLE_RATE = 1000;        // 1kHz sample rate
const size_t SAMPLES_PER_BUFFER = 50;     // Muestras por buffer por canal
const size_t NUM_BUFFERS = 6;             // Número de buffers en pool
const size_t MIN_SAMPLES_PER_CHANNEL = 100; // Mínimo requerido por canal

// Variables para almacenar muestras
struct ChannelData {
    Sample samples[MIN_SAMPLES_PER_CHANNEL * 2]; // Buffer extra por seguridad
    size_t count;
    float filtered_value;
    float prev_filtered_value;
};

// Datos para cada ADC
ChannelData adc1_channels[3]; // ADC1 tiene 3 canales
ChannelData adc3_channels[3]; // ADC3 tiene 3 canales

// Estados de captura
enum CaptureState {
    IDLE,
    CAPTURING,
    PROCESSING
};

CaptureState capture_state = IDLE;
unsigned long capture_start_time = 0;
const unsigned long CAPTURE_DURATION_MS = 150; // Duración de captura en ms

// Declaraciones de funciones
void initializeChannelData();
void startCapture();
void stopCapture();
void readRemainingBuffers();
void collectSamples(AdvancedADC &adc, ChannelData *channels, size_t num_channels);
void processDigitalFilters();
void processADCChannels(ChannelData *channels, size_t num_channels, const char* adc_name);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("FSR Multi-Channel Digital Filter Example");
    Serial.println("=======================================");

    // Inicializar estructuras de datos
    initializeChannelData();

    // Configurar ADC1 (3 canales)
    if (!adc1.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: Failed to initialize ADC1!");
        while (1) delay(100);
    }
    Serial.println("ADC1 initialized (3 channels)");

    // Configurar ADC3 (3 canales)
    if (!adc3.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false)) {
        Serial.println("ERROR: Failed to initialize ADC3!");
        while (1) delay(100);
    }
    Serial.println("ADC3 initialized (3 channels)");

    Serial.println("System ready. Starting capture cycles...");
    Serial.println();
}

void initializeChannelData() {
    // Inicializar datos de canales para ADC1
    for (int i = 0; i < 3; i++) {
        adc1_channels[i].count = 0;
        adc1_channels[i].filtered_value = 0.0;
        adc1_channels[i].prev_filtered_value = 0.0;
    }

    // Inicializar datos de canales para ADC3
    for (int i = 0; i < 3; i++) {
        adc3_channels[i].count = 0;
        adc3_channels[i].filtered_value = 0.0;
        adc3_channels[i].prev_filtered_value = 0.0;
    }
}

void startCapture() {
    Serial.println("Starting capture cycle...");

    // Limpiar buffers previos
    adc1.clear();
    adc3.clear();

    // Resetear contadores
    for (int i = 0; i < 3; i++) {
        adc1_channels[i].count = 0;
        adc3_channels[i].count = 0;
    }

    // Iniciar ambos ADCs
    if (!adc1.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC1!");
        return;
    }

    if (!adc3.start(SAMPLE_RATE)) {
        Serial.println("ERROR: Failed to start ADC3!");
        adc1.stop();
        return;
    }

    capture_state = CAPTURING;
    capture_start_time = millis();
}

void stopCapture() {
    Serial.println("Stopping capture...");

    // Detener ambos ADCs
    adc1.stop();
    adc3.stop();

    // Leer buffers restantes
    readRemainingBuffers();

    capture_state = PROCESSING;
}

void readRemainingBuffers() {
    // Leer buffers restantes de ADC1
    while (adc1.available()) {
        collectSamples(adc1, adc1_channels, 3);
    }

    // Leer buffers restantes de ADC3
    while (adc3.available()) {
        collectSamples(adc3, adc3_channels, 3);
    }
}

void collectSamples(AdvancedADC &adc, ChannelData *channels, size_t num_channels) {
    if (!adc.available()) return;

    SampleBuffer buf = adc.read();
    size_t buffer_size = buf.size();

    // Las muestras están intercaladas por canal
    for (size_t sample = 0; sample < buffer_size / num_channels; sample++) {
        for (size_t channel = 0; channel < num_channels; channel++) {
            if (channels[channel].count < MIN_SAMPLES_PER_CHANNEL * 2) {
                size_t index = sample * num_channels + channel;
                if (index < buffer_size) {
                    channels[channel].samples[channels[channel].count] = buf[index];
                    channels[channel].count++;
                }
            }
        }
    }

    buf.release();
}

void processDigitalFilters() {
    Serial.println("Processing digital filters...");

    // Procesar ADC1
    Serial.println("ADC1 Results:");
    processADCChannels(adc1_channels, 3, "ADC1");

    // Procesar ADC3
    Serial.println("ADC3 Results:");
    processADCChannels(adc3_channels, 3, "ADC3");

    Serial.println();
}

void processADCChannels(ChannelData *channels, size_t num_channels, const char* adc_name) {
    const char* channel_names[] = {"Ch0", "Ch1", "Ch2"};

    for (size_t ch = 0; ch < num_channels; ch++) {
        if (channels[ch].count >= MIN_SAMPLES_PER_CHANNEL) {
            // Aplicar filtro paso bajo simple (media móvil)
            float sum = 0.0;
            for (size_t i = 0; i < channels[ch].count; i++) {
                sum += channels[ch].samples[i];
            }
            float average = sum / channels[ch].count;

            // Filtro paso bajo IIR simple (alpha = 0.3)
            float alpha = 0.3;
            channels[ch].filtered_value = alpha * average + (1.0 - alpha) * channels[ch].prev_filtered_value;
            channels[ch].prev_filtered_value = channels[ch].filtered_value;

            // Calcular estadísticas
            float min_val = channels[ch].samples[0];
            float max_val = channels[ch].samples[0];
            for (size_t i = 1; i < channels[ch].count; i++) {
                if (channels[ch].samples[i] < min_val) min_val = channels[ch].samples[i];
                if (channels[ch].samples[i] > max_val) max_val = channels[ch].samples[i];
            }

            Serial.print("  ");
            Serial.print(adc_name);
            Serial.print("-");
            Serial.print(channel_names[ch]);
            Serial.print(": Samples=");
            Serial.print(channels[ch].count);
            Serial.print(", Avg=");
            Serial.print(average, 1);
            Serial.print(", Filtered=");
            Serial.print(channels[ch].filtered_value, 1);
            Serial.print(", Min=");
            Serial.print(min_val, 1);
            Serial.print(", Max=");
            Serial.println(max_val, 1);
        } else {
            Serial.print("  ");
            Serial.print(adc_name);
            Serial.print("-");
            Serial.print(channel_names[ch]);
            Serial.print(": WARNING - Only ");
            Serial.print(channels[ch].count);
            Serial.println(" samples collected!");
        }
    }
}

void loop() {
    switch (capture_state) {
        case IDLE:
            // Esperar un poco antes del próximo ciclo
            delay(1000);
            startCapture();
            break;

        case CAPTURING:
            // Leer muestras mientras capturamos
            if (adc1.available()) {
                collectSamples(adc1, adc1_channels, 3);
            }

            if (adc3.available()) {
                collectSamples(adc3, adc3_channels, 3);
            }

            // Verificar si hemos capturado suficiente tiempo o suficientes muestras
            if (millis() - capture_start_time >= CAPTURE_DURATION_MS ||
                (adc1_channels[0].count >= MIN_SAMPLES_PER_CHANNEL &&
                 adc3_channels[0].count >= MIN_SAMPLES_PER_CHANNEL)) {
                stopCapture();
            }
            break;

        case PROCESSING:
            processDigitalFilters();
            capture_state = IDLE;
            break;
    }
}
