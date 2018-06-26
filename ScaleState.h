#pragma once
#include "device/DeviceStateSerial.h"

// additional commands
#define CMD_CALC_RATE          "calc-rate" // device calc-rate <unit> [notes] : whether to calculate rates and if so which time units they have
#define CMD_CALC_RATE_OFF      "off"  // device calc-rate off : do not calculate rate
#define CMD_CALC_RATE_SEC      CMD_TIME_SEC  // device calc-rate s : calculate the rate in units of [mass]/s
#define CMD_CALC_RATE_MIN      CMD_TIME_MIN  // device calc-rate m : calculate the rate in units of [mass]/min
#define CMD_CALC_RATE_HR       CMD_TIME_HR   // device calc-rate h : calculate the rate in units of [mass]/hr
#define CMD_CALC_RATE_DAY      CMD_TIME_DAY  // device calc-rate d : calculate the rate in units of [mass]/day

// state values
#define CALC_RATE_OFF   0
#define CALC_RATE_SEC   1
#define CALC_RATE_MIN   60
#define CALC_RATE_HR    3600
#define CALC_RATE_DAY   86400

// scale state
struct ScaleState : public DeviceStateSerial {

  uint calc_rate;

  ScaleState() {};

  ScaleState (bool locked, bool state_logging, bool data_logging, uint data_reading_period_min, uint data_reading_period, uint data_logging_period, uint8_t data_logging_type, uint calc_ate) :
    DeviceStateSerial(locked, state_logging, data_logging, data_reading_period_min, data_reading_period, data_logging_period, data_logging_type), calc_rate(calc_rate) {};

};

/**** textual translations of state values ****/

// logging_period (any pattern)
static void getStateCalcRateText(uint calc_rate, char* target, int size, char* pattern, bool include_key = true) {
  char units[10];
  if (calc_rate == CALC_RATE_OFF) strcpy(units, CMD_CALC_RATE_OFF);
  else if (calc_rate == CALC_RATE_SEC) strcpy(units, CMD_CALC_RATE_SEC);
  else if (calc_rate == CALC_RATE_MIN) strcpy(units, CMD_CALC_RATE_MIN);
  else if (calc_rate == CALC_RATE_HR) strcpy(units, CMD_CALC_RATE_HR);
  else if (calc_rate == CALC_RATE_DAY) strcpy(units, CMD_CALC_RATE_DAY);
  else {
    strcpy(units, "?");
    Serial.printf("ERROR: unknown calc rate setting %d\n", calc_rate);
  }
  getStateStringText(CMD_CALC_RATE, units, target, size, pattern, include_key);
}

// read period (standard patterns)
static void getStateCalcRateText(uint calc_rate, char* target, int size, bool value_only = false) {
  (value_only) ?
      getStateCalcRateText(calc_rate, target, size, PATTERN_V_SIMPLE, false) :
      getStateCalcRateText(calc_rate, target, size, PATTERN_KV_JSON_QUOTED, true);
}
