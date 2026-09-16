/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hubble BLE backend through ESP32's own BLE library. Used instead of
 * ArduinoBLE for ESP32 boards that have their own BLE stack.
 */

#include "hubble_ble.h"

#if defined(HUBBLE_BLE_ESP32)

#include <Arduino.h>

#if !defined(SOC_BLE_SUPPORTED)
#error "This ESP32 variant has no Bluetooth LE radio, so it cannot reach the Hubble terrestrial network. Select an ESP32 with BLE."
#endif

#include <BLEAdvertising.h>
#include <BLEDevice.h>

bool hubbleBleBegin(void)
{
	if (BLEDevice::getInitialized()) {
		return true;
	}

	return BLEDevice::init("");
}

void hubbleBleEnd(void)
{
	if (!BLEDevice::getInitialized()) {
		return;
	}

	BLEDevice::deinit(true);
}

bool hubbleBleAdvertise(const uint8_t *ad, size_t len,
			       uint16_t intervalUnits)
{
	BLEAdvertising *advertising = BLEDevice::getAdvertising();
	BLEAdvertisementData advData;

	if (advertising == NULL) {
		return false;
	}

	/*
	 * addData() takes a non-const char *, but only reads from it; the cast
	 * is to satisfy the signature, not to permit a write.
	 */
	advData.addData(const_cast<char *>(reinterpret_cast<const char *>(ad)),
			len);

	/*
	 * addData() silently does nothing when the payload would overflow
	 * so check the payload arrived instead of advertising a truncated one.
	 */
	if (advData.getPayload().length() != len) {
		return false;
	}

	(void)advertising->stop();

	advertising->setScanResponse(false);
	advertising->setMinInterval(intervalUnits);
	advertising->setMaxInterval(intervalUnits);

	/* A beacon only advertises; it should not accept connections. */
#if defined(CONFIG_BLUEDROID_ENABLED)
	advertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
#else
	advertising->setAdvertisementType(BLE_GAP_CONN_MODE_NON);
#endif

	if (!advertising->setAdvertisementData(advData)) {
		return false;
	}

	/* Allow HUBBLE_BLE_ADVERTISE_ATTEMPTS to start advertising. */
	for (unsigned int attempt = 0; attempt < HUBBLE_BLE_ADVERTISE_ATTEMPTS;
	     attempt++) {
		if (advertising->start()) {
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
	BLEAdvertising *advertising = BLEDevice::getAdvertising();

	if (advertising != NULL) {
		(void)advertising->stop();
	}
}

void hubbleBlePoll(void)
{
	/* Nothing to do */
}

#endif /* HUBBLE_BLE_ESP32 */
