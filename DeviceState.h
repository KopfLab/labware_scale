/*
 * DeviceState.h
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 * Structure that captures the device state. Note that this is trouble as a class or a structure
 * with methods because of issues in EEPROM storage. Methods that modify and save/restore states
 * are implemented in DeviceController.h and derived classes instead.
 */

#pragma once
#include "DeviceCommands.h"

// scale state
#define STATE_VERSION    4 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location
#define STATE_VARIABLE   "state" // name of the particle exposed state variable

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

// NOTE: size is always passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)

// generate generic key value pair
static void getStateKeyValuePair(char* target, int size, char* key, char* value, bool quote_value = true, bool include_key = true, char* sep = ":") {
  // NOTE: consider implementing better error catching for overlong key/value pairs
  if (include_key && quote_value) {
    snprintf(target, size, "\"%s\"%s\"%s\"", key, sep, value);
  } else if (include_key && !quote_value) {
    snprintf(target, size, "\"%s\"%s%s", key, sep, value);
  } else if (!include_key && quote_value) {
    snprintf(target, size, "\"%s\"", value);
  } else if (!include_key && !quote_value) {
    snprintf(target, size, "%s", value);
  }
}

// helper function to assemble boolean state text
static void getStateBooleanText(bool value, char* key, char* value_true, char* value_false, char* target, int size, bool value_only = false) {
  char value_text[20];
  value_text[sizeof(value_text) - 1] = 0; // make sure last index is null pointer just to be extra safe
  value ? strncpy(value_text, value_true, sizeof(value_text) - 1) : strncpy(value_text, value_false, sizeof(value_text) - 1);
  getStateKeyValuePair(target, size, key, value_text, !value_only, !value_only);
}

// helper function to assemble integer state text
static void getStateIntText(int value, char* key, char* target, int size, bool value_only = false) {
  char value_text[4];
  snprintf(value_text, sizeof(value_text), "%d", value);
  getStateKeyValuePair(target, size, key, value_text, false, !value_only);
}

// locked text
static void getStateLockedText(bool locked, char* target, int size, bool value_only = false) {
  getStateBooleanText(locked, CMD_LOCK, CMD_LOCK_ON, CMD_LOCK_OFF, target, size, value_only);
}

// state logging
static void getStateStateLoggingText(bool state_logging, char* target, int size, bool value_only = false) {
  getStateBooleanText(state_logging, CMD_STATE_LOG, CMD_STATE_LOG_ON, CMD_STATE_LOG_OFF, target, size, value_only);
}

// data logging
static void getStateDataLoggingText(bool data_logging, char* target, int size, bool value_only = false) {
  getStateBooleanText(data_logging, CMD_DATA_LOG, CMD_DATA_LOG_ON, CMD_DATA_LOG_OFF, target, size, value_only);
}

// timezone
static void getStateTimezoneText(int tz, char* target, int size, bool value_only = false) {
  getStateIntText(tz, CMD_TIMEZONE, target, size, value_only);
}
