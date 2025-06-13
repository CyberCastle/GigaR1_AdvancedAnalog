/*
  This file is part of the GigaR1_AdvancedAnalog library.
  Copyright (c) 2023 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __ADVANCED_ADC_H__
#define __ADVANCED_ADC_H__

#include "AdvancedAnalog.h"

struct adc_descr_t;

/**
 * @brief ADC sampling time enumeration
 *
 * Defines the available sampling times for ADC conversion in cycles.
 * Longer sampling times provide more accurate results but reduce maximum sampling rate.
 */
typedef enum {
    AN_ADC_SAMPLETIME_1_5 = ADC_SAMPLETIME_1CYCLE_5,      ///< 1.5 cycles sampling time
    AN_ADC_SAMPLETIME_2_5 = ADC_SAMPLETIME_2CYCLES_5,     ///< 2.5 cycles sampling time
    AN_ADC_SAMPLETIME_8_5 = ADC_SAMPLETIME_8CYCLES_5,     ///< 8.5 cycles sampling time (default)
    AN_ADC_SAMPLETIME_16_5 = ADC_SAMPLETIME_16CYCLES_5,   ///< 16.5 cycles sampling time
    AN_ADC_SAMPLETIME_32_5 = ADC_SAMPLETIME_32CYCLES_5,   ///< 32.5 cycles sampling time
    AN_ADC_SAMPLETIME_64_5 = ADC_SAMPLETIME_64CYCLES_5,   ///< 64.5 cycles sampling time
    AN_ADC_SAMPLETIME_387_5 = ADC_SAMPLETIME_387CYCLES_5, ///< 387.5 cycles sampling time
    AN_ADC_SAMPLETIME_810_5 = ADC_SAMPLETIME_810CYCLES_5, ///< 810.5 cycles sampling time
} adc_sample_time_t;

/**
 * @brief Advanced ADC class for high-performance analog sampling
 *
 * This class provides advanced ADC functionality for the STM32H7 microcontroller,
 * supporting multiple channels, high sampling rates, and buffered data acquisition.
 * It can utilize any of the three available ADC instances (ADC1, ADC2, ADC3) and
 * supports both single-channel and multi-channel sampling configurations.
 */
class AdvancedADC {
  private:
    size_t n_channels;
    adc_descr_t *descr;
    int adc_index;
    PinName adc_pins[AN_MAX_ADC_CHANNELS];

  public:
    /**
     * @brief Constructor for AdvancedADC with ADC number specification
     * @param adc_num ADC number to use (1, 2, or 3)
     * @param p0 First analog pin to be used
     * @param args Additional analog pins (up to 16 total channels)
     *
     * Creates an AdvancedADC object with a specific ADC unit and channels.
     * The ADC number must be specified along with the channels to use.
     */
    template <typename... T>
    AdvancedADC(int adc_num, PinName p0, T... args) : n_channels(0), descr(nullptr), adc_index(-1) {
        static_assert(sizeof...(args) < AN_MAX_ADC_CHANNELS,
                      "A maximum of 16 channels can be sampled successively.");

        // Set the ADC index based on the provided ADC number
        if (adc_num >= 1 && adc_num <= 3) {
            adc_index = adc_num - 1;
        } else {
            adc_index = -1;
        }

        // Initialize the array elements
        for (size_t i = 0; i < AN_MAX_ADC_CHANNELS; ++i) {
            adc_pins[i] = NC;
        }

        for (auto p : {p0, args...}) {
            adc_pins[n_channels++] = p;
        }
    }
    /**
     * @brief Default constructor for AdvancedADC
     *
     * Creates an AdvancedADC object without specifying ADC or channels.
     * ADC and channels must be configured later using setADC() and begin() methods.
     */
    AdvancedADC() : n_channels(0), descr(nullptr), adc_index(-1) {
        // Initialize the array elements
        for (size_t i = 0; i < AN_MAX_ADC_CHANNELS; ++i) {
            adc_pins[i] = NC;
        }
    }

    /**
     * @brief Destructor for AdvancedADC
     *
     * Cleans up and releases resources used by the AdvancedADC object.
     */
    ~AdvancedADC();

    /**
     * @brief Get the ADC instance ID
     * @return ADC instance ID (0-2 for ADC1-ADC3, or -1 if not set)
     *
     * Returns the zero-based index of the ADC instance being used.
     */
    int id();

    /**
     * @brief Check if sample data is available
     * @return true if sample data is available to read, false otherwise
     *
     * Returns true when there is at least one complete sample buffer ready to be read.
     */
    bool available();

    /**
     * @brief Read a sample buffer
     * @return SampleBuffer containing the sampled data
     *
     * Reads and returns the next available sample buffer from the ADC queue.
     * Call available() first to check if data is ready.
     */
    SampleBuffer read();

