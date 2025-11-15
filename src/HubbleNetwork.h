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
#include "hubble/ble.h"


/**
 * @brief Arduino wrappers for Hubble network devices.
 *
 * This class provides both a wrapper for core SDK functionality
 * as well as a convenience wrapper on top of that which handles all
 * of the BLE functionality for you.
 */
class HubbleNetworkClass {
public:
  /**
   * @brief Some standard advertising intervals
   */
  static constexpr uint16_t ADV_INTERVAL_FAST = 0x00a0; // 100ms
  static constexpr uint16_t ADV_INTERVAL_SLOW = 0x0640; // 1s

  /**
   * @brief Maximum length of data in bytes
   */
  static constexpr size_t HUBBLE_NETWORK_MAX_DATA_LEN = 13;

  /**
   * @brief Key length in bytes
   */
  static constexpr size_t HUBBLE_NETWORK_KEY_LEN = 32;

  /**
   * Initialize Hubble BLE.
   *
   * This will initialize the BLE subsystem, so ensure nothing
   * else is using BLE before this is called.
   *
   * @param utc_time UTC time in seconds since Unix epoch.
   * @param key      Base64 encoded key string.
   * @return         boolean of success.
   */
  bool begin(uint32_t utc_time, const char *key);

  /**
   * Deinitialize Hubble BLE.
   *
   * Releases all holds on the BLE subsystem
   */
  bool end(void);

  /**
   * Run in your loop() loop.
   */
  void poll(void);

  /**
   * Send data over the Hubble network.
   *
   * This will package the data to be advertised by the device.
   * Note that this does not guarantee the reception of the data by the
   * network.
   *
   * @param data Data to transmit (maximum 13 bytes).
   * @param len  Length of data.
   * @return     boolean of success.
   */
  bool setData(const uint8_t *data, size_t len);
  bool setData(const char *str);

  /**
   * Start advertising on the network.
   *
   * Begins advertising on the Hubble network with the given
   * advertising interval. If you wish to change the interval,
   * you must stop and start advertising.
   * If no data has been set before advertising starts, no payload
   * will be present in the received packet on the backend.
   *
   * @param advertisingInterval Advertising in units of 0.625ms.
   * @return     boolean of success.
   */
  bool start(uint16_t advertisingInterval = ADV_INTERVAL_SLOW);
  bool stop(void);

private:
  enum State {NOINIT, STOPPED, STARTED};

  volatile enum State state = NOINIT;
  uint16_t advertisingInterval;
  uint8_t masterKey[HUBBLE_NETWORK_KEY_LEN];
  size_t dataLen;
  uint8_t data[HUBBLE_NETWORK_MAX_DATA_LEN];
  uint32_t lastUpdateTime;
}; // class HubbleNetworkClass

extern HubbleNetworkClass HubbleNetwork;

#endif // HUBBLENETWORK_H
