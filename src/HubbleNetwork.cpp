/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino-style wrapper over the Hubble SDK's C APIs.
 * Sketches can use this class, directly call the C APIs, or both.
 * This library sets Hubble configurations in hubble_config.h. You
 * may edit that file to change Hubble key size, rotation period, etc.
 */

#include "HubbleNetwork.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <Arduino.h>

#include "b64.h"
#include "hubble_ble.h"

HubbleNetworkClass HubbleNetwork;

/* AD types, from the Bluetooth Assigned Numbers document. */
#define AD_TYPE_UUID16_ALL 0x03 /* Complete list of 16-bit service UUIDs. */
#define AD_TYPE_SVC_DATA16 0x16 /* Service Data - 16-bit UUID. */

/* The 4-byte UUID list, then the 2-byte service data header. */
#define AD_PREFIX_LEN      6U

/* The advice every entry point that needs begin() to have run gives. */
static const char *const NEEDS_BEGIN_HINT =
	"call HubbleNetwork.begin() first, and check that it returned true";

/*
 * The advertisement is built to exactly fill a legacy packet, so a change to
 * any of these sizes has to be noticed here rather than at run time, where it
 * would show up as advertisements that silently stop being emitted.
 */
static_assert((AD_PREFIX_LEN + HUBBLE_BLE_ADV_HEADER_SIZE +
	       HUBBLE_BLE_MAX_DATA_LEN) <= HUBBLE_BLE_ADV_MAX_LEN,
	      "Hubble payload does not fit in one legacy advertisement");

/* Record a failure and report it through the SDK's log. */
bool HubbleNetworkClass::fail(const char *what, int error, const char *hint)
{
	(void)hubble_log(HUBBLE_LOG_ERROR, "%s failed (err=%d)\n", what, error);

	if (hint != NULL) {
		(void)hubble_log(HUBBLE_LOG_ERROR, "  %s\n", hint);
	}

	return false;
}

bool HubbleNetworkClass::sdkInit(uint64_t initialTime)
{
	int status = hubble_init(initialTime, this->masterKey);

	if (status != 0) {
		return this->fail("hubble_init", status);
	}

	this->state = READY;

	return true;
}

bool HubbleNetworkClass::begin(uint64_t initialTime, const char *keyBase64)
{
	int status;

	if ((keyBase64 == NULL) || (keyBase64[0] == '\0')) {
		/*
		 * Deliberately allowed, but issue a warning. The device will not
		 * decode on the Hubble Cloud, but allowing transmission can be an
		 * useful debugging test.
		 */
		(void)hubble_log(HUBBLE_LOG_WARNING,
				 "no master key set -- transmitting with an "
				 "all-zero key\n");

		memset(this->masterKey, 0, sizeof(this->masterKey));

		return this->sdkInit(initialTime);
	}

	status = b64_decode(keyBase64, this->masterKey,
				   sizeof(this->masterKey));
	if (status != 0) {
		/* A rejected decode may have filled part of the buffer. */
		memset(this->masterKey, 0, sizeof(this->masterKey));

		(void)hubble_log(HUBBLE_LOG_ERROR,
				 "master key did not decode to %u bytes; "
				 "check it was pasted in full (err=%d)\n",
				 (unsigned int)sizeof(this->masterKey), status);

		return false;
	}

	return this->sdkInit(initialTime);
}

void HubbleNetworkClass::end(void)
{
	if (this->state == NOINIT) {
		return;
	}

	(void)this->stop();

	if (this->bleBegun) {
		hubbleBleEnd();
		this->bleBegun = false;
	}

	memset(this->payload, 0, sizeof(this->payload));
	memset(this->masterKey, 0, sizeof(this->masterKey));

	this->payloadLen = 0;
	this->advLen = 0;
	this->state = NOINIT;
}

