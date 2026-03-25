/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Espressif Systems
 *    integrated circuit in a product or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* light intensity level */
#define LIGHT_DEFAULT_ON  1
#define LIGHT_DEFAULT_OFF 0

/* LED strip configuration */
#define CONFIG_EXAMPLE_STRIP_LED_GPIO   8
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 1

/**
* @brief color light driver init, be invoked where you want to use color light
*
* @param power power on/off
*/
void light_driver_init(bool power);

/**
* @brief Set light power (on/off).
*
* @param  power  The light power to be set
*/
void light_driver_set_power(bool power);

/**
* @brief Set light brightness level.
*
* @param level  Brightness level (0-254, ZCL range)
*/
void light_driver_set_level(uint8_t level);

/**
* @brief Set light color hue.
*
* @param hue  Hue value (0-254, ZCL range maps to 0-360 degrees)
*/
void light_driver_set_color_hue(uint8_t hue);

/**
* @brief Set light color saturation.
*
* @param saturation  Saturation value (0-254, ZCL range)
*/
void light_driver_set_color_saturation(uint8_t saturation);

/**
* @brief Set light color using CIE 1931 XY chromaticity coordinates.
*
* @param x  CIE X value (ZCL range 0-65279, represents 0.0-0.9961)
* @param y  CIE Y value (ZCL range 0-65279, represents 0.0-0.9961)
*/
void light_driver_set_color_xy(uint16_t x, uint16_t y);

/**
* @brief Get current CIE X color value.
* @return ZCL CIE X value (0-65279)
*/
uint16_t light_driver_get_color_x(void);

/**
* @brief Get current CIE Y color value.
* @return ZCL CIE Y value (0-65279)
*/
uint16_t light_driver_get_color_y(void);

/**
* @brief Start LED blinking to indicate network search/join.
*        Overrides the current light state visually until stopped.
*
* @param period_ms  Full blink period in milliseconds (on + off).
*/
void light_driver_blink_start(uint32_t period_ms);

/**
* @brief Stop LED blinking and restore the actual light state.
*/
void light_driver_blink_stop(void);

#ifdef __cplusplus
} // extern "C"
#endif
