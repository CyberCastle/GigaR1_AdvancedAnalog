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

#include "AdvancedADC.h"
#include "Arduino.h"
#include "HALConfig.h"

#define ADC_NP ((ADCName)NC)
#define ADC_PIN_ALT_MASK (uint32_t)(ALT0 | ALT1)

struct adc_descr_t {
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t tim_trig;
    DMAPool<Sample> *pool;
    DMABuffer<Sample> *dmabuf[2];
};

static uint32_t adc_pin_alt[3] = {0, ALT0, ALT1};

static adc_descr_t adc_descr_all[3] = {
    {{ADC1}, {DMA1_Stream1, {DMA_REQUEST_ADC1}}, DMA1_Stream1_IRQn, {TIM1}, ADC_EXTERNALTRIG_T1_TRGO, nullptr, {nullptr, nullptr}},
    {{ADC2}, {DMA1_Stream2, {DMA_REQUEST_ADC2}}, DMA1_Stream2_IRQn, {TIM2}, ADC_EXTERNALTRIG_T2_TRGO, nullptr, {nullptr, nullptr}},
    {{ADC3}, {DMA1_Stream3, {DMA_REQUEST_ADC3}}, DMA1_Stream3_IRQn, {TIM3}, ADC_EXTERNALTRIG_T3_TRGO, nullptr, {nullptr, nullptr}},
};

static uint32_t ADC_RES_LUT[] = {
    ADC_RESOLUTION_8B,
    ADC_RESOLUTION_10B,
    ADC_RESOLUTION_12B,
    ADC_RESOLUTION_14B,
    ADC_RESOLUTION_16B,
};

extern "C" {

void DMA1_Stream1_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[0].adc.DMA_Handle);
}

void DMA1_Stream2_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[1].adc.DMA_Handle);
}

void DMA1_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[2].adc.DMA_Handle);
}

} // extern C

static adc_descr_t *adc_descr_get(ADC_TypeDef *adc) {
    if (adc == ADC1) {
        return &adc_descr_all[0];
    } else if (adc == ADC2) {
        return &adc_descr_all[1];
    } else if (adc == ADC3) {
        return &adc_descr_all[2];
    }
    return NULL;
}

static void adc_descr_stop(adc_descr_t *descr) {
    if (descr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_ADC_Stop_DMA(&descr->adc);
    }
}

static void adc_descr_deinit(adc_descr_t *descr) {
    if (descr) {
        // Stop conversion first
        adc_descr_stop(descr);

        // Release DMA buffers
        for (size_t i = 0; i < AN_ARRAY_SIZE(descr->dmabuf); i++) {
            if (descr->dmabuf[i]) {
                descr->dmabuf[i]->release();
                descr->dmabuf[i] = nullptr;
            }
        }

        // Deallocate buffer pool
        if (descr->pool) {
            delete descr->pool;
            descr->pool = nullptr;
        }
    }
}

int AdvancedADC::id() {
    if (descr) {
        ADC_TypeDef *adc = descr->adc.Instance;
        if (adc == ADC1) {
            return 1;
        } else if (adc == ADC2) {
            return 2;
        } else if (adc == ADC3) {
            return 3;
        }
    }
    return -1;
}

bool AdvancedADC::available() {
    if (descr != nullptr) {
        return descr->pool->readable();
    }
    return false;
}

SampleBuffer AdvancedADC::read() {
    static DMABuffer<Sample> NULLBUF;
    if (descr != nullptr) {
        while (!available()) {
            __WFI();
        }
        return *descr->pool->alloc(DMA_BUFFER_READ);
    }
    return NULLBUF;
}