int HubbleNetworkClass::advertisementGet(uint8_t *out, size_t *out_len)
{
	uint8_t hubbleAd[HUBBLE_BLE_ADV_HEADER_SIZE + MAX_DATA_LEN];
	size_t hubbleLen = sizeof(hubbleAd);
	size_t n = 0;
	int status;

	if ((out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if (this->state == NOINIT) {
		return -EPERM;
	}

	status = hubble_ble_advertise_get(
		(this->payloadLen > 0) ? this->payload : NULL, this->payloadLen,
		hubbleAd, &hubbleLen);
	if (status != 0) {
		return status;
	}

	/*
	 * Checked before anything is written, so a caller's buffer -- and the
	 * advertisement currently on the air -- survives a failure intact.
	 */
	if (*out_len < (AD_PREFIX_LEN + hubbleLen)) {
		return -ENOMEM;
	}

	/* Complete list of 16-bit service UUIDs: just Hubble's, little-endian. */
	out[n++] = 3;
	out[n++] = AD_TYPE_UUID16_ALL;
	out[n++] = (uint8_t)(HUBBLE_BLE_UUID & 0xFF);
	out[n++] = (uint8_t)(HUBBLE_BLE_UUID >> 8);

	/* Service data. hubbleAd already opens with the same UUID. */
	out[n++] = (uint8_t)(hubbleLen + 1);
	out[n++] = AD_TYPE_SVC_DATA16;
	memcpy(&out[n], hubbleAd, hubbleLen);
	n += hubbleLen;

	*out_len = n;

	return 0;
}

/* Bring the BLE stack up, once. */
bool HubbleNetworkClass::bleUp(void)
{
	if (this->bleBegun) {
		return true;
	}

	if (!hubbleBleBegin()) {
		return this->fail("BLE begin", -EIO,
				  "the BLE stack would not start; does this "
				  "board have Bluetooth?");
	}

	this->bleBegun = true;

	return true;
}

/* Set the custom payload data to a buffer. */
bool HubbleNetworkClass::setData(const uint8_t *data, size_t len)
{
	if (this->state == NOINIT) {
		return this->fail("setData", -EPERM, NEEDS_BEGIN_HINT);
	}

	if ((data == NULL) && (len > 0)) {
		return this->fail("setData", -EINVAL);
	}

	if (len > sizeof(this->payload)) {
		return this->fail(
			"setData", -EMSGSIZE,
			"payload is longer than HubbleNetwork.MAX_DATA_LEN "
			"bytes, which is all a single advertisement holds");
	}

	if (len > 0) {
		memcpy(this->payload, data, len);
	}

	this->payloadLen = len;

	/* Republish now, so the payload does not wait for the next rotation. */
	if (this->state == ADVERTISING) {
		return this->start(this->advInterval);
	}

	return true;
}

/* Set the custom payload data to a string. */
bool HubbleNetworkClass::setData(const char *str)
{
	if (str == NULL) {
		return this->fail("setData", -EINVAL);
	}

	return this->setData((const uint8_t *)str, strlen(str));
}

bool HubbleNetworkClass::start(uint16_t advertisingInterval)
{
	size_t len = sizeof(this->advBuf);
	int status;

	if (this->state == NOINIT) {
		return this->fail("start", -EPERM, NEEDS_BEGIN_HINT);
	}

	this->advInterval = advertisingInterval;

	if (!this->bleUp()) {
		this->state = ADVERTISING_FAIL;
		return false;
	}

	status = this->advertisementGet(this->advBuf, &len);
	if (status != 0) {
		/*
		 * If already advertising, the old advertisement remains.
		 * Report failure, but leave state as ADVERTISING.
		 */
		if (this->state != ADVERTISING) {
			this->state = ADVERTISING_FAIL;
		}

		return this->fail("advertisementGet", status);
	}

	this->advLen = len;

	/* On failure, advertising stops, so set advValidMs to 0 to retry on poll(). */
	if (!hubbleBleAdvertise(this->advBuf, this->advLen,
				advertisingInterval)) {
		this->advValidMs = 0;
		this->state = ADVERTISING_FAIL;

		return this->fail("BLE advertise", -EIO,
				  "the BLE stack would not start advertising");
	}

	this->advBuiltMs = millis();
	this->advValidMs = hubble_ble_advertise_expiration_get();
	this->state = ADVERTISING;

	return true;
}

bool HubbleNetworkClass::stop(void)
{
	if (this->state == NOINIT) {
		return this->fail("stop", -EPERM, NEEDS_BEGIN_HINT);
	}

	if ((this->state == ADVERTISING) || (this->state == ADVERTISING_FAIL)) {
		hubbleBleStopAdvertise();
	}

	this->state = READY;

	return true;
}

uint32_t HubbleNetworkClass::msUntilRefresh(void) const
{
	if (this->state != ADVERTISING) {
		return 0;
	}

	/* Unsigned, so this is wrap-safe the same way poll() is. */
	uint32_t elapsed = millis() - this->advBuiltMs;

	if (elapsed >= this->advValidMs) {
		return 0;
	}

	return this->advValidMs - elapsed;
}

void HubbleNetworkClass::poll(void)
{
	/* Keeps the uptime clock's millis() wrap tracking valid. */
	(void)hubble_uptime_get();

	if (this->bleBegun) {
		hubbleBlePoll();
	}

	/* Don't try to start() if not currently trying to advertise */
	if ((this->state != ADVERTISING) && (this->state != ADVERTISING_FAIL)) {
		return;
	}

	/*
	 * Unsigned subtraction, so this stays correct across the millis()
	 * wrap.
	 */
	if ((millis() - this->advBuiltMs) < this->advValidMs) {
		return;
	}

	/*
	 * The EID has rotated; re-encrypt with the new counter. A failure
	 * keeps the old advertisement running and is retried on the next
	 * call, so a transient error does not take the device off the air.
	 */
	(void)this->start(this->advInterval);
}
