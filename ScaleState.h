#pragma once
#include "device/DeviceState.h"
#include "ScaleCommands.h"

// scale state
struct ScaleState : public DeviceState {
  int data_logging_period; // period between logs

  ScaleState() {};
  ScaleState(int timezone, bool locked, bool state_logging, bool data_logging, int data_logging_period) :
    DeviceState(timezone, locked, state_logging, data_logging), data_logging_period(data_logging_period) {};
};

/**** textual translations of state values ****/

// read_period (any pattern)
static void getStateDataLoggingPeriodText(int logging_period, char* target, int size, char* pattern, bool include_key = true) {
  if (logging_period == 0) {
    // manual mode
    getStateStringText(CMD_DATA_LOG_PERIOD, CMD_DATA_LOG_PERIOD_MANUAL, target, size, pattern, include_key);
  } else {
    // specific logging period
    getStateIntText(CMD_DATA_LOG_PERIOD, logging_period, "s", target, size, pattern, include_key);
  }
}

// read period (standard patterns)
static void getStateDataLoggingPeriodText(int logging_period, char* target, int size, bool value_only = false) {
  if (value_only) {
    getStateDataLoggingPeriodText(logging_period, target, size, PATTERN_VU_SIMPLE, false);
  } else {
    (logging_period == 0) ?
      getStateDataLoggingPeriodText(logging_period, target, size, PATTERN_KV_JSON_QUOTED, true) : // manual
      getStateDataLoggingPeriodText(logging_period, target, size, PATTERN_KVU_JSON, true); // number
  }
}
