/*
 * DeviceState.h
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 * Structure that captures the device state. Note that this is trouble as a class or a structure
 * with methods because of issues in EEPROM storage. Methods that modify and save/restore states
 * are implemented in DeviceController.h and derived classes instead.
 */

#pragma once
#include "DeviceCommands.h"
#include "DeviceInfo.h"

// scale state
#define STATE_VERSION    4 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location
#define STATE_VARIABLE   "device_state" // name of the particle exposed state variable

struct DeviceState {
  const int version = STATE_VERSION;
  bool locked = false; // whether state is locked
  bool state_logging = false; // whether state is logged (whenever there is a change)
  bool data_logging = false; // whether data is logged
  int timezone = 0; // what time zone the device is in

  DeviceState() {};
  DeviceState(int timezone, bool locked, bool state_logging, bool data_logging) :
    timezone(timezone), locked(locked), state_logging(state_logging), data_logging(data_logging) {
      Time.zone(timezone);
    };
};

/**** textual translations of state values ****/

// locked text
static void getStateLockedText(bool locked, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_LOCK, locked, CMD_LOCK_ON, CMD_LOCK_OFF, target, size, pattern, include_key);
}
static void getStateLockedText(bool locked, char* target, int size, bool value_only = false) {
  if (value_only) getStateLockedText(locked, target, size, PATTERN_V_SIMPLE, false);
  else getStateLockedText(locked, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// state logging
static void getStateStateLoggingText(bool state_logging, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_STATE_LOG, state_logging, CMD_STATE_LOG_ON, CMD_STATE_LOG_OFF, target, size, pattern, include_key);
}

static void getStateStateLoggingText(bool state_logging, char* target, int size, bool value_only = false) {
  if (value_only) getStateStateLoggingText(state_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateStateLoggingText(state_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// data logging
static void getStateDataLoggingText(bool data_logging, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_DATA_LOG, data_logging, CMD_DATA_LOG_ON, CMD_DATA_LOG_OFF, target, size, pattern, include_key);
}

static void getStateDataLoggingText(bool data_logging, char* target, int size, bool value_only = false) {
  if (value_only) getStateDataLoggingText(data_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateDataLoggingText(data_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// timezone
static void getStateTimezoneText(int tz, char* target, int size, char* pattern, bool include_key = true) {
  getStateIntText(CMD_TIMEZONE, tz, "hrs", target, size, pattern, include_key);
}

static void getStateTimezoneText(int tz, char* target, int size, bool value_only = false) {
  if (value_only) getStateTimezoneText(tz, target, size, PATTERN_VU_SIMPLE, false);
  else getStateTimezoneText(tz, target, size, PATTERN_KVU_JSON, true);
}
