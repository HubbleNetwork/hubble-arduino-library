/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hubble Network BLE Beacon Sample
 *
 * Sends encrypted packets to Hubble's terrestrial (BLE) network.
 *
 * REQUIREMENTS
 *   - Arduino IDE 2.0 or later
 *   - Supported Arduino board (see README.md)
 *   - Download the "ArduinoBLE" library
 *     -- In IDE toolbar: Tools > Manage Libraries > "ArduinoBLE" > Install
 *
 * FLASHING
 *   1. Connect your board and select the correct COM port.
 *   2. Select your board under Tools > Board.
 *   3. Provision your device:
 *      a. Get a Hubble device key from https://dash.hubble.com/dashboard.
 *         -- Add a Device > I just need a Device Key.
 *         -- Leave "Encryption" as default. Set "Encryption Mode" to
 *            "Device Uptime".
 *      b. Paste the key into the HUBBLE_KEY_B64 below.
 *   4. Click "Upload" in Arduino IDE.
 *
 * TESTING
 *   1. Open the Serial Monitor at 115200 baud.
 *   2. Watch for error messages.
 *   3. Install the Hubble Connect mobile app and sign in.
 *   4. Enable Bluetooth and location permissions in the app.
 *   5. The device name associated with your Hubble key should appear
 *      under "My Devices".
 *   6. Check https://dash.hubble.com/dashboard for your device's location.
 */

#include <HubbleNetwork.h>

/*
 * Hubble Key in Base64. Paste your key from
 * https://dash.hubble.com/dashboard here! It should look like this
 * "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefg=". Make sure to paste in all
 * of the text, including any "/" characters and the trailing equal sign.
 *
 * WARNING: You should keep your key secret. Do not publish code with
 * your key in it.
 */
#define HUBBLE_KEY_B64 ""

void setup()
{
	Serial.begin(115200);

	unsigned long start = millis();
	while (!Serial && ((millis() - start) < 3000UL)) {
	}

	Serial.println("Hubble Network Terrestrial Sample started");

	/* Decodes the key and initializes the SDK. Reports its own failures. */
	HubbleNetwork.begin(millis(), HUBBLE_KEY_B64);

	/* Brings up Bluetooth and starts advertising. */
	HubbleNetwork.start(HubbleNetwork.ADV_INTERVAL_FAST);
}

void loop()
{
	/*
	 * Services Bluetooth, and re-encrypts and republishes the
	 * advertisement when the Hubble EID rotates. Skipping this would
	 * leave the network unable to decode anything the device sends after
	 * the next rotation.
	 */
	HubbleNetwork.poll();
}
