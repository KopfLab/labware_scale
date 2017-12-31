#pragma once
#include "DeviceState.h"
#include "ScaleCommands.h"

// scale state
struct ScaleState : public DeviceState {
  int data_logging_period; // period between logs

  ScaleState() {};
  ScaleState(int timezone, bool locked, bool state_logging, bool data_logging, int data_logging_period) :
    DeviceState(timezone, locked, state_logging, data_logging), data_logging_period(data_logging_period) {};
};

/**** textual translations of state values ****/

// read_period
static void getStateDataLoggingPeriodText(int logging_period, char* target, int size, bool value_only = false) {
  getStateIntText(logging_period, CMD_DATA_LOG_PERIOD, target, size, value_only);
}
