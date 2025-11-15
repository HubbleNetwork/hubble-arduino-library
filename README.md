# Hubble Network Arduino Library

The `HubbleNetwork` Arduino library enables Arduino devices to connect to and advertise data to the Hubble Network using BLE. It contains both the core functionality for constructing a Hubble-compatible BLE advertisement and a convenience wrapper that manages BLE for you.


## Contents

* [Prerequisites](#prerequisites)
* [Quick start / Example sketch](#quick-start--example-sketch)
* [API (functions & behavior)](#api-functions--behavior)
* [Installation](#installation)
* [Building and running](#building-and-running)
* [Library structure](#library-structure)
* [Maintenance & syncing](#maintenance--syncing)
* [Versioning & release](#release-and-versioning)
* [Supported architectures](#supported-architectures)
* [Troubleshooting & debugging](#troubleshooting--debugging)
* [Contact & support](#contact-and-support)
* [License](#license)


## Prerequisites

* You must have an organization ID and API token from Hubble, and use those to generate device keys required to connect to the Hubble network.
  * [Get API keys](https://docs.hubble.com/docs/api-specification/hubble-platform-api#api-access)
  * Self-serve sign up flows available soon, until then [Contact Us](https://hubble.com/contact-us)
* This Arduino library depends on:

  * **PSACrypto** (for crypto primitives)
  * **ArduinoBLE** (BLE stack)
    These are required in `library.properties`.
* The Hubble device key provided to `begin()` must be a Base64-encoded key (the library decodes this into a 32-byte master key). See the API notes below.


## Quick start / Example sketch

```cpp
#include <Arduino.h>
#include <HubbleNetwork.h>

// Helper: produce current UTC in seconds since Unix epoch.
// On many Arduinos you must provide real UTC time yourself (RTC, NTP, or host).
uint32_t getUtcMillis() {
  // TODO: replace with real RTC or network-provided epoch ms
  return 1700000000UL; // example epoch seconds
}

const char* DEVICE_KEY_BASE64 = "BASE64_ENCODED_32B_KEY"; // replace with generated device key

void setup() {
  Serial.begin(115200);

  // Initialize Hubble network. Return value indicates success.
  if (!HubbleNetwork.begin(getUtcMillis(), DEVICE_KEY_BASE64)) {
    Serial.println("HubbleNetwork.begin() failed");
    while (1) delay(1000);
  }

  // Example payload (max 13 bytes — see API below)
  uint8_t payload[] = { 0x01, 0x02, 0x03 };
  if (!HubbleNetwork.setData(payload, sizeof(payload))) {
    Serial.println("HubbleNetwork.setData() failed");
  }

  // Start advertising. Default interval units are 0.625ms; default parameter = 3200 (2s).
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

Notes:

* `begin()` expects UTC time **in seconds since Unix epoch** and a **base64-encoded key** string. The library will decode the key and initialize the crypto/ble internals.
* `setData()` accepts a data buffer up to **13 bytes** (the library packages this into the BLE service data).
* `start()` takes an advertising interval parameter in units of **0.625 ms**; the default argument is **3200** (3200 × 0.625 ms = 2000 ms = 2 s).


## API (functions & behavior)

The Arduino wrapper exposes the `HubbleNetwork` class with the following functions:

**`bool HubbleNetwork.begin(uint32_t utc_time, const char *key)`**

* Initialize the Hubble BLE subsystem and core SDK state.
* `utc_time` is UTC in seconds since Unix epoch; `key` is the Base64-encoded device key.
* Returns `true` on success. Failure conditions include invalid base64 key or failure to initialize BLE.

**`bool HubbleNetwork.end(void)`**

* Deinitialize and release BLE. Returns `true` on success.

**`void HubbleNetwork.poll(void)`**

* Call regularly from `loop()` to service the BLE stack and internal operations. If uninitialized, this is a no-op.

**`bool HubbleNetwork.setData(const uint8_t *data, size_t len)`**

* Set the 13-byte (maximum) payload to be advertised. The library constructs the Hubble BLE service data and updates the advertisement. If advertising is active, advertisement is briefly stopped while the new payload is installed and then restarted at the previous interval. Returns `true` on success.
* The data set will be stored and the advertisement updated periodically with my up-to-date encryption credentials.

**`bool HubbleNetwork.start(uint16_t advertisingInterval = ADV_INTERVAL_SLOW)`**

* Start advertising on the Hubble network. `advertisingInterval` is specified in units of 0.625 ms. If already started, `start()` stops and restarts advertising. Returns `true` on success.

**`bool HubbleNetwork.stop(void)`**

* Stop advertising and return `true` on success.


## Installation

1. Ensure dependencies are available in your Arduino environment:

   * Install **ArduinoBLE** (available from the Library Manager).
   * Install **PSACrypto** (or include it as a library dependency).
2. Install this library:

   * Option A — **Arduino Library Manager** (if published): search for `HubbleNetwork` and install.
   * Option B — **Manual**: clone or download this repository and place it under your Arduino `libraries/` folder:
     `~/Arduino/libraries/hubble-arduino-library/`
3. Include the library from your sketch:

```cpp
#include <HubbleNetwork.h>
```

The library metadata (`library.properties`) lists the `depends` libraries and author/maintainer information.


## Building and running

* Provide a valid, Base64-encoded device key (generated from your Hubble organization account / core SDK) to `begin()`.
* Ensure your board supports ArduinoBLE and the hardware BLE implementation. The library will call `BLE.begin()` and fail if the BLE subsystem cannot initialize. Typical failure points are:

  * Invalid base64 key (decoding fails). The implementation decodes the key into a 32-byte master key.
  * `hubble_ble_init()` failing (core SDK initialization).
  * `BLE.begin()` returning false.


## Library structure

* `library.properties` — Library metadata (name, version, dependencies).
* `src/HubbleNetwork.h` — Public Arduino wrapper header with documentation for each function.
* `src/HubbleNetwork.cpp` — Implementation: BLE management, advertisement buffer construction, `begin`/`start`/`stop`/`setData` logic.


## Maintenance & syncing

This library wraps the functionality provided by the core Hubble Networks SDK located at [HubbleNetwork/sdk](https://github.com/HubbleNetwork/sdk).

Due to how Arduino libraries are managed, this SDK is manually synced to this library and headers placed into `src/` where they can be referenced. Please contact support if there is an issue with this syncing or this library has become outdated.


## Supported architectures

* The library metadata lists `architectures=*`, indicating broad architecture support (limited by the dependencies: PSACrypto and ArduinoBLE).


## Troubleshooting & debugging

**`begin()` fails**

* Base64 key decode failed ⇒ ensure the key is a valid Base64 string and decodes to the expected 32 bytes. The library decodes the key and returns `false` if decoding fails.
* `hubble_ble_init()` failed ⇒ ensure the core SDK is available and the provided master key / time are correct.
* `BLE.begin()` failed ⇒ check hardware support for ArduinoBLE and other libraries that may be holding the BLE controller.

**`setData()` fails**

* `hubble_ble_advertise_get()` returned an error (payload build/crypto failure). Make sure data length is ≤13 bytes and the master key and SDK initialization succeeded.

**`start()` fails / advertising does not appear**

* `BLE.advertise()` returned false. Verify board BLE support, and that `BLE.setAdvertisingInterval()` receives a valid interval (units: 0.625 ms). If you change the interval, `start()` stops and restarts advertising.

**General debug tips**

* Print return values from `begin()`, `setData()`, and `start()` to serial to isolate where failure occurs.
* Confirm the master key is 32 bytes (after base64 decode). The implementation stores the master key in a 32-byte buffer.


## Contact & support

Maintainer: Paul Buckley `<paul@hubble.com>`

For support, email `support@hubble.com`

## License

This library is distributed under the **Apache-2.0** license (see `library.properties`).
