/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include <vector>
#include "ScaleState.h"
#include "device/SerialDeviceController.h"

// serial communication constants
#define SCALE_DATA_REQUEST  "#" // data request command


// controller class
class ScaleController : public SerialDeviceController {

  private:

    // state
    ScaleState* state;
    SerialDeviceState* dss = state;
    DeviceState* ds = state;

    // weight memory for rate calculation
    RunningStats prev_weight1;
    RunningStats prev_weight2;
    unsigned long prev_data_time1;
    unsigned long prev_data_time2;

    // serial communication
    int data_pattern_pos;
    void construct();

  public:

    // constructors
    ScaleController();

    // without LCD
    ScaleController (int reset_pin, const long baud_rate, const long serial_config, const int error_wait, ScaleState* state) :
      SerialDeviceController(reset_pin, baud_rate, serial_config, SCALE_DATA_REQUEST, error_wait), state(state) { construct(); }

    // with LCD
    ScaleController (int reset_pin, DeviceDisplay* lcd, const long baud_rate, const long serial_config, const int error_wait, ScaleState* state) :
      SerialDeviceController(reset_pin, lcd, baud_rate, serial_config, SCALE_DATA_REQUEST, error_wait), state(state) { construct(); }

    // serial
    void startSerialData();
    int processSerialData(byte b);
    void completeSerialData(int error_count);

    // state
    void updateStateInformation();
    void assembleStateInformation();
    DeviceState* getDS() { return(ds); }; // return device state
    SerialDeviceState* getDSS() { return(dss); }; // return device state serial
    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

    // particle commands
    void parseCommand (); // parse a cloud command
    bool changeCalcRate(uint rate);
    bool parseCalcRate();

    // data logging
    bool assembleDataLog();
    void logData();
    void resetData();

    // rate calculations
    void calculateRate();
    void setRateUnits();

};

/**** CONSTRUCTION ****/

void ScaleController::construct() {
  // start data vector
  data.resize(2);
  data[0] = DeviceData(1, "weight");
  data[1] = DeviceData(2, "rate");
  data[1].setAutoClear(false);
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
  SerialDeviceController::startSerialData();
  data_pattern_pos = 0;
}

int ScaleController::processSerialData(byte b) {
  // keep track of all data
  SerialDeviceController::processSerialData(b);

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
      resetData();
      data[0].setUnits(units_buffer);
      setRateUnits();
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

void ScaleController::completeSerialData(int error_count) {
  // weight
  if (error_count == 0) {
    data[0].setNewestValue(value_buffer, true, 2L); // infer decimals and add 2 to improve accuracy of offline calculated rate
    data[0].saveNewestValue(true); // average
  }
}

/****** STATE INFORMATION *******/

void ScaleController::assembleStateInformation() {
  SerialDeviceController::assembleStateInformation();
  char pair[60];
  getStateCalcRateText(state->calc_rate, pair, sizeof(pair)); addToStateInformation(pair);
}

void ScaleController::updateStateInformation() {

  // state information
  DeviceController::updateStateInformation();

  // LCD update
  if (lcd) {

    // latest data
    if (data[0].newest_value_valid)
      getDataDoubleText("Last", data[0].newest_value, data[0].units, lcd_buffer, sizeof(lcd_buffer), PATTERN_KVU_SIMPLE, data[0].decimals - 1);
    else
      strcpy(lcd_buffer, "Last: no data yet");
    lcd->printLine(2, lcd_buffer);

    // running data
    if (data[0].getN() > 0)
      getDataDoubleText("Avg", data[0].getValue(), data[0].units, data[0].getN(),
        lcd_buffer, sizeof(lcd_buffer), PATTERN_KVUN_SIMPLE, data[0].getDecimals());
    else
      strcpy(lcd_buffer, "Avg: no data yet");
    lcd->printLine(3, lcd_buffer);

    // rate
    if (state->calc_rate == CALC_RATE_OFF) {
      if (data[0].getN() > 1)
        getDataDoubleText("SD", data[0].getStdDev(), data[0].units, data[0].getN(),
          lcd_buffer, sizeof(lcd_buffer), PATTERN_KVUN_SIMPLE, data[0].getDecimals());
      else
        strcpy(lcd_buffer, "SD: not enough data");
      lcd->printLine(4, lcd_buffer);
    } else {
      if (data[1].newest_value_valid)
        getDataDoubleText("Rate", data[1].newest_value, data[1].units, lcd_buffer, sizeof(lcd_buffer), PATTERN_KVU_SIMPLE, data[1].decimals);
      else
        strcpy(lcd_buffer, "Rate: not enough data");
      lcd->printLine(4, lcd_buffer);
    }

  }

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

  SerialDeviceController::parseCommand();

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
    setRateUnits(); // update units
    calculateRate(); // recalculate rate
    updateDataInformation(); // update data information
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

/** DATA **/

bool ScaleController::assembleDataLog() { SerialDeviceController::assembleDataLog(false); }

void ScaleController::logData() {
  // calculate rate every time the weight data is logged
  prev_weight2 = prev_weight1;
  prev_data_time2 = prev_data_time1;
  prev_weight1 = data[0].value;
  prev_data_time1 = data[0].data_time.getMean();
  calculateRate();
  SerialDeviceController::logData();
}

void ScaleController::resetData() {
  SerialDeviceController::resetData();
  // also reset stored weight data
  prev_weight1.clear();
  prev_weight2.clear();
}

/**** RATE CALCULATIONS ****/

void ScaleController::calculateRate() {

  if (state->calc_rate == CALC_RATE_OFF || prev_weight1.getN() == 0 || prev_weight2.getN() == 0) {
    // no rate calculation OR not enough data for rate calculation, make sure to clear
    data[1].clear(true);
  } else {
    // calculate rate
    double time_diff = (double) prev_data_time1 - (double) prev_data_time2;
    if (state->calc_rate == CALC_RATE_SEC) time_diff = time_diff / 1000.;
    else if (state->calc_rate == CALC_RATE_MIN) time_diff = time_diff / 1000. / 60.;
    else if (state->calc_rate == CALC_RATE_HR) time_diff = time_diff / 1000. / 60. / 60.;
    else if (state->calc_rate == CALC_RATE_DAY) time_diff = time_diff / 1000. / 60. / 60. / 24.;
    double rate = (prev_weight1.getMean() - prev_weight2.getMean()) / time_diff;
    data[1].setNewestValue(rate);

    // calculate mean data time
    unsigned long data_time = (unsigned long) round( 0.5 * (double) prev_data_time1 + 0.5 * (double) prev_data_time2 );
    data[1].setNewestDataTime(data_time);
    data[1].saveNewestValue(false);
    data[1].value.n = prev_weight1.getN() + prev_weight2.getN();

    // set decimals to 4 significant digits
    data[1].setDecimals(find_signif_decimals (rate, 5, false, 6));
  }
}

void ScaleController::setRateUnits() {
  char rate_units[10];
  strncpy(rate_units, data[0].units, sizeof(rate_units) - 1);
  strcpy(rate_units + strlen(data[0].units), "/");
  getStateCalcRateText(state->calc_rate, rate_units + strlen(data[0].units) + 1, sizeof(rate_units), true);
  rate_units[sizeof(rate_units) - 1] = 0;
  data[1].setUnits(rate_units);
}
