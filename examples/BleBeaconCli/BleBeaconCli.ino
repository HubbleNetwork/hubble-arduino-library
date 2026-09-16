/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hubble Network BLE Beacon CLI Sample
 *
 * A serial console for the terrestrial (BLE) network: provision a key at run
 * time using serial monitor, then you can set a custom payload to send.
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
 *   3. Click "Upload" in Arduino IDE. The key is entered at run time, so
 *      nothing needs to be pasted into this sketch.
 *
 * TESTING
 *   1. Open the Serial Monitor at 115200 baud.
 *   2. Get a Hubble device key from https://dash.hubble.com/dashboard.
 *      -- Add a Device > I just need a Device Key.
 *      -- Leave "Encryption" as default. Set "Encryption Mode" to
 *         "Device Uptime".
 *   3. Type: provision <your base64 key>
 *   4. Type: send hello
 *   5. Check https://dash.hubble.com/dashboard for your device and its
 *      payload.
 */

#include <HubbleNetwork.h>

static bool provisioned = false;
static String payload;

/* Print the help message to the serial console. */
static void print_help(void)
{
	Serial.println();
	Serial.println("Commands:");
	Serial.println("  provision <base64 key>  initialize with a Hubble key");
	Serial.print("  send <text>             set the payload (up to ");
	Serial.print((int)HubbleNetwork.MAX_DATA_LEN);
	Serial.println(" bytes)");
	Serial.println("  clear                   advertise presence only");
	Serial.println("  start                   start advertising");
	Serial.println("  stop                    stop advertising");
	Serial.println("  status                  show current settings");
	Serial.println("  help                    this message");
	Serial.println();
}

/* Decode the key and bring up the SDK, then start advertising. */
static void cmd_provision(const String &arg)
{
	if (arg.length() == 0) {
		Serial.println("provision needs a base64 key");
		return;
	}

	if (!HubbleNetwork.begin(millis(), arg.c_str())) {
		return;
	}

	provisioned = true;

	/* Brings up Bluetooth and starts advertising. */
	if (HubbleNetwork.start(HubbleNetwork.ADV_INTERVAL_FAST)) {
		Serial.println("Advertising started");
	}
}

/* Set the payload the advertisement carries. */
static void cmd_send(const String &arg)
{
	if (!provisioned) {
		Serial.println("Not provisioned yet -- run provision first");
		return;
	}

	if (arg.length() > HubbleNetwork.MAX_DATA_LEN) {
		Serial.print("Payload too long: ");
		Serial.print((int)arg.length());
		Serial.print(" bytes, maximum is ");
		Serial.println((int)HubbleNetwork.MAX_DATA_LEN);
		return;
	}

	/*
	 * setData() republishes straight away when advertising, so the new
	 * payload goes out without waiting for the next EID rotation.
	 */
	if (HubbleNetwork.setData(arg.c_str())) {
		payload = arg.c_str();
		Serial.print("Payload set to: ");
		Serial.println(payload);
	}
}

/* Print the current settings to the serial console. */
static void print_status(void)
{
	Serial.print("Provisioned : ");
	Serial.println(provisioned ? "yes" : "no");
	Serial.print("Payload : ");
	Serial.println(payload);
}

/* Handle a line of input from the serial console. */
static void handle_line(String line)
{
	line.trim();
	if (line.length() == 0) {
		return;
	}

	int space = line.indexOf(' ');
	String cmd = (space < 0) ? line : line.substring(0, space);
	String arg = (space < 0) ? String("") : line.substring(space + 1);
	arg.trim();

	if (cmd == "provision") {
		cmd_provision(arg);
	} else if (cmd == "send") {
		cmd_send(arg);
	} else if (cmd == "clear") {
		if (HubbleNetwork.setData(NULL, 0)) {
			Serial.println("Payload cleared");
		}
	} else if (cmd == "start") {
		if (HubbleNetwork.start(HubbleNetwork.ADV_INTERVAL_FAST)) {
			Serial.println("Advertising started");
		}
	} else if (cmd == "stop") {
		if (HubbleNetwork.stop()) {
			Serial.println("Advertising stopped");
		}
	} else if (cmd == "status") {
		print_status();
	} else if (cmd == "help") {
		print_help();
	} else {
		Serial.print("unknown command: ");
		Serial.println(cmd);
		print_help();
	}
}

void setup()
{
	Serial.begin(115200);

	unsigned long start = millis();
	while (!Serial && ((millis() - start) < 3000UL)) {
	}

	Serial.println("Hubble Network BLE Beacon Shell started");

	print_help();
}

void loop()
{
	static String line;

	/*
	 * Services Bluetooth, and re-encrypts and republishes when the Hubble
	 * EID rotates. Safe to call before the first provision command.
	 */
	HubbleNetwork.poll();

	while (Serial.available() > 0) {
		char c = (char)Serial.read();

		if ((c == '\n') || (c == '\r')) {
			if (line.length() > 0) {
				handle_line(line);
				line = "";
			}
		} else if (line.length() < 80) {
			line += c;
		}
	}
}