bool AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
                        size_t n_buffers, bool start, adc_sample_time_t sample_time) {

    ADCName instance = ADC_NP;
    // Sanity checks.
    if (resolution >= AN_ARRAY_SIZE(ADC_RES_LUT) || (descr && descr->pool)) {
        return false;
    }

    // Clear ALTx pin.
    for (size_t i = 0; i < n_channels; i++) {
        // Clear the ALTx bits from the pin name.
        // This is necessary to ensure that the pin is correctly mapped to the ADC.
        // This is done to avoid issues with alternate function pins.
        // For example, if the pin is PA_0C_ALT0, we want to use PA_0C without the ALT0.
        adc_pins[i] = (PinName)(adc_pins[i] & ~(ADC_PIN_ALT_MASK));
    }

    if (adc_index >= 0 && adc_index < (int)AN_ARRAY_SIZE(adc_descr_all)) {
        descr = &adc_descr_all[adc_index];
        if (descr->pool != nullptr) {
            return false;
        }

        for (size_t i = 0; instance == ADC_NP && i < AN_ARRAY_SIZE(adc_pin_alt); i++) {
            PinName pin = (PinName)(adc_pins[0] | adc_pin_alt[i]);
            if ((PinName)pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                break;
            }

            if (descr->adc.Instance == ((ADC_TypeDef *)pinmap_peripheral(pin, PinMap_ADC))) {
                instance = (ADCName)pinmap_peripheral(pin, PinMap_ADC);
                adc_pins[0] = pin;
            }
        }
    } else {
        // Find an ADC that can be used with these set of pins/channels.
        for (size_t i = 0; instance == ADC_NP && i < AN_ARRAY_SIZE(adc_pin_alt); i++) {
            // Calculate alternate function pin.
            PinName pin = (PinName)(adc_pins[0] | adc_pin_alt[i]); // First pin decides the ADC.

            // Check if pin is mapped.
            if ((PinName)pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                break;
            }

            // Find the first free ADC according to the available ADCs on pin.
            for (size_t j = 0; instance == ADC_NP && j < AN_ARRAY_SIZE(adc_descr_all); j++) {
                descr = &adc_descr_all[j];
                if (descr->pool == nullptr) {
                    ADCName tmp_instance = (ADCName)pinmap_peripheral(pin, PinMap_ADC);
                    if (descr->adc.Instance == ((ADC_TypeDef *)tmp_instance)) {
                        instance = tmp_instance;
                        adc_pins[0] = pin;
                    }
                }
            }
        }
    }

    if (instance == ADC_NP) {
        // Couldn't find a free ADC/descriptor.
        descr = nullptr;
        return false;
    }

    // Configure ADC pins.
    pinmap_pinout(adc_pins[0], PinMap_ADC);

    uint8_t ch_init = 1;
    for (size_t i = 1; i < n_channels; i++) {
        for (size_t j = 0; j < AN_ARRAY_SIZE(adc_pin_alt); j++) {
            // Calculate alternate function pin.
            PinName pin = (PinName)(adc_pins[i] | adc_pin_alt[j]);
            // Check if pin is mapped.
            if ((PinName)pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                break;
            }
            // Check if pin is connected to the selected ADC.
            if (instance == (ADCName)pinmap_peripheral(pin, PinMap_ADC)) {
                pinmap_pinout(pin, PinMap_ADC);
                adc_pins[i] = pin;
                ch_init++;
                break;
            }
        }
    }

    // All channels must share the same instance; if not, bail out.
    if (ch_init < n_channels) {
        return false;
    }

    // Allocate DMA buffer pool.
    descr->pool = new DMAPool<Sample>(n_samples, n_channels, n_buffers);
    if (descr->pool == nullptr) {
        return false;
    }

    // Allocate the two DMA buffers used for double buffering.
    descr->dmabuf[0] = descr->pool->alloc(DMA_BUFFER_WRITE);
    descr->dmabuf[1] = descr->pool->alloc(DMA_BUFFER_WRITE);

    // Init and config DMA.
    if (!hal_dma_config(&descr->dma, descr->dma_irqn, DMA_PERIPH_TO_MEMORY)) {
        return false;
    }

    // Init and config ADC.
    if (!hal_adc_config(&descr->adc, ADC_RES_LUT[resolution], descr->tim_trig, adc_pins, n_channels, sample_time)) {
        return false;
    }

    // Link DMA handle to ADC handle.
    __HAL_LINKDMA(&descr->adc, DMA_Handle, descr->dma);

    if (start) {
        return this->start(sample_rate);
    }

    return true;
}

