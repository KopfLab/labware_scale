#pragma once
#include "device/DeviceCommands.h"

// logging rate
#define CMD_DATA_LOG_PERIOD           "log-period" // scale log-period number/manual [notes] : number of seconds between each data logging (if log is on)
  #define CMD_DATA_LOG_PERIOD_MANUAL    "manual"   // to send scale data only when PRINT is pressed manually
