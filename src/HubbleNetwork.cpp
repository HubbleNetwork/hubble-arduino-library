#include <HubbleNetwork.h>
#include <Arduino.h>
#include <ArduinoBLE.h>
#include "hubble/ble.h"
#include "b64.h"

#include <stdint.h>
#include <stddef.h>


HubbleNetworkClass HubbleNetwork;

#define BLE_ADV_LEN                   31 // Max length
#define AD_TYPE_COMPLETE_16BIT_UUIDS  0x03
#define AD_TYPE_SERVICE_DATA          0x16
// Advertising segments go [len, type, data] - we construct this for 2 types:
// 1. AD_TYPE_COMPLETE_16BIT_UUIDS
// 2. AD_TYPE_SERVICE_DATA
// For AD_TYPE_COMPLETE_16BIT_UUIDS we know the length, but for AD_TYPE_SERVICE_DATA
// this will be overwritten later as well as the data set
#define HUBBLE_BLE_ADV_HEADER \
  0x03 , AD_TYPE_COMPLETE_16BIT_UUIDS, 0xA6, 0xFC, /* Hubble UUID */ \
  0x01, AD_TYPE_SERVICE_DATA,
#define HUBBLE_BLE_ADV_HEADER_SIZE      6
#define HUBBLE_BLE_ADV_SVC_DATA_LEN_IDX 4

// Period where we force updates of advertisements
#define ADV_UPDATE_INTERVAL_MS (1000 * 60 * 60)


bool HubbleNetworkClass::begin(uint32_t utc_time, const char *key)
{
  int status;

  if (b64_decoded_size(key) != HUBBLE_NETWORK_KEY_LEN) {
    return false;
  }

  status = b64_decode(key, this->masterKey, sizeof(this->masterKey));
  if (status != 0) {
    return false;
  }

  // Convert to milliseconds for the API
  uint64_t utc_time_ms = (uint64_t)utc_time * 1000;

  status = hubble_ble_init(utc_time_ms, this->masterKey);
  if (status != 0) {
    return false;
  }

  if (!BLE.begin()) {
    return false;
  }

  BLEAdvertisingData emptyScan;
  BLE.setScanResponseData(emptyScan);

  // Seed the advertisement with some null data
  this->state = HubbleNetworkClass::STOPPED;
  return setData(NULL, 0);
}

bool HubbleNetworkClass::end(void)
{
  BLE.end();
  return true;
}

void HubbleNetworkClass::poll(void)
{
  if (this->state == HubbleNetworkClass::NOINIT) {
    return;
  }
  BLE.poll();

  // If currently advertising, we need to periodically update the
  // advertisement payload to maintain valid encryption/EIDs
  if ((this->lastUpdateTime + ADV_UPDATE_INTERVAL_MS) < millis()) {
    this->setData(this->data, this->dataLen);
  }
}

bool HubbleNetworkClass::setData(const uint8_t *data, size_t len)
{
  if (this->state == HubbleNetworkClass::NOINIT) {
    return false;
  }

  if (len > HUBBLE_NETWORK_MAX_DATA_LEN) {
    return false;
  }

  // Copy data over so we can manually update the advertisement later
  memcpy(this->data, data, len);
  this->dataLen;

  static uint8_t adv_buf[BLE_ADV_LEN] = {HUBBLE_BLE_ADV_HEADER};
  size_t out_len = BLE_ADV_LEN - HUBBLE_BLE_ADV_HEADER_SIZE;

  int status = hubble_ble_advertise_get(
    this->data, len, &adv_buf[HUBBLE_BLE_ADV_HEADER_SIZE], &out_len);
  if (status != 0) {
    return false;
  }
  adv_buf[HUBBLE_BLE_ADV_SVC_DATA_LEN_IDX] = out_len + 1;

  BLEAdvertisingData adv_data;
  if (!adv_data.setRawData(adv_buf, out_len + HUBBLE_BLE_ADV_HEADER_SIZE + 1)) {
    return false;
  }

  bool was_stopped = false;
  if (this->state == HubbleNetworkClass::STARTED) {
    stop();
    was_stopped = true;
  }

  BLE.setAdvertisingData(adv_data);

  if (was_stopped) {
    start(this->advertisingInterval);
  }

  this->lastUpdateTime = millis();

  return true;
}

bool HubbleNetworkClass::setData(const char *str)
{
  this->setData((const uint8_t *)str, strlen(str));
}

bool HubbleNetworkClass::start(uint16_t advertisingInterval)
{
  this->advertisingInterval = advertisingInterval;
  if (this->state == HubbleNetworkClass::NOINIT) {
    return false;
  }

  if (this->state == HubbleNetworkClass::STARTED) {
    stop();
  }
  BLE.setAdvertisingInterval(advertisingInterval);
  if (!BLE.advertise()) {
    return false;
  }
  this->state = HubbleNetworkClass::STARTED;
  return true;
}

bool HubbleNetworkClass::stop(void)
{
  if (this->state == HubbleNetworkClass::NOINIT) {
    return false;
  }

  if (this->state == HubbleNetworkClass::STOPPED) {
    return true;
  }

  BLE.stopAdvertise();

  this->state = HubbleNetworkClass::STOPPED;
  return true;
}
