/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino-style wrapper over the Hubble SDK's C APIs.
 * Sketches can use this class, directly call the C APIs, or both.
 * This library sets Hubble configurations in hubble_config.h. You
 * may edit that file to change Hubble key size, rotation period, etc.
 */

#ifndef HUBBLE_NETWORK_H
#define HUBBLE_NETWORK_H

/* Include config file first. Ensures files can access the set configs. */
#include "hubble_config.h"

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/version.h>
#include <hubble/port/crypto.h>
#include <hubble/port/sys.h>


/* Total bytes a legacy BLE advertisement can carry. */
#define HUBBLE_BLE_ADV_MAX_LEN 31

#ifdef __cplusplus

/**
 * @brief Arduino wrappers for Hubble network devices.
 *
 * This class provides a wrapper for core SDK functionality.
 */
class HubbleNetworkClass
{
public:
	/**
	 * @brief Advertising intervals, in units of 0.625 ms.
	 */
	static const uint16_t ADV_INTERVAL_FAST = 160;  /* 100 ms */
	static const uint16_t ADV_INTERVAL_SLOW = 1600; /* 1 s */

	/** @brief Master key length this library was built for. */
	static const size_t KEY_LEN = CONFIG_HUBBLE_KEY_SIZE;

	/** @brief Largest payload setData() accepts. */
	static const size_t MAX_DATA_LEN = HUBBLE_BLE_MAX_DATA_LEN;

	/**
	 * @brief Initialize the SDK with a base64 master key.
	 *
	 * Takes the key exactly as the Hubble dashboard issues it, and holds
	 * it for as long as the SDK needs it.
	 *
	 * An empty key ("") is allowed and logs a warning: the device still
	 * transmits, which is useful for checking the RF on a scanner or an
	 * SDR before a key exists, but nothing it sends can reach the Hubble
	 * Cloud. A key that is present but malformed is an error.
	 *
	 * This does not bring up BLE -- start() does. See the class note.
	 *
	 * @param initialTime The initial time used for tracking key rotations.
	 *                    Pass in millis() for this library's default device-uptime
	 *                    counter. Or, any value you want the timer to start at. 0
	 *                    is an acceptable input. In a build switched to using
	 *                    CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME, pass in the current
	 *                    Unix time in milliseconds.
	 * @param keyBase64   Master key, base64, as issued. An empty string
	 *                    selects an all-zero key, with a warning.
	 *
	 * @return true on success.
	 */
	bool begin(uint64_t initialTime, const char *keyBase64);

	/**
	 * @brief Release the SDK.
	 *
	 * Stops advertising, takes the BLE stack down if start() had brought
	 * it up, and clears the key and payload from RAM.
	 */
	void end(void);

	/**
	 * @brief Set the payload carried in the advertisement.
	 *
	 * Payload can be empty - the advertisements are still emitted and will
	 * reach the Hubble Cloud. If already advertising, the new payload is
	 * beaconed immediately. Otherwise, it is beaconed on the next start().
	 *
	 * @param data Payload bytes, copied.
	 * @param len  Payload length, at most MAX_DATA_LEN.
	 *
	 * @return true on success.
	 */
	bool setData(const uint8_t *data, size_t len);

	/** @brief setData() for a NULL-terminated string, excluding the NULL. */
	bool setData(const char *str);

	/**
	 * @brief Start advertising to the terrestrial network.
	 *
	 * Brings the BLE stack up if it is not already, then advertises. The
	 * advertisement is non-connectable: it is a beacon. A sketch that
	 * needs to be connectable as well should drive its board's BLE
	 * library itself and get the payload from advertisementGet().
	 *
	 * @param advertisingInterval In units of 0.625 ms.
	 *
	 * @return true on success.
	 */
	bool start(uint16_t advertisingInterval = ADV_INTERVAL_SLOW);

	/**
	 * @brief Stop advertising.
	 *
	 * The key, the payload and the BLE stack are all kept, so start()
	 * can resume cheaply.
	 */
	bool stop(void);

	/**
	 * @brief Give the library time to run. Call from loop().
	 *
	 * Services the BLE stack, and rebuilds and republishes the
	 * advertisement when the Hubble EID rotates -- which it must do, or
	 * the network stops being able to decode it. Cheap enough to call
	 * every iteration, and safe to call before start().
	 */
	void poll(void);

	/**
	 * @brief Milliseconds until the advertisement has to be rebuilt.
	 *
	 * poll() does the rebuild on its own, so a sketch needs this only to
	 * plan around it -- a power-managed sketch can sleep for this long
	 * and know it will wake in time.
	 *
	 * @return 0 when not advertising, or when a rebuild is already due.
	 */
	uint32_t msUntilRefresh(void) const;

	/**
	 * @brief Get the raw advertising data, for a sketch driving its own
	 *        BLE stack.
	 *
	 * Writes a complete AD payload: the 16-bit service UUID list
	 * followed by Hubble's service data, ready for
	 * BLEAdvertisingData::setRawData() or any other stack.
	 *
	 * start() does this internally; this is the escape hatch for
	 * whatever start() does not cover.
	 *
	 * @param out     Buffer of at least HUBBLE_BLE_ADV_MAX_LEN bytes.
	 * @param out_len in: size of @p out. out: bytes written.
	 *
	 * @retval 0       On success.
	 * @retval -EPERM  If begin() has not succeeded.
	 * @retval -ENOMEM If @p out is too small.
	 */
	int advertisementGet(uint8_t *out, size_t *out_len);

private:
	enum State {
		NOINIT,
		READY,
		ADVERTISING,
		ADVERTISING_FAIL,
	};

	bool sdkInit(uint64_t initialTime);
	bool fail(const char *what, int error, const char *hint = NULL);

	State state = NOINIT;
	uint8_t masterKey[KEY_LEN] = {0};

	bool bleUp(void);

	bool bleBegun = false;

	uint16_t advInterval = ADV_INTERVAL_SLOW;
	uint32_t advBuiltMs = 0;
	uint32_t advValidMs = 0;

	/* The BLE advertisement buffer. */
	uint8_t advBuf[HUBBLE_BLE_ADV_MAX_LEN] = {0};
	size_t advLen = 0;

	/* The custom payload the advertisement carries. */
	uint8_t payload[MAX_DATA_LEN] = {0};
	size_t payloadLen = 0;
};

/** @brief The Hubble Network instance a sketch uses. */
extern HubbleNetworkClass HubbleNetwork;

#endif /* __cplusplus */

#endif /* HUBBLE_NETWORK_H */
