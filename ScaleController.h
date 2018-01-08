/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include "ScaleState.h"
#include "ScaleCommands.h"
#include "ScaleData.h"
#include "device/DeviceControllerSerial.h"

// serial communication constants
#define SCALE_DATA_REQUEST  "#" // data request command


// controller class
class ScaleController : public DeviceControllerSerial {

  private:

    // state
    ScaleState *state;
    DeviceState *ds = state;

    // data
    ScaleData *data;

    /**** serial communication *****/
    // pattern pieces
    const int P_VAL = -1; // [ +-0-9]
    const int P_UNIT = -2; // [GOC]
    const int P_STABLE = -3; // [ S]
    const int P_BYTE = 0; // anything > 0 is specific byte

    // specific ascii characters (actual byte values)
    const int B_SPACE = 32; // \\s
    const int B_CR = 13; // \r
    const int B_NL = 10; // \n
    const int B_0 = 48; // 0
    const int B_9 = 57; // 9

    // overall data pattern
    int data_pattern[20];
    int data_pattern_size;
    int data_pattern_pos;

  public:

    // constructors
    ScaleController();
    ScaleController (int reset_pin, const long baud_rate, const long serial_config, const int request_wait, const int error_wait, ScaleState *state, ScaleData *data) :
      DeviceControllerSerial(reset_pin, baud_rate, serial_config, "#", request_wait, error_wait), state(state), data(data) {
        const int pattern[] = {P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, B_SPACE, B_SPACE, P_UNIT, P_STABLE, B_CR, B_NL};
        data_pattern_size = sizeof(pattern) / sizeof(pattern[0]) - 1;
        for (int i=0; i <= data_pattern_size; i++) data_pattern[i] = pattern[i];
      }

    // setup and loop methods
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

/**** SETUP & LOOP ****/



/**** SERIAL COMMUNICATION ****/

bool ScaleController::serialIsManual(){
  return(state->data_logging_period == 0);
}

void ScaleController::startSerialData() {
  data->resetBuffers();
  data->storeReceivedDataTime(millis());
  data_pattern_pos = 0;
}

int ScaleController::processSerialData(byte b) {

  char c = (char) b;

  if ( data_pattern[data_pattern_pos] == P_VAL && ( (b >= B_0 && b <= B_9) || c == ' ' || c == '+' || c == '-' || c == '.') ) {
    // value (ignoring spaces)
    if (b != B_SPACE) data->appendToValueBuffer(b);
  } else if (data_pattern[data_pattern_pos] == P_UNIT && (c == 'G' || c == 'O' || c == 'C')) {
    // units
    if (c == 'G') data->setUnitsBuffer("g"); // grams
    else if (c == 'O') data->setUnitsBuffer("oz"); // ounces
    else if (c == 'C') data->setUnitsBuffer("ct"); // what is ct??
    if (!data->compareUnits()) {
      // new units!
      data->newValue();
      data->setUnits(data->units_buffer);
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
  data->storeValue();
  data->setValue(true); // average
  data->resetBuffers();
  data->assembleLog();
}

/*
// loop function
void ScaleController::update() {
  DeviceController::update();

  // FIXME: some of the stuff after this could be abstracted into a new DeviceControllerSerial class

  // check serial communication
  while (Serial1.available()) {
    byte b = Serial1.read();


    // if error --> allow time to empty the serial buffer
    if (data_error) last_error = millis();

    // if not waiting for response
    if (data_error || !waiting_for_response) continue;

    // proces byte
    if (!processSerialData(b)) {
      data_error = true;
      Serial.print("WARNING - unexpected character at pattern_pos " + String(pattern_pos) + ": ");
      Serial.print(b);
      Serial.print(" = ");
      Serial.println((char) b);
    }

    // message complete
    if (pattern_pos == pattern_size) {
      data_received = true;
    }
  }

  // message completely received
  if (waiting_for_response && data_received) {
    Serial.println("INFO: data reading complete");
    data->storeValue();
    data->setValue(true); // average
    data->resetBuffers();
    data->assembleLog();
    waiting_for_response = false;
  }

  // error
  if (waiting_for_response && data_error) {
    Serial.println("INFO: encountered data error -- resetting");
    data->resetBuffers();
    last_error = millis();
    temp_message = "";
    waiting_for_response = false;
  }

  // data request: if not currently waiting for a response and
  //   - either there was an error and and it's past the error reset time
  //   - or if it's data logging time
  //   - or if it's on manual request mode
  bool request_data =
    !waiting_for_response &&
    (
      (data_error && millis() - last_error > ERROR_RESET_TIME) ||
      (state->data_logging_period > 0 && millis() > last_read + read_period) ||
      (state->data_logging_period == 0)
    );


  // (re)-request information from scale
  if (request_data){

    // datetime
    char date_time_buffer[21];
    Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
    Serial.print("INFO: ");
    Serial.print(date_time_buffer);
    if (state->data_logging_period == 0) {
      // manual logging?
      Serial.println(" - listening to manual data transmission from scale...");
    } else {
      // data request
      Serial.println(" - issuing request for data to scale.");
      Serial1.println(SCALE_DATA_REQUEST);
    }

    // request parameters
    last_read = millis();
    waiting_for_response = true;
    data_received = false;
    data_error = false;
    pattern_pos = 0;
  }
}
*/

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
