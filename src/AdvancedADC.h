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

typedef enum {
    AN_ADC_SAMPLETIME_1_5 = ADC_SAMPLETIME_1CYCLE_5,
    AN_ADC_SAMPLETIME_2_5 = ADC_SAMPLETIME_2CYCLES_5,
    AN_ADC_SAMPLETIME_8_5 = ADC_SAMPLETIME_8CYCLES_5,
    AN_ADC_SAMPLETIME_16_5 = ADC_SAMPLETIME_16CYCLES_5,
    AN_ADC_SAMPLETIME_32_5 = ADC_SAMPLETIME_32CYCLES_5,
    AN_ADC_SAMPLETIME_64_5 = ADC_SAMPLETIME_64CYCLES_5,
    AN_ADC_SAMPLETIME_387_5 = ADC_SAMPLETIME_387CYCLES_5,
    AN_ADC_SAMPLETIME_810_5 = ADC_SAMPLETIME_810CYCLES_5,
} adc_sample_time_t;

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
    ~AdvancedADC();
    int id();
    bool available();
    SampleBuffer read();
    int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
              size_t n_buffers, bool start = true, adc_sample_time_t sample_time = AN_ADC_SAMPLETIME_8_5);
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
    int start(uint32_t sample_rate);
    int stop();
    void clear();
    size_t channels();
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

class AdvancedADCDual {
  private:
    AdvancedADC &adc1;
    AdvancedADC &adc2;
    size_t n_channels;

  public:
    AdvancedADCDual(AdvancedADC &adc1_in, AdvancedADC &adc2_in) : adc1(adc1_in), adc2(adc2_in), n_channels(0) {
    }
    ~AdvancedADCDual();
    int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
              size_t n_buffers, adc_sample_time_t sample_time = AN_ADC_SAMPLETIME_8_5);
    int stop();
};

#endif // __ADVANCED_ADC_H__
