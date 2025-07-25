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

#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__

#include "AdvancedAnalog.h"
#include "Arduino.h"

bool hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq);
bool hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t direction);
size_t hal_dma_get_ct(DMA_HandleTypeDef *dma);
void hal_dma_enable_dbm(DMA_HandleTypeDef *dma, void *m0 = nullptr, void *m1 = nullptr);
void hal_dma_update_memory(DMA_HandleTypeDef *dma, void *addr);
bool hal_adc_config(ADC_HandleTypeDef *adc, uint32_t resolution, uint32_t trigger,
                    PinName *adc_pins, uint32_t n_channels, uint32_t sample_time);
bool hal_adc_enable_dual_mode(bool enable);

#endif // __HAL_CONFIG_H__