bool AdvancedADC::start(uint32_t sample_rate) {
    if (descr == nullptr || descr->pool == nullptr) {
        // ADC not initialized, call begin() first
        return false;
    }

    // Stop any ongoing conversion
    adc_descr_stop(descr);

    // Restart ADC with DMA
    if (HAL_ADC_Start_DMA(&descr->adc, (uint32_t *)descr->dmabuf[0]->data(), descr->dmabuf[0]->size()) != HAL_OK) {
        return false;
    }

    // Re/enable DMA double buffer mode
    HAL_NVIC_DisableIRQ(descr->dma_irqn);
    hal_dma_enable_dbm(&descr->dma, descr->dmabuf[0]->data(), descr->dmabuf[1]->data());
    HAL_NVIC_EnableIRQ(descr->dma_irqn);

    // Initialize and configure the ADC timer
    if (!hal_tim_config(&descr->tim, sample_rate)) {
        return false;
    }

    // Start the ADC timer. Note, if dual ADC mode is enabled,
    // this will also start ADC2.
    if (HAL_TIM_Base_Start(&descr->tim) != HAL_OK) {
        return false;
    }

    return true;
}

bool AdvancedADC::stop() {
    if (descr == nullptr) {
        return false;
    }
    adc_descr_stop(descr);
    return true;
}

bool AdvancedADC::end() {
    if (descr == nullptr) {
        return false;
    }
    adc_descr_deinit(descr);
    descr = nullptr;
    return true;
}

void AdvancedADC::clear() {
    if (descr && descr->pool) {
        descr->pool->flush();
    }
}

Sample AdvancedADC::analogRead(size_t channel) {
    if (descr == nullptr) {
        Serial.println("AdvancedADC not initialized");
        return 0;
    }

    if (channel >= n_channels) {
        Serial.println("Invalid channel");
        return 0;
    }

    while (!available()) {
        // Wait for the DMA buffer to be ready
        __WFI();
    }

    SampleBuffer buf = read();
    Sample value = buf[channel];

    Serial.print("analogRead value: ");
    Serial.println(value);
    Serial.print("timestamp: ");
    Serial.println(buf.timestamp());

    if (buf.get_flags(DMA_BUFFER_DISCONT)) {
        Serial.println("DMA buffer discontinuity detected - clearing queue");
        clear();
    }

    buf.release();
    return value;
}

size_t AdvancedADC::channels() {
    return n_channels;
}

AdvancedADC::~AdvancedADC() {
    adc_descr_deinit(descr);
}

bool AdvancedADCDual::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
                            size_t n_buffers, adc_sample_time_t sample_time) {
    // The two ADCs must have the same number of channels.
    if (adc1.channels() != adc2.channels()) {
        return false;
    }

    // Configure the ADCs.
    if (!adc1.begin(resolution, sample_rate, n_samples, n_buffers, false, sample_time)) {
        return false;
    }

    if (!adc2.begin(resolution, sample_rate, n_samples, n_buffers, false, sample_time)) {
        adc1.stop();
        return false;
    }

    // Note only ADC1 (master) and ADC2 can be used in dual mode.
    if (adc1.id() != 1 || adc2.id() != 2) {
        adc1.stop();
        adc2.stop();
        return false;
    }

    // Enable dual ADC mode.
    hal_adc_enable_dual_mode(true);

    // Start ADC1, note ADC2 is also automatically started.
    return adc1.start(sample_rate);
}

bool AdvancedADCDual::stop() {
    adc1.stop();
    adc2.stop();
    // Disable dual mode.
    hal_adc_enable_dual_mode(false);
    return true;
}

bool AdvancedADCDual::end() {
    stop();
    adc1.end();
    adc2.end();
    return true;
}

AdvancedADCDual::~AdvancedADCDual() {
    end();
}

extern "C" {

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *descr = adc_descr_get(adc->Instance);
    // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
    size_t ct = !hal_dma_get_ct(&descr->dma);

    // Timestamp the buffer.
    descr->dmabuf[ct]->timestamp(us_ticker_read());

    if (descr->pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmabuf[ct]->invalidate();
        // Move current DMA buffer to ready queue.
        descr->dmabuf[ct]->release();
        // Allocate a new free buffer.
        descr->dmabuf[ct] = descr->pool->alloc(DMA_BUFFER_WRITE);
        // Currently, all multi-channel buffers are interleaved.
        if (descr->dmabuf[ct]->channels() > 1) {
            descr->dmabuf[ct]->set_flags(DMA_BUFFER_INTRLVD);
        }
    } else {
        descr->dmabuf[ct]->set_flags(DMA_BUFFER_DISCONT);
    }

    // Update the next DMA target pointer.
    // NOTE: If the pool was empty, the same buffer is reused.
    hal_dma_update_memory(&descr->dma, descr->dmabuf[ct]->data());
}

} // extern C
