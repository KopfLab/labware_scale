/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include "ScaleState.h"
#include "DeviceCommands.h"
#include "DeviceController.h"

// controller class
class ScaleController : public DeviceController {


  public:

    // state
    ScaleState state;

    // command
    DeviceCommand command;

    // callback
    typedef void (*ScaleCommandCallbackType) (const ScaleController&);

    // constructors
    ScaleController (int reset_pin, ScaleState state, ScaleCommandCallbackType callback) :
        DeviceController(reset_pin), state(state), command_callback(callback) {
      Particle.function(CMD_ROOT, &ScaleController::parseCommand, this);
    }

    // setup and loop methods
    void init() {
      DeviceController::init();
    }
    void update(); // to be run during loop()

    // data logging
    void setReadPeriod(int period);
    void setLogPeriod(int period);

    // commands
    void restoreState() {
      state.load();
    }
    int parseCommand (String command); // parse a cloud command

  // scale command callback
  private: ScaleCommandCallbackType command_callback;

};


// IMPLEMENTATION

// loop function
void ScaleController::update() {

  // FIXME: implement
  // do the data reading and do the data logging if logs are on

}

/**** CHANGING STATE ****/

void ScaleController::setReadPeriod(int period) {
  Serial.println("INFO: setting read period to " + String(period) + " seconds");
  state.read_period = period;
  state.save();
}

void ScaleController::setLogPeriod(int period) {
  Serial.println("INFO: setting log period to " + String(period) + " seconds");
  state.log_period = period;
  state.save();
}


/****** WEB COMMAND PROCESSING *******/

int ScaleController::parseCommand(String command_string) {

  command = DeviceCommand(command_string);
  command.assignVariable();

  // decision tree
  if (command.parseVariable(CMD_LOCK)) {
    // locking
    command.assignValue();
    if (command.parseValue(CMD_LOCK_ON)) {
      command.success(state.changeLocked(true), true);
    } else if (command.parseValue(CMD_LOCK_OFF)) {
      command.success(state.changeLocked(false), true);
    }
  } else if (state.locked) {
    // device is locked --> no other commands allowed
    command.errorLocked();
  } else if (command.parseVariable(CMD_STATE_LOG)) {
    // state logging
    command.assignValue();
    if (command.parseValue(CMD_STATE_LOG_ON)) {
      command.success(state.changeStateLogging(true), true);
    } else if (command.parseValue(CMD_STATE_LOG_OFF)) {
      command.success(state.changeStateLogging(false), true);
    }
  } else if (command.parseVariable(CMD_TIMEZONE)) {
    // timezone
    command.assignValue();
    int tz = atoi(command.value);
    if (tz < -12 || tz > 14) {
      command.error(CMD_RET_ERR_TZ, CMD_RET_ERR_TZ_TEXT);
    }
    command.success(state.changeTimezone(tz));
  } else {
    // other commands
  }

  // finalize command
  command.finalize();

  // command reporting callback
  if (command_callback) {
    command_callback(*this);
  }

  return(command.ret_val);
}
