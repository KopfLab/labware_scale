/*
 * DeviceState.h
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 * Structure that captures the device state. Note that this is trouble as a class or a structure
 * with methods because of issues in EEPROM storage. Methods that modify and save/restore states
 * are implemented in DeviceController.h and derived classes instead.
 */

#pragma once

// scale state
#define STATE_VERSION    3 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location

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
