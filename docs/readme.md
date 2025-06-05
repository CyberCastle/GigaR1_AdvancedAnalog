# GigaR1 Advanced Analog Library

[![License](https://img.shields.io/badge/License-LGPLv2.1-blue.svg)](../LICENSE)

The GigaR1 Advanced Analog library enables high performance ADC applications on boards based on the STM32H7 microcontrollers:
- [Arduino GIGA R1 WiFi](https://store.arduino.cc/products/giga-r1-wifi)
- [Arduino Portenta H7](https://store.arduino.cc/products/portenta-h7)

## Features

- Efficient memory management using DMA buffer pools.
- Configurable sample rate, sampling time, resolution, and number of channels.
- Samples are stored in dynamically configurable queues.
- ADC multi-channel acquisition and dual mode support.
- All drivers utilize DMA in double buffer mode.

## Guides

To learn more about using the ADC on the GIGA R1 WiFi, check out the [GIGA Advanced ADC Guide](https://docs.arduino.cc/tutorials/giga-r1-wifi/giga-audio).

This includes examples such as audio playback from USB storage, generate sine waves and more.
## Usage

### ADC

To use this library for ADC applications, you must have a supported Arduino board and include the AdvancedAnalog library in your Arduino sketch. Here is a minimal example:

```cpp
#include <AdvancedADC.h>

AdvancedADC adc1(A0);

void setup() {
    Serial.begin(9600);

    // Initialize ADC with: resolution, sample rate, number of samples per channel, queue depth
    if (!adc1.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start ADC!");
        while (1);
    }
}

void loop() {
    // Check if an ADC measurement is ready
    if (adc1.available()) {
        // Get read buffer
        SampleBuffer buf = adc1.read();

        // Print sample from read buffer
        Serial.println(buf[0]);

        // Release read buffer
        buf.release();
    }
}
```

#### ADC Multichannel (GIGA R1 WiFi)
This library supports concurrent usage of up to **three** ADCs (_ADC1_, _ADC2_ and _ADC3_).
Each ADC instance can handle up to **16** channels.

**Note:** It's important to be aware that certain pins cannot be used across multiple ADCs or cannot share the same ADC.

*Please ensure that you refer to tables below when configuring your project to avoid conflicts in pin assignments.*

Below is a table illustrating the pin mapping for each ADC in **Arduino Giga R1 WiFi**:

| Pin   | ADC1  | ADC2  | ADC3  |
|-------|-------|-------|-------|
| A0    | X     | X      |      |
| A1    | X     | X      |      |
| A2    | X     | X      |      |
| A3    | X     | X      |      |
| A4    | X     | X      |      |
| A5    | X     | X      | X    |
| A6    | X     | X      | X    |
| A7    | X     |        |      |
| A8    |       |        | X    |
| A9    |       |        | X    |
| A10   | X     | X      |      |
| A11   | X     | X      |      |


All pins from `A0` to `A11` are now supported.

You may optionally select which ADC instance to use by calling `setADC()` on the
`AdvancedADC` object **before** `begin()`. If omitted, the library will
automatically pick the ADC associated with the first pin, as long as the rest of
the channels belong to the same instance.

Here is a example for the Arduino GIGA R1 WiFi:

```cpp
#include <AdvancedADC.h>

AdvancedADC adc_a(A0, A1); 
/* Mapped to ADC1 */

AdvancedADC adc_b(A2);
/* Mapped to ADC2, because ADC1 is occupied by A0 and A1 */

void setup() {
...
```

#### ADC Multichannel (Portenta H7)

Below is a table illustrating the pin mapping for each ADC in **Portenta H7**:

| Pin   | ADC1  | ADC2  | ADC3  |
|-------|-------|-------|-------|
| A0    | X     | X     |       |
| A1    | X     | X     |       |
| A2    |       |       | X     |
| A3    |       |       | X     |
| A4    | X     | X     | X     |
| A5    | X     | X     |       |
| A6    | X     | X     |       |
| A7    | X     | X     |       |

Here is an example for the Portenta H7:

```cpp
#include <AdvancedADC.h>

AdvancedADC adc_c(A2, A3, A4); 
/* Mapped to ADC3 */

AdvancedADC adc_d(A5);
/* Mapped to ADC1 */

void setup() {
...
```

## Examples
- **[Advanced](../examples/Advanced):** This folder contains more specific examples showing advanced API configurations.
- **[ADC_Manual_Selection](../examples/Advanced/ADC_Manual_Selection):** Demonstrates manually selecting the ADC instance with `setADC()`.

## API

The API documentation can be found [here](./api.md).

## License

This library is released under the [LGPLv2.1 license](../LICENSE).

