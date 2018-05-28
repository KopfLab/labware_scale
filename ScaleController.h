/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include <vector>
#include "ScaleState.h"
#include "ScaleCommands.h"
#include "device/DeviceControllerSerial.h"

// serial communication constants
#define SCALE_DATA_REQUEST  "#" // data request command


// controller class
class ScaleController : public DeviceControllerSerial {

  private:

    // state
    ScaleState* state;
    DeviceState* ds = state;

    // serial communication
    int data_pattern_pos;
    unsigned long last_data_log;
    void construct(const int digits);

  public:

    // constructors
    ScaleController();

    // without LCD
    ScaleController (int reset_pin, const long baud_rate, const long serial_config, const int request_wait, const int error_wait, const int digits, ScaleState* state) :
      DeviceControllerSerial(reset_pin, baud_rate, serial_config, "#", request_wait, error_wait), state(state) { construct(digits); }

    // with LCD
    ScaleController (int reset_pin, DeviceDisplay* lcd, const long baud_rate, const long serial_config, const int request_wait, const int error_wait, const int digits, ScaleState* state) :
      DeviceControllerSerial(reset_pin, lcd, baud_rate, serial_config, "#", request_wait, error_wait), state(state) { construct(digits); }

    // setup and loop
    void init();

    // check whether time for data reset and log
    bool isTimeForDataReset();

    // serial
    bool serialIsManual();
    void startSerialData();
    int processSerialData(byte b);
    void completeSerialData();

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

/**** CONSTRUCTION ****/

void ScaleController::construct(const int digits) {
  // start data vector
  data.resize(1);
  data[0] = DeviceData("weight", digits);
}

/**** SETUP & LOOP ****/

void ScaleController::init() {
  DeviceControllerSerial::init();
  last_data_log = millis();
}

/**** DATA LOGGING ****/

bool ScaleController::isTimeForDataReset() {
  unsigned long log_period = state->data_logging_period * 1000;
  if (serialIsActive() && !serialIsManual() && (millis() - last_data_log) > log_period) {
    last_data_log = millis();
    return(true);
  }
  return(false);
}

/**** SERIAL COMMUNICATION ****/

// pattern pieces
#define P_VAL       -1 // [ +-0-9]
#define P_UNIT      -2 // [GOC]
#define P_STABLE    -3 // [ S]
#define P_BYTE       0 // anything > 0 is specific byte

// specific ascii characters (actual byte values)
#define B_SPACE      32 // \\s
#define B_CR         13 // \r
#define B_NL         10 // \n
#define B_0          48 // 0
#define B_9          57 // 9

const int data_pattern[] = {P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, B_SPACE, B_SPACE, P_UNIT, P_STABLE, B_CR, B_NL};
const int data_pattern_size = sizeof(data_pattern) / sizeof(data_pattern[0]) - 1;

bool ScaleController::serialIsManual(){
  return(state->data_logging_period == 0);
}

void ScaleController::startSerialData() {
  DeviceControllerSerial::startSerialData();
  data_pattern_pos = 0;
}

int ScaleController::processSerialData(byte b) {
  // keep track of all data
  DeviceControllerSerial::processSerialData(b);

  // pattern interpretation
  char c = (char) b;
  if ( data_pattern[data_pattern_pos] == P_VAL && ( (b >= B_0 && b <= B_9) || c == ' ' || c == '+' || c == '-' || c == '.') ) {
    if (b != B_SPACE) appendToSerialValueBuffer(b); // value (ignoring spaces)
  } else if (data_pattern[data_pattern_pos] == P_UNIT && (c == 'G' || c == 'O' || c == 'C')) {
    // units
    if (c == 'G') setSerialUnitsBuffer("g"); // grams
    else if (c == 'O') setSerialUnitsBuffer("oz"); // ounces
    else if (c == 'C') setSerialUnitsBuffer("ct"); // what is ct??
    if (!data[0].isUnitsIdentical(units_buffer)) {
      // units are switching
      data[0].resetValue();
      data[0].setUnits(units_buffer);
    }
  } else if (data_pattern[data_pattern_pos] == P_STABLE && (c == 'S' || c == ' ')) {
    // whether the reading is stable - note: not currently interpreted
  } else if (data_pattern[data_pattern_pos] > P_BYTE && b == data_pattern[data_pattern_pos]) {
    // specific ascii characters
  } else {
    // unrecognized part of data --> error
    return(SERIAL_DATA_ERROR);
  }

  // next data pattern position
  data_pattern_pos++;

  // message complete
  if (data_pattern_pos > data_pattern_size) {
    return(SERIAL_DATA_COMPLETE);
  }
  return(SERIAL_DATA_WAITING);
}

void ScaleController::completeSerialData() {
  data[0].setNewestValue(value_buffer);
  data[0].saveNewestValue(true); // average
  DeviceControllerSerial::completeSerialData();
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
  } else if (parseDataLoggingPeriod()) {
    // parsing read period
  } else {
    // other commands
    // FIXME: continue here
  }

}
