/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Build configuration for the HubbleNetwork Arduino library.
 *
 * NOTE: a library has ONE configuration shared by every sketch that uses
 * it -- Arduino provides no way to configure a library per sketch. Edit
 * this file to change the whole library's behavior.
 */

#ifndef HUBBLE_CONFIG_H
#define HUBBLE_CONFIG_H

#include "hubble_platform.h"

#define CONFIG_HUBBLE_BLE_NETWORK                          1

#define CONFIG_HUBBLE_NETWORK_KEY_256                      1

/* Counter source set to device uptime to reduce provision efforts. */
#define CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME         1

#define CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC              86400

#define CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK 1

/*
 * Hubble library log verbosity:
 *   0  nothing
 *   1  error
 *   2  warning
 *   3  info (default)
 *   4  debug
 */
#define CONFIG_HUBBLE_ARDUINO_LOG_LEVEL 3


#endif /* HUBBLE_CONFIG_H */
