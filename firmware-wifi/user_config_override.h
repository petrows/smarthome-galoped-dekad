/*
  user_config_override.h - user configuration overrides my_user_config.h for Tasmota

  Copyright (C) 2021  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

/*****************************************************************************************************\
 * USAGE:
 *   To modify the stock configuration without changing the my_user_config.h file:
 *   (1) copy this file to "user_config_override.h" (It will be ignored by Git)
 *   (2) define your own settings below
 *
 ******************************************************************************************************
 * ATTENTION:
 *   - Changes to SECTION1 PARAMETER defines will only override flash settings if you change define CFG_HOLDER.
 *   - Expect compiler warnings when no ifdef/undef/endif sequence is used.
 *   - You still need to update my_user_config.h for major define USE_MQTT_TLS.
 *   - All parameters can be persistent changed online using commands via MQTT, WebConsole or Serial.
\*****************************************************************************************************/

#undef PROJECT
#define PROJECT "galoped"

#undef MQTT_TOPIC
#define MQTT_TOPIC PROJECT "_%06X"

#undef FRIENDLY_NAME
#define FRIENDLY_NAME "Galoped"

// Enabke custom Galoped UI (not part of original Tasmota)
#define USE_GALOPED

// Advanced animation modules for RGB
#define USE_BERRY_ANIMATION

// Required features for this project:
// [I2cDriver43] Enable AHT20/AM2301B instead of AHT1x humidity and temperature sensor (I2C address 0x38) (+0k8 code)
#ifndef USE_AHT2x
    #define USE_AHT2x
#endif
// [I2cDriver10] Enable BMP085/BMP180/BMP280/BME280 sensors (I2C addresses 0x76 and 0x77) (+4k4 code)
#ifndef USE_BMP
    #define USE_BMP
#endif
// Add support for SenseAir K30, K70 and S8 CO2 sensor (+2k3 code)
#ifndef USE_SENSEAIR
    #define USE_SENSEAIR
#endif
// Add support for VID6608 Automotive analog gauge driver (+0k7 code)
#define USE_VID6608
// Reset VID6608 on init (default: true), change if you control this manually
// Starting from 2026-04-03: reset is controlled from FW Galoped driver
#define VID6608_RESET_ON_INIT false
// Default steps - same for singe- and bi-axial versions, 320° + 270°
#define VID6608_STEPS_1 12 * 320
#define VID6608_STEPS_2 12 * 275

// Enable WS2812 leds number (any HW model)
#undef WS2812_LEDS
// If RGB backlight is used, it will have 40 LED's
#define WS2812_LEDS 40

// Template defaults, apply on flash reset
// Apply default backlight options, i.e. color and level
// Default color: #4D3912
#define GALOPED_TEMPLATE_CMD "Power1 ON|Dimmer1 30|Color1 4D3912"

// Define chipset here
#if CONFIG_IDF_TARGET_ESP32
#define GALOPED_PLATFORM_ESP32
#elif CONFIG_IDF_TARGET_ESP32C6
#define GALOPED_PLATFORM_ESP32C6
#else
#error "Unsupported platform"
#endif

// Defined Galoped models for pre-configuration
// Axis count (1 or 2)
#ifndef GALOPED_AXIS
#   define GALOPED_AXIS 1  // Default is single-axis
#endif
// Backlight
#define GALOPED_BACKLIGHT_RGB 0
#define GALOPED_BACKLIGHT_RETRO 1
#ifndef GALOPED_BACKLIGHT
#   define GALOPED_BACKLIGHT GALOPED_BACKLIGHT_RGB // Default is RGB
#endif

// Generate our template (ESP32 and ESP32C6 require different)
// ESP32-WROOM32
#ifdef GALOPED_PLATFORM_ESP32
#if GALOPED_BACKLIGHT == GALOPED_BACKLIGHT_RGB
#define GALOPED_TEMPLATE_BACKLIGHT "0,1376"
#endif
#if GALOPED_BACKLIGHT == GALOPED_BACKLIGHT_RETRO
#define GALOPED_TEMPLATE_BACKLIGHT "416,0"
#endif
#if GALOPED_AXIS == 1
#define GALOPED_TEMPLATE_AXIS_2 "1,1"
#endif
#if GALOPED_AXIS == 2
#define GALOPED_TEMPLATE_AXIS_2 "12161,12193"
#endif

#define USER_TEMPLATE "{\"NAME\":\"Galoped\",\"GPIO\":[1,1,1,1,1,1,1,1,1,1," GALOPED_TEMPLATE_BACKLIGHT ",1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0," GALOPED_TEMPLATE_AXIS_2 ",32,1,1,0,0,1],\"FLAG\":0,\"BASE\":1,\"CMND\":\"" GALOPED_TEMPLATE_CMD "\"}"

#endif // GALOPED_PLATFORM_ESP32

// ESP32-C6
#ifdef GALOPED_PLATFORM_ESP32C6
#if GALOPED_BACKLIGHT != GALOPED_BACKLIGHT_RGB
#error "ESP32-C6 has RGB backlight only"
#endif
#if GALOPED_AXIS == 1
#define GALOPED_TEMPLATE_AXIS_1 "12160"
#define GALOPED_TEMPLATE_AXIS_2 "12192,1,1"
#endif
#if GALOPED_AXIS == 2
#define GALOPED_TEMPLATE_AXIS_1 "12160"
#define GALOPED_TEMPLATE_AXIS_2 "12192,12161,12193"
#endif

#define USER_TEMPLATE "{\"NAME\":\"Galoped-s\",\"ARCH\":\"ESP32C6\",\"GPIO\":[1600,1632,608,640,1,1,1376,1,1377,32,0,0,0,0," GALOPED_TEMPLATE_AXIS_1 ",544,0,0," GALOPED_TEMPLATE_AXIS_2 ",1,0,0,0,0,0,0,0,12193,32],\"FLAG\":0,\"BASE\":1,\"CMND\":\"" GALOPED_TEMPLATE_CMD "\"}"

#endif // GALOPED_PLATFORM_ESP32C6

// Activate template on reset/flash new
#undef MODULE
#define MODULE USER_MODULE

#endif  // _USER_CONFIG_OVERRIDE_H_
