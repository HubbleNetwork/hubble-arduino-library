/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The BLE stack has a selectable backend to maximize portability: some
 * ESP32 boards are not compatible with ArduinoBLE. The correct backend is
 * selected automatically, in hubble_platform.h, from the Arduino core.
 */

#ifndef HUBBLE_BLE_H
#define HUBBLE_BLE_H

/* Include config file first. Ensures files can access the set configs. */
#include "hubble_config.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Bring the BLE stack up. Idempotent.
 *
 * @return true on success.
 */
bool hubbleBleBegin(void);

/** @brief Take the BLE stack down and release what it holds. */
void hubbleBleEnd(void);

/**
 * @brief Put @p ad on the air, replacing whatever was advertising.
 *
 * Sets new advertising data, @p ad as a nonconnectable beacon.
 *
 * @param ad             Complete AD payload, at most HUBBLE_BLE_ADV_MAX_LEN
 *                       bytes. Must stay valid and unchanged until the next
 *                       call -- a backend may keep the pointer rather than
 *                       copy, so a caller's buffer has to outlive the call.
 * @param len            Bytes in @p ad.
 * @param intervalUnits  Advertising interval in units of 0.625 ms.
 *
 * @return true if the radio is advertising @p ad.
 */
bool hubbleBleAdvertise(const uint8_t *ad, size_t len,
			uint16_t intervalUnits);

/** @brief Stop advertising, leaving the stack up. */
void hubbleBleStopAdvertise(void);

/** @brief Give the stack time to run, from the sketch's loop(). */
void hubbleBlePoll(void);

/* Allow a few retries to start advertising. */
#define HUBBLE_BLE_ADVERTISE_ATTEMPTS 3U
#define HUBBLE_BLE_ADVERTISE_RETRY_MS 50U

#endif /* HUBBLE_BLE_H */
