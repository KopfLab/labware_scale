/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include <vector>
#include "ScaleState.h"
#include "device/DeviceControllerSerial.h"

// serial communication constants
#define SCALE_DATA_REQUEST  "#" // data request command


// controller class
class ScaleController : public DeviceControllerSerial {

  private:

    // state
    ScaleState* state;
    DeviceStateSerial* dss = state;
    DeviceState* ds = state;

    // serial communication
    int data_pattern_pos;
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

    // serial
    void startSerialData();
    int processSerialData(byte b);
    void completeSerialData();

    // state
    void assembleStateInformation();
    DeviceState* getDS() { return(ds); }; // return device state
    DeviceStateSerial* getDSS() { return(dss); }; // return device state serial
    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

    // particle commands
    void parseCommand (); // parse a cloud command
    bool changeCalcRate(uint rate);
    bool parseCalcRate();

};

/**** CONSTRUCTION ****/

void ScaleController::construct(const int digits) {
  // start data vector
  data.resize(1);
  data[0] = DeviceData("weight", digits);
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

/****** STATE INFORMATION *******/

void ScaleController::assembleStateInformation() {
  DeviceControllerSerial::assembleStateInformation();
  char pair[60];
  getStateCalcRateText(state->calc_rate, pair, sizeof(pair)); addToStateInformation(pair);
}


/**** STATE PERSISTENCE ****/

// save device state to EEPROM
void ScaleController::saveDS() {
  EEPROM.put(STATE_ADDRESS, *state);
  #ifdef STATE_DEBUG_ON
    Serial.println("INFO: scale state saved in memory (if any updates were necessary)");
  #endif
}

// load device state from EEPROM
bool ScaleController::restoreDS(){
  ScaleState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  bool recoverable = saved_state.version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, *state);
    Serial.printf("INFO: successfully restored state from memory (version %d)\n", STATE_VERSION);
  } else {
    Serial.printf("INFO: could not restore state from memory (found version %d), sticking with initial default\n", saved_state.version);
    saveDS();
  }
  return(recoverable);
}


/****** WEB COMMAND PROCESSING *******/

void ScaleController::parseCommand() {

  DeviceControllerSerial::parseCommand();

  if (command.isTypeDefined()) {
    // command processed successfully by parent function
  } else if (parseCalcRate()) {
    // calc rate command parsed
  }

  // more additional, device specific commands

}

bool ScaleController::changeCalcRate(uint rate) {

  bool changed = rate != state->calc_rate;

  if (changed) {
    state->calc_rate = rate;
    resetData(); // make sure no data is averaged with different calc-rate settings
  }

  #ifdef STATE_DEBUG_ON
    if (changed) Serial.printf("INFO: setting rate to %d\n", rate);
    else Serial.printf("INFO: rate unchanged (%d)\n", rate);
  #endif

  if (changed) saveDS();

  return(changed);
}

bool ScaleController::parseCalcRate() {
  if (command.parseVariable(CMD_CALC_RATE)) {
    // parse calc rate
    command.extractValue();
    if (command.parseValue(CMD_CALC_RATE_OFF)){
      // no rate calculation
      command.success(changeCalcRate(CALC_RATE_OFF));
    } else if (command.parseValue(CMD_CALC_RATE_SEC)) {
      // [mass]/second
      command.success(changeCalcRate(CALC_RATE_SEC));
    } else if (command.parseValue(CMD_CALC_RATE_MIN)) {
      // [mass]/minute
      command.success(changeCalcRate(CALC_RATE_MIN));
    } else if (command.parseValue(CMD_CALC_RATE_HR)) {
      // [mass]/hour
      command.success(changeCalcRate(CALC_RATE_HR));
    } else if (command.parseValue(CMD_CALC_RATE_DAY)) {
      // [mass]/day
      command.success(changeCalcRate(CALC_RATE_DAY));
    } else {
      // invalid value
      command.errorValue();
    }
    getStateCalcRateText(state->calc_rate, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}
