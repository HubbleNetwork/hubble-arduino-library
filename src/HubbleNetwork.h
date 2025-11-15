/*
 * Copyright (c) 2024 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HUBBLENETWORK_H
#define HUBBLENETWORK_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <hubble/ble.h>

/**
 * @brief Arduino wrapper for Hubble Network BLE functions.
 *
 * This class is a very small, zero-state wrapper around the C SDK APIs:
 *  - hubble_ble_init
 *  - hubble_ble_advertise_get
 *  - hubble_ble_utc_set
 *  - hubble_ble_key_set
 *
 * All functions return the original SDK `int` return code (0 = success).
 * Convenience boolean helpers are provided (return true on success).
 *
 * Notes:
 *  - The underlying SDK documents that hubble_ble_advertise_get is not
 *    thread-safe or reentrant; callers must ensure proper synchronization.
 */
namespace HubbleNetwork {

  /**
   * Initialize Hubble BLE.
   *
   * @param utc_time UTC time in milliseconds since Unix epoch. Pass 0 if you
   *                 intend to set time later with utcSet().
   * @param key      Pointer to an opaque key object. If nullptr, caller must
   *                 set the key later via keySet().
   * @return         boolean of success
   */
  bool init(uint64_t utc_time, const void *key) {
    return hubble_ble_init(utc_time, key) == 0;
  }

  /**
   * Create an advertisement payload from input data.
   *
   * The SDK writes `out_len` to the actual written length. The caller must
   * provide an output buffer large enough for the expected advertisement (the
   * SDK advertises within the service-data AD structure).
   *
   * Example usage:
   *   uint8_t adv_buf[31];
   *   size_t adv_len = sizeof(adv_buf);
   *   int rc = hubble.advertiseGet(input, input_len, adv_buf, adv_len);
   *
   * @param input     Pointer to input data.
   * @param input_len Length of input data.
   * @param out       Output buffer to receive advertisement data.
   * @param out_len   IN: maximum out size; OUT: actual advertisement length.
   * @return          boolean of success
   */
   bool advertiseGet(const uint8_t *input, size_t input_len,
                   uint8_t *out, size_t *out_len) {
    return hubble_ble_advertise_get(input, input_len, out, out_len) == 0;
  }

  /**
   * Set UTC time (ms).
   *
   * @param utc_time UTC time in milliseconds.
   * @return         boolean of success
   */
  bool utcSet(uint64_t utc_time) {
    return hubble_ble_utc_set(utc_time) == 0;
  }

  /**
   * Set the encryption key used by the SDK.
   *
   * @param key Pointer to opaque key object/bytes expected by the SDK.
   * @return    boolean of success
   */
  bool keySet(const void *key) {
    return hubble_ble_key_set(key) == 0;
  }
}; // namespace HubbleNetwork

#endif // HUBBLENETWORK_H
