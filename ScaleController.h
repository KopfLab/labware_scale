/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include "ScaleState.h"
#include "ScaleCommands.h"
#include "device/DeviceController.h"

// controller class
class ScaleController : public DeviceController {

  private:

    // state
    ScaleState *state;
    DeviceState *ds = state;

  public:

    // constructors
    ScaleController();
    ScaleController (int reset_pin, ScaleState *state) : DeviceController(reset_pin), state(state) {}

    // setup and loop methods
    void init();
    void update(); // to be run during loop()

    // state
    DeviceState* getDS() { return(ds); }; // return device state
    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

    // state changes and corresponding commands
    bool changeDataLoggingPeriod(int period);
    bool parseDataLoggingPeriod();
    void assembleStateInformation();

    // particle command
    void parseCommand (); // parse a cloud command
};

/**** SETUP & LOOP ****/

// init function
void ScaleController::init() {
  DeviceController::init();
}

// loop function
void ScaleController::update() {
  DeviceController::update();
  // FIXME: implement
  // do the data reading and do the data logging if logs are on
}

/**** STATE PERSISTENCE ****/

// save device state to EEPROM
void ScaleController::saveDS() {
  EEPROM.put(STATE_ADDRESS, *state);
  Serial.println("INFO: scale state saved in memory (if any updates were necessary)");
}

// load device state from EEPROM
bool ScaleController::restoreDS(){
  ScaleState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  bool recoverable = saved_state.version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, *state);
    Serial.println("INFO: successfully restored state from memory (version " + String(STATE_VERSION) + ")");
  } else {
    Serial.println("INFO: could not restore state from memory (found version " + String(saved_state.version) + "), sticking with initial default");
    saveDS();
  }
  return(recoverable);
}

/**** CHANGING STATE ****/

// read period
bool ScaleController::changeDataLoggingPeriod(int period) {
  bool changed = period != state->data_logging_period;

  if (changed) {
    state->data_logging_period = period;
    Serial.println("INFO: setting data logging period to " + String(period) + " seconds");
    saveDS();
  } else {
    Serial.println("INFO: data logging period unchanged (" + String(period) + ")");
  }
  return(changed);
}

bool ScaleController::parseDataLoggingPeriod() {
  if (command.parseVariable(CMD_DATA_LOG_PERIOD)) {
    // parse read period
    command.extractValue();
    if (command.parseValue(CMD_DATA_LOG_PERIOD_MANUAL)){
      // manual logging
      command.success(changeDataLoggingPeriod(0));
    } else {
      int period = atoi(command.value);
      if (period > 0) {
        command.success(changeDataLoggingPeriod(period));
        strcpy(command.units, "seconds");
      } else {
        command.errorValue();
      }
    }
    getStateDataLoggingPeriodText(state->data_logging_period, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}

/******  STATE INFORMATION *******/

void ScaleController::assembleStateInformation() {
  DeviceController::assembleStateInformation();
  char pair[60];
  getStateDataLoggingPeriodText(state->data_logging_period, pair, sizeof(pair)); addToStateInformation(pair);
  // FIXME: continue here
}

/****** WEB COMMAND PROCESSING *******/

void ScaleController::parseCommand() {

  // decision tree
  if (parseLocked()) {
    // locked is getting parsed
  } else if (parseStateLogging()) {
    // state logging getting parsed
  } else if (parseDataLogging()) {
    // state logging getting parsed
  } else if (parseTimezone()) {
    // time zone getting parsed
  } else if (parseDataLoggingPeriod()) {
    // parsing read period
  } else {
    // other commands
    // FIXME: continue here
  }

}
