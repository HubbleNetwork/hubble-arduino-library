/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The Hubble SDK is in extras/ and must be manually added to the library,
 * as it is not compiled automatically. Any SDK file included here (directly
 * or through another SDK file) must be listed in
 * extras/hubble-device-sdk-files.txt, or the sync workflow will drop it.
 *
 * Multiple crypto backends are available. This file determines which one
 * should be compiled. If none are available, a #error is triggered.
 */

/* Include config file first. Ensures files can access the set configs. */
#include "hubble_config.h"

#include "../extras/hubble-device-sdk/src/hubble.c"
#include "../extras/hubble-device-sdk/src/hubble_ble.c"
#include "../extras/hubble-device-sdk/src/hubble_crypto.c"

#if defined(ARDUINO_ARCH_MBED)
/*
 * Mbed OS carries the whole of MbedTLS and no PSA, so it takes MbedTLS.
 * MBEDTLS_CIPHER_MODE_CTR is commented out in the stock mbedtls/config.h.
 * It is turned on here to match the prebuilt libmbed.a, which has it on.
 */
#ifndef MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_CIPHER_MODE_CTR
#endif

/* Gives visibility to config used below */
#include <mbedtls/cipher.h>

#if !defined(MBEDTLS_CMAC_C)
#error "No AES-CMAC found in MbedTLS."
#endif

#include "../extras/hubble-device-sdk/src/crypto/mbedtls.c"

#else // defined(ARDUINO_ARCH_MBED)

/* Default to using PSA Crypto. */
#ifdef __has_include
#if !__has_include(<psa/crypto.h>)
#error "No Hubble crypto backend is currently supported for this Arduino core."
#endif
#endif

/* Gives visibility to configs used below */
#include <psa/crypto.h>


#if !defined(PSA_WANT_ALG_CMAC) || !defined(PSA_WANT_ALG_CTR)
#error "No AES-CMAC or no AES-CTR found in PSA Crypto."
#endif

#include "../extras/hubble-device-sdk/src/crypto/psa.c"

#endif