    /**
     * @brief Initialize and configure the ADC
     * @param resolution ADC resolution (8, 10, 12, 14, or 16 bits)
     * @param sample_rate Sampling rate in Hz
     * @param n_samples Number of samples per buffer per channel
     * @param n_buffers Number of buffers in the queue
     * @param start Whether to start sampling immediately (default: true)
     * @param sample_time ADC sampling time in cycles (default: AN_ADC_SAMPLETIME_8_5)
     * @return 0 on success, negative value on error
     *
     * Configures the ADC with the specified parameters and optionally starts sampling.
     * To reconfigure, call stop() first.
     */
    int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
              size_t n_buffers, bool start = true, adc_sample_time_t sample_time = AN_ADC_SAMPLETIME_8_5);

    /**
     * @brief Initialize and configure the ADC with dynamic pin configuration
     * @param resolution ADC resolution (8, 10, 12, 14, or 16 bits)
     * @param sample_rate Sampling rate in Hz
     * @param n_samples Number of samples per buffer per channel
     * @param n_buffers Number of buffers in the queue
     * @param n_pins Number of pins in the pins array
     * @param pins Array of PinName values specifying which pins to use
     * @param start Whether to start sampling immediately (default: true)
     * @param sample_time ADC sampling time in cycles (default: AN_ADC_SAMPLETIME_8_5)
     * @return 0 on success, negative value on error
     *
     * Configures the ADC with the specified parameters and pin array.
     * This version allows dynamic configuration of pins at runtime.
     */
    int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
              size_t n_buffers, size_t n_pins, PinName *pins, bool start = true,
              adc_sample_time_t sample_time = AN_ADC_SAMPLETIME_8_5) {
        if (n_pins > AN_MAX_ADC_CHANNELS) {
            n_pins = AN_MAX_ADC_CHANNELS;
        }
        for (size_t i = 0; i < n_pins; ++i) {
            adc_pins[i] = pins[i];
        }

        n_channels = n_pins;
        return begin(resolution, sample_rate, n_samples, n_buffers, start, sample_time);
    }

    /**
     * @brief Start ADC sampling
     * @param sample_rate Sampling rate in Hz
     * @return 0 on success, negative value on error
     *
     * Starts the ADC sampling at the specified rate. The ADC must be
     * initialized with begin() before calling this method.
     */
    int start(uint32_t sample_rate);

    /**
     * @brief Stop ADC sampling
     * @return 0 on success, negative value on error
     *
     * Stops the ADC sampling but preserves the configuration.
     * Call start() to resume sampling with the same configuration.
     */
    int stop();

    /**
     * @brief End ADC operation and release all resources
     * @return 0 on success, negative value on error
     *
     * Stops the ADC sampling and releases all associated resources
     * including DMA buffers and memory pools. Call this method to
     * completely free resources before reconfiguring with begin().
     */
    int end();

    /**
     * @brief Clear all sample buffers
     *
     * Clears all pending sample buffers from the queue.
     * Useful for discarding old data before starting fresh sampling.
     */
    void clear();

    /**
     * @brief Get the number of configured channels
     * @return Number of channels configured for this ADC instance
     *
     * Returns the total number of analog channels that have been configured
     * for sampling on this ADC instance.
     */
    size_t channels();

    /**
     * @brief Read a single sample from a specific channel
     * @param channel Channel number to read from (default: 0)
     * @return Sample value from the specified channel
     *
     * Reads a single sample value from the specified channel. This is a
     * convenience method for debugging and simple applications.
     */
    Sample analogRead(size_t channel = 0);
    /**
     * @brief Set the ADC number to use
     * @param adc ADC number (1, 2, or 3)
     *
     * Sets which ADC unit to use for analog-to-digital conversion.
     * Valid values are 1, 2, or 3 corresponding to ADC1, ADC2, and ADC3.
     */
    void setADC(int adc) {
        if (adc >= 1 && adc <= 3) {
            adc_index = adc - 1;
        } else {
            adc_index = -1;
        }
    }
};

/**
 * @brief Dual ADC class for simultaneous sampling on two ADC instances
 *
 * This class allows simultaneous sampling using two separate ADC instances,
 * enabling higher throughput and more complex sampling scenarios.
 */
class AdvancedADCDual {
  private:
    AdvancedADC &adc1;
    AdvancedADC &adc2;
    size_t n_channels;

  public:
    /**
     * @brief Constructor for AdvancedADCDual
     * @param adc1_in Reference to the first ADC instance
     * @param adc2_in Reference to the second ADC instance
     *
     * Creates a dual ADC object that can coordinate sampling between
     * two separate AdvancedADC instances.
     */
    AdvancedADCDual(AdvancedADC &adc1_in, AdvancedADC &adc2_in) : adc1(adc1_in), adc2(adc2_in), n_channels(0) {
    }

    /**
     * @brief Destructor for AdvancedADCDual
     *
     * Cleans up and releases resources used by the AdvancedADCDual object.
     */
    ~AdvancedADCDual();

    /**
     * @brief Initialize and configure both ADC instances for dual mode
     * @param resolution ADC resolution (8, 10, 12, 14, or 16 bits)
     * @param sample_rate Sampling rate in Hz
     * @param n_samples Number of samples per buffer per channel
     * @param n_buffers Number of buffers in the queue
     * @param sample_time ADC sampling time in cycles (default: AN_ADC_SAMPLETIME_8_5)
     * @return 0 on success, negative value on error
     *
     * Configures both ADC instances for synchronized dual-mode operation.
     * Both ADCs will sample simultaneously at the specified rate.
     */
    int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
              size_t n_buffers, adc_sample_time_t sample_time = AN_ADC_SAMPLETIME_8_5);

    /**
     * @brief Stop both ADC instances
     * @return 0 on success, negative value on error
     *
     * Stops sampling on both ADC instances but preserves the configuration.
     * Call begin() again to resume sampling with the same configuration.
     */
    int stop();

    /**
     * @brief End both ADC instances and release all resources
     * @return 0 on success, negative value on error
     *
     * Stops sampling on both ADC instances and releases all associated
     * resources. Call this method to completely free resources before
     * reconfiguring with begin().
     */
    int end();
};

#endif // __ADVANCED_ADC_H__
