/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include "ScaleState.h"
#include "ScaleCommands.h"
#include "DeviceController.h"

// controller class
class ScaleController : public DeviceController {


  public:

    // state
    ScaleState state;
    DeviceState *ds = &state;

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

    // state
    DeviceState* getDS() { return(ds); };

    // save device state to EEPROM
    void saveDS() {
      Serial.println("DERIVED SAVE DS");
      EEPROM.put(STATE_ADDRESS, state);
      Serial.println("INFO: scale state saved in memory (if any updates were necessary)");
    }
    
    // load device state from EEPROM
    bool restoreDS(){
      Serial.println("DERIVED LOAD DS");
      ScaleState saved_state;
      EEPROM.get(STATE_ADDRESS, saved_state);
      bool recoverable = saved_state.version == STATE_VERSION;
      if(recoverable) {
        EEPROM.get(STATE_ADDRESS, state);
        Serial.println("INFO: successfully restored state from memory (version " + String(STATE_VERSION) + ")");
      } else {
        Serial.println("INFO: could not restore state from memory (found version " + String(saved_state.version) + "), sticking with initial default");
        saveDS();
      }
      return(recoverable);
    }


    // data logging
    bool changeReadPeriod(int period);
    void setLogPeriod(int period);

    // commands
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

// read period
bool ScaleController::changeReadPeriod(int period) {
  bool changed = period != state.read_period;

  if (changed) {
    state.read_period = period;
    Serial.println("INFO: setting read period to " + String(period) + " seconds");
    saveDS();
  } else {
    Serial.println("INFO: read period unchanged (" + String(period) + ")");
  }
  return(changed);
}

void ScaleController::setLogPeriod(int period) {
  Serial.println("INFO: setting log period to " + String(period) + " seconds");
  state.log_period = period;
  saveDS();
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
      command.success(changeLocked(true), true);
    } else if (command.parseValue(CMD_LOCK_OFF)) {
      command.success(changeLocked(false), true);
    }
  } else if (state.locked) {
    // device is locked --> no other commands allowed
    command.errorLocked();
  } else if (command.parseVariable(CMD_STATE_LOG)) {
    // state logging
    command.assignValue();
    if (command.parseValue(CMD_STATE_LOG_ON)) {
      command.success(changeStateLogging(true), true);
    } else if (command.parseValue(CMD_STATE_LOG_OFF)) {
      command.success(changeStateLogging(false), true);
    }
  } else if (command.parseVariable(CMD_TIMEZONE)) {
    // timezone
    command.assignValue();
    int tz = atoi(command.value);
    (tz >= -12 && tz <= 14) ? command.success(changeTimezone(tz)) : command.errorValue();
  } else if (command.parseVariable(CMD_READ_PERIOD)) {
    // read period
    command.assignValue();
    int period = atoi(command.value);
    (period >= 0) ? command.success(changeReadPeriod(period)) : command.errorValue();
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
