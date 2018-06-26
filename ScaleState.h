#pragma once
#include "device/DeviceStateSerial.h"

// scale state
struct ScaleState : public DeviceStateSerial {

  ScaleState() {};

  ScaleState (bool locked, bool state_logging, bool data_logging, uint data_reading_period_min, uint data_reading_period, uint data_logging_period, uint8_t data_logging_type) :
    DeviceStateSerial(locked, state_logging, data_logging, data_reading_period_min, data_reading_period, data_logging_period, data_logging_type) {};

};
