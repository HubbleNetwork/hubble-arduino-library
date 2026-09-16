/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hubble BLE backend through ArduinoBLE. This library covers most boards
 * and cores, and is a good default backend.
 */

#include "hubble_ble.h"

#if defined(HUBBLE_BLE_ARDUINOBLE)

#include <Arduino.h>
#include <ArduinoBLE.h>

static bool _begun = false;

bool hubbleBleBegin(void)
{
	if (_begun) {
		return true;
	}

	if (BLE.begin() == 0) {
		return false;
	}

	_begun = true;

	return true;
}

void hubbleBleEnd(void)
{
	if (!_begun) {
		return;
	}

	BLE.end();
	_begun = false;
}

bool hubbleBleAdvertise(const uint8_t *ad, size_t len,
			       uint16_t intervalUnits)
{
	BLEAdvertisingData advData;

	/*
	 * setRawData() keeps the pointer it is given -- updateData() only
	 * copies out of it later, inside BLE.advertise(). So the caller's
	 * buffer must outlive this call.
	 *
	 * Raw data also suppresses the Flags element ArduinoBLE would
	 * otherwise generate, which is what frees the whole 31 bytes.
	 */
	if (!advData.setRawData(ad, (int)len)) {
		return false;
	}

	BLE.stopAdvertise();
	BLE.setAdvertisingData(advData);
	BLE.setAdvertisingInterval(intervalUnits);

	/* A beacon only advertises; it should not accept connections. */
	BLE.setConnectable(false);

	/* Allow HUBBLE_BLE_ADVERTISE_ATTEMPTS to start advertising. */
	for (unsigned int attempt = 0; attempt < HUBBLE_BLE_ADVERTISE_ATTEMPTS;
	     attempt++) {
		if (BLE.advertise() != 0) {
			return true;
		}

		if ((attempt + 1U) < HUBBLE_BLE_ADVERTISE_ATTEMPTS) {
			delay(HUBBLE_BLE_ADVERTISE_RETRY_MS);
		}
	}

	return false;
}

void hubbleBleStopAdvertise(void)
{
	BLE.stopAdvertise();
}

void hubbleBlePoll(void)
{
	if (_begun) {
		BLE.poll();
	}
}

#endif /* HUBBLE_BLE_ARDUINOBLE */
