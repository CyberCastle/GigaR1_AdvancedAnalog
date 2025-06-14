# Obtener 100+ Muestras por Canal en Ciclos Start/Stop

Este documento explica cómo implementar la captura de 100+ muestras por canal entre ciclos start/stop para aplicar filtros digitales a sensores FSR.

## Configuración de Hardware

```cpp
// Configuración de FSRs según tu definición
AdvancedADC adc1(1, ANALOG_PORT_FSR_LEFT_CENTER, ANALOG_PORT_FSR_RIGHT_CENTER, ANALOG_PORT_FSR_RIGHT_BOTTOM);
AdvancedADC adc3(3, ANALOG_PORT_FSR_LEFT_TOP, ANALOG_PORT_FSR_RIGHT_TOP, ANALOG_PORT_FSR_LEFT_BOTTOM);
```

- **ADC1**: 3 canales (LEFT_CENTER, RIGHT_CENTER, RIGHT_BOTTOM)
- **ADC3**: 3 canales (LEFT_TOP, RIGHT_TOP, LEFT_BOTTOM)

## Parámetros Clave de Configuración

```cpp
const uint32_t SAMPLE_RATE = 2000;        // 2kHz
const size_t SAMPLES_PER_BUFFER = 50;     // 50 muestras por buffer por canal
const size_t NUM_BUFFERS = 8;             // 8 buffers en el pool
const size_t MIN_SAMPLES_REQUIRED = 100;  // Mínimo requerido
```

### Cálculo de Tiempo de Captura

Para obtener 100 muestras por canal a 2kHz:
- Tiempo mínimo = 100 muestras ÷ 2000 Hz = **50ms**
- Con margen de seguridad: **~100ms** de captura

### Cálculo de Buffers Necesarios

- Cada buffer contiene: 50 muestras × 3 canales = 150 muestras totales
- Para 100 muestras por canal necesitas: 100 ÷ 50 = **2 buffers mínimo**
- Con 8 buffers tienes suficiente margen para latencia de procesamiento

## Flujo de Trabajo Implementado

### 1. Inicialización (una sola vez)
```cpp
adc1.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false);
adc3.begin(AN_RESOLUTION_12, SAMPLE_RATE, SAMPLES_PER_BUFFER, NUM_BUFFERS, false);
```

### 2. Ciclo Start/Stop para Cada Medición

```cpp
// FASE 1: Iniciar conversión
adc1.start(SAMPLE_RATE);
adc3.start(SAMPLE_RATE);

// FASE 2: Recolectar muestras
while (!enough_samples) {
    // Leer buffers de ADC1
    while (adc1.available()) {
        SampleBuffer buf = adc1.read();
        collectSamplesFromBuffer(buf, adc1_samples, adc1_counts, 3);
        buf.release();
    }

    // Leer buffers de ADC3
    while (adc3.available()) {
        SampleBuffer buf = adc3.read();
        collectSamplesFromBuffer(buf, adc3_samples, adc3_counts, 3);
        buf.release();
    }

    // Verificar si tenemos 100+ muestras en todos los canales
    enough_samples = checkMinimumSamples(adc1_counts, adc3_counts);
}

// FASE 3: Detener conversión
adc1.stop();
adc3.stop();

// FASE 4: Recoger buffers restantes
readRemainingBuffers();

// FASE 5: Aplicar filtros digitales
applyDigitalFilters();
```

## Estructura de Datos

### Almacenamiento de Muestras
```cpp
Sample adc1_samples[3][MIN_SAMPLES_REQUIRED + 50]; // +50 buffer extra
Sample adc3_samples[3][MIN_SAMPLES_REQUIRED + 50];
size_t adc1_counts[3] = {0, 0, 0};  // Contador por canal
size_t adc3_counts[3] = {0, 0, 0};
```

### Mapeo de Canales
- **ADC1 Canal 0**: LEFT_CENTER (PA_0)
- **ADC1 Canal 1**: RIGHT_CENTER (PA_1C)
- **ADC1 Canal 2**: RIGHT_BOTTOM (PA_0C)
- **ADC3 Canal 0**: LEFT_TOP (PC_2C)
- **ADC3 Canal 1**: RIGHT_TOP (PC_3C)
- **ADC3 Canal 2**: LEFT_BOTTOM (PC_0)

## Recolección de Muestras Intercaladas

Las muestras en cada buffer están intercaladas por canal:
```
Buffer layout: [Ch0, Ch1, Ch2, Ch0, Ch1, Ch2, Ch0, Ch1, Ch2, ...]
```

La función `collectSamplesFromBuffer()` separa las muestras por canal:

```cpp
void collectSamplesFromBuffer(SampleBuffer &buf, Sample samples[][MIN_SAMPLES_REQUIRED + 50],
                             size_t counts[], size_t num_channels) {
    size_t buffer_size = buf.size();
    size_t samples_per_channel = buffer_size / num_channels;

    for (size_t sample = 0; sample < samples_per_channel; sample++) {
        for (size_t ch = 0; ch < num_channels; ch++) {
            if (counts[ch] < MIN_SAMPLES_REQUIRED + 50) {
                size_t buffer_index = sample * num_channels + ch;
                samples[ch][counts[ch]] = buf[buffer_index];
                counts[ch]++;
            }
        }
    }
}
```

## Filtros Digitales Aplicables

### 1. Filtro de Media Móvil
```cpp
float moving_avg = 0.0;
for (size_t i = 0; i < sample_count; i++) {
    moving_avg += samples[i];
}
moving_avg /= sample_count;
```

### 2. Filtro Paso Bajo IIR
```cpp
float filtered_value = samples[0];
float alpha = 0.1; // Factor de suavizado
for (size_t i = 1; i < sample_count; i++) {
    filtered_value = alpha * samples[i] + (1.0 - alpha) * filtered_value;
}
```

### 3. Filtro de Mediana (para eliminar picos)
```cpp
// Ordenar muestras y tomar la mediana
qsort(samples, sample_count, sizeof(Sample), compare_samples);
float median = samples[sample_count / 2];
```

## Ventajas de esta Implementación

1. **Sin pérdida de muestras**: El double buffering garantiza captura continua
2. **Flexibilidad**: Puedes ajustar el número de muestras según necesidades
3. **Filtrado robusto**: 100+ muestras proporcionan base sólida para filtros
4. **Eficiencia**: Solo usa recursos cuando necesitas medir
5. **Sincronización**: Ambos ADCs capturan simultáneamente

## Consideraciones de Rendimiento

- **Tiempo de captura**: ~100ms para 100 muestras a 2kHz
- **Memoria utilizada**: ~2.4KB para almacenar todas las muestras
- **Frecuencia de medición**: Limitada por tiempo de procesamiento de filtros
- **Resolución temporal**: 0.5ms entre muestras (2kHz)

## Ejemplo de Uso

```cpp
void loop() {
    // Ejecutar ciclo de medición cada 2 segundos
    performCaptureCycle();
    delay(2000);
}
```

Cada ciclo te dará:
- 100+ muestras filtradas por cada uno de los 6 sensores FSR
- Estadísticas (media, min, max, desviación estándar)
- Valores filtrados listos para usar en tu aplicación
