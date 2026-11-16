# Hubble Network Arduino Library

The `HubbleNetwork` Arduino library enables Arduino devices to connect to and advertise data to the Hubble Network using BLE. It contains both the core functionality for constructing a Hubble-compatible BLE advertisement and a convenience wrapper that manages BLE for you.


## Contents

* [Prerequisites](#prerequisites)
* [Supported architectures](#supported-architectures)
* [Limitations](#limitations)
* [Quick start / Example sketch](#quick-start--example-sketches)
* [API (functions & behavior)](#api-functions--behavior)
* [Installation](#installation)
* [Building and running](#building--running)
* [Testing](#testing)
* [Library structure](#library-structure)
* [Maintenance & syncing](#maintenance--syncing)
* [Troubleshooting & debugging](#troubleshooting--debugging)
* [Contact & support](#contact--support)
* [License](#license)


## Prerequisites

* You must have an organization ID and API token from Hubble, and use those to generate device keys required to connect to the Hubble network.
  * [Get API keys](https://docs.hubble.com/docs/api-specification/hubble-platform-api#api-access)
  * Self-serve sign up flows available soon, until then [Contact Us](https://hubble.com/contact-us)
* The Hubble device key provided to `begin()` must be a Base64-encoded key (the library decodes this into a 32-byte master key). See the API notes below.
* This Arduino library depends on:
  * BLE-capable board
  * BLE stack
    * **ArduinoBLE** (for most boards)
    * **If using esp32 core, version 3.0.0+**
      * Some esp32's are not supported by ArduinoBLE. Instead, the Hubble library uses the esp32 core BLE driver.
      * ArduinoBLE will not be used. Having ArduinoBLE installed anyway will *not* be problematic.
  * Crypto driver
    * The Hubble SDK requires AES-CTR and AES-CMAC implementations. Those should be provided through a crypto library.
    * Currently, supported options are using MbedTLS or PSA Crypto provided through the Arduino core.


## Supported architectures

* The library metadata lists `architectures=*`, indicating broad architecture support (limited by the dependency ArduinoBLE).


### Tested cores

The following cores have been tested:
* Arduino Mbed OS Nano Boards
* esp32

### Extra core instructions:
* Arduino Mbed OS Nano Boards
  * There likely will be compilation warnings as some ArduinoBLE functionality is deprecated. The build still succeeds and flashes as normal.
* esp32
  * Make sure to enable logging to aid in debugging:
  * Tools > USB CDC on Boot > Enable

If you have a BLE-capable Arduino, that supports ArduinoBLE or is an esp32, and can't build the Hubble library, [contact us](#contact--support).


## Limitations

### Packet rebuild limits

For security, every advertisement the library builds is encrypted with a number that is never reused (a *nonce*). There are 1024 of these per 24-hour key-rotation period. Each call below increments the nonce value:

* each `start()`
* each `setData()` while advertising, even if the data didn't change
* each `advertisementGet()`
* the automatic rebuild `poll()` does once a day

Reusing an old nonce is a security vulnerability. So, once all 1024 nonces are used, attempting to update the advertisement will fail.

To stay inside this limit, only call `setData()` when your payload changes, or update it no more than once every 2 minutes (720 times per day).


## Quick start / example sketches

Get started with a library example, or with the simple sketch below.

### Library examples

| Example | What it does |
| --- | --- |
| [BleBeacon](examples/BleBeacon/) | Broadcasts empty packets to Hubble terrestrial network |
| [BleBeaconCli](examples/BleBeaconCli/) |  Sets up a serial console to provision your device and send custom data |

### Simple sketch

Paste in this simple sketch below:

```cpp
#include <Arduino.h>
#include <HubbleNetwork.h>

// TODO: replace with your generated device key.
const char* HUBBLE_KEY_B64 = "BASE64_ENCODED_32B_KEY";

void setup() {
  Serial.begin(115200);

  // Initialize Hubble network. Return value indicates success.
  if (!HubbleNetwork.begin(millis(), HUBBLE_KEY_B64)) {
    Serial.println("HubbleNetwork.begin() failed");
    while (1) delay(1000);
  }

  // Example payload (max 13 bytes — see API below)
  uint8_t payload[] = { 0x01, 0x02, 0x03 };
  if (!HubbleNetwork.setData(payload, sizeof(payload))) {
    Serial.println("HubbleNetwork.setData() failed");
  }

  /* Start advertising. Interval units are 0.625ms. Default parameter:
     HubbleNetwork.ADV_INTERVAL_SLOW = 1600 (1 s). */
  if (!HubbleNetwork.start()) {
    Serial.println("HubbleNetwork.start() failed");
  }
}

void loop() {
  // Must be called regularly to service BLE stack and Hubble internals.
  HubbleNetwork.poll();

  // ... your application logic ...
}
```


## API (functions & behavior)

The Arduino wrapper exposes the `HubbleNetwork` class with the following constants and functions:

### Constants

| Name | Value | Meaning |
| --- | --- | --- |
| `HubbleNetwork.ADV_INTERVAL_FAST` | 160 (100 ms) | A fast advertising interval. |
| `HubbleNetwork.ADV_INTERVAL_SLOW` | 1600 (1 s) | The slower advertising interval, default value. |
| `HubbleNetwork.MAX_DATA_LEN` | 13 | The largest payload, in bytes, that `setData()` accepts. |
| `HubbleNetwork.KEY_LEN` | 32 | The size, in bytes, of the master key after Base64 decoding. |

### Functions

**`bool HubbleNetwork.begin(uint64_t initialTime, const char *key)`**

* Initialize the Hubble BLE subsystem and core SDK state.
* `initialTime` is the starting value used for key rotation.
* `key` is the Base64-encoded device key, exactly as the Hubble dashboard issues it.
* The "Encryption Mode" the key was issued with on the dashboard changes the meaning of `initialTime`:
    * **Device Uptime** (this library's default, set in `src/hubble_config.h`): a starting number used to track key rotation. This can be any value - `millis()`, 0, etc.
    * **Unix time**: the current time, in milliseconds, since the Unix epoch. 0 is rejected in this mode.
* Returns `true` on success. Failure conditions include invalid base64 key or failure to initialize BLE.

**`void HubbleNetwork.end(void)`**

* Deinitialize and release BLE. Returns `true` on success.

**`void HubbleNetwork.poll(void)`**

* Call regularly from `loop()` to service the BLE stack and internal operations. If uninitialized, this is a no-op.

**`bool HubbleNetwork.setData(const uint8_t *data, size_t len)`**

* Set the 13-byte (maximum) payload to be advertised. The library constructs the Hubble BLE service data and updates the advertisement. If advertising is active, advertisement is briefly stopped while the new payload is installed and then restarted at the previous interval. Returns `true` on success.
* The data set will be stored and the advertisement updated periodically with encryption credentials.
* There is an overload, **`bool HubbleNetworkClass::setData(const char *str)`**, to directly input a string as payload data.

**`bool HubbleNetwork.start(uint16_t advertisingInterval = ADV_INTERVAL_SLOW)`**

* Start advertising on the Hubble network. `advertisingInterval` is specified in units of 0.625 ms. The default argument is `ADV_INTERVAL_SLOW` = **1600** (1600 * 0.625 ms = 1000 ms). If already started, `start()` stops and restarts advertising. Returns `true` on success.

**`bool HubbleNetwork.stop(void)`**

* Stop advertising and return `true` on success.


## Installation

1. Ensure dependencies are available in your Arduino environment:

   * Install **ArduinoBLE** (available from the Library Manager).
2. Install this library:

   * Option A : **Arduino Library Manager**: search for `HubbleNetwork` and install.
   * Option B : **Manual**: clone or download this repository and place it under your Arduino `libraries/` folder:
     `~/Arduino/libraries/hubble-arduino-library/`
3. Include the library from your sketch:

```cpp
#include <HubbleNetwork.h>
```

The library metadata (`library.properties`) lists the `depends` libraries and author/maintainer information.

## Building & running

* Provide a valid, Base64-encoded device key (generated from your Hubble organization account / core SDK) to `begin()`.
* Ensure your board supports ArduinoBLE (or is an ESP32) and the hardware BLE implementation.
* Upload one of our examples, or a custom sketch, to your board.


## Testing

1. Install the Hubble Connect mobile app and sign in.
2. Enable Bluetooth and location permissions in the app.
3. The device name associated with your Hubble key should appear under "My Devices".
4. Check https://dash.hubble.com/dashboard for your device's location and any custom payload you added.


## Library structure

* `library.properties` — Library metadata (name, version, dependencies).
* `src/HubbleNetwork.h` — Public Arduino wrapper header with documentation for each function.
* `src/HubbleNetwork.cpp` — Implementation: BLE management, advertisement buffer construction, `begin`/`start`/`stop`/`setData` logic.
* `src/hubble_config.h` — The one build configuration every sketch using the library shares.
* `src/hubble_platform.h` — Picks the BLE backend from the Arduino core.
* `src/hubble_ble_*.cpp` — The selectable BLE backends, one file each.
* `src/hubble/` — The SDK's public headers, the one directory Arduino puts on a sketch's include path.
* `extras/hubble-device-sdk/src/` — Required files from the Hubble SDK.
* `extras/hubble-device-sdk-files.txt` — The list of SDK files the sync workflow copies into `extras/hubble-device-sdk/src/`.
* `src/import_sources.c` — Selectively compiles the correct files from the SDK.


## Maintenance & syncing

This library wraps the functionality provided by the core Hubble Networks SDK located at [HubbleNetwork/hubble-device-sdk](https://github.com/HubbleNetwork/hubble-device-sdk).

Arduino libraries cannot depend on a submodule, so rather than pin the SDK with one, the [Sync Hubble SDK](.github/workflows/sync-sdk.yml) workflow clones the SDK at a given ref and copies in only the files this library compiles (see [Library structure](#library-structure)): the public headers into `src/hubble/`, and the files listed in `extras/hubble-device-sdk-files.txt` (everything `src/import_sources.c` reaches) into `extras/hubble-device-sdk/src/`, where Arduino won't compile it on its own. It is currently pinned at the `v3.0.0` release. To move that pin, run the workflow from the Actions tab with the new ref (branch, tag, or SHA) and merge the pull request it opens. Please [contact us](#contact--support) if there is an issue with this syncing or this library has become outdated.


## Troubleshooting & debugging

**`begin()` fails**

* Base64 key decode failed ⇒ ensure the key is a valid Base64 string and decodes to the expected 32 bytes (`CONFIG_HUBBLE_KEY_SIZE`). The library decodes the key and returns `false` if decoding fails, which a 128-bit key pasted into this 256-bit build will do.
* `hubble_init()` failed ⇒ ensure the core SDK is available and the provided master key / time are correct.
* `hubbleBleBegin()` failed ⇒ check hardware support for ArduinoBLE (or esp32 library) and other libraries that may be holding the BLE controller.

**`setData()` fails**

* `hubble_ble_advertise_get()` returned an error (payload build/crypto failure). Make sure data length is ≤13 bytes and the master key and SDK initialization succeeded.
* It worked for a while, then started failing with `Re-using same nonce is insecure` in the log ⇒ the sketch has used up the 1024 advertisements allowed per 24 hours. Call `setData()` less often. See [Packet rebuild limits](#packet-rebuild-limits).

**`start()` fails / advertising does not appear**

* `hubbleBleAdvertise()` returned false. Verify board BLE support, and that your advertising interval is valid (units: 0.625 ms). If you change the interval, `start()` stops and restarts advertising.

**General debug tips**

* Print return values from `begin()`, `setData()`, and `start()` to serial to isolate where failure occurs.
* Confirm the master key is 32 bytes (after base64 decode). The library is built for AES-256 (`CONFIG_HUBBLE_NETWORK_KEY_256`) and sizes the master key buffer from it. A 16-byte key is rejected here; change `hubble_config.h` to `CONFIG_HUBBLE_NETWORK_KEY_128` if that is what your network issues.


## Contact & support

Maintainer: Micah Morefield `micah@hubble.com`

For support, email `support@hubble.com`

## License

This library is distributed under the **Apache-2.0** license (see `library.properties`).
