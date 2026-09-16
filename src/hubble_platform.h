/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Configuration for varying Arduino cores and boards.
 */

#ifndef HUBBLE_PLATFORM_H
#define HUBBLE_PLATFORM_H

/*
 * Bluetooth backend - always includes hubble_ble.h. A source file
 * is selectively compiled based on the BLE backend defined below.
 *
 * ArduinoBLE is the default and covers the most ground.
 *
 * Espressif's esp32 core (3.x) does not support ArduinoBLE for some boards.
 * Instead, it just uses its own BLE library. The Arduino-maintained ESP32 core
 * is on version 2.0.18, which does support ArduinoBLE.
 */
#ifdef ARDUINO_ARCH_ESP32
#include <esp_arduino_version.h>
#endif

#if defined(ARDUINO_ARCH_ESP32) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
#define HUBBLE_BLE_ESP32 1
#else
#define HUBBLE_BLE_ARDUINOBLE 1
#endif

#endif /* HUBBLE_PLATFORM_H */
