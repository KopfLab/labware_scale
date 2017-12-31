/*
 * ScaleController.h
 * Created on: December 29, 2017
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */

#pragma once
#include "ScaleState.h"
#include "ScaleCommands.h"

// controller class
class ScaleController {

  private:

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void saveState(); // returns TRUE if state was updated, FALSE if it didn't need updating
    bool loadState(); // returns TRUE if state loaded successfully, FALSE if it didn't match the required structure and default was loaded instead

  public:

    // state
    ScaleState state;

    // command
    ScaleCommand command;

    // callback
    typedef void (*ScaleCommandCallbackType) (const ScaleController&);

    // constructors
    ScaleController (ScaleState state, ScaleCommandCallbackType callback) :
        state(state), command_callback(callback) {
      Particle.function(CMD_ROOT, &ScaleController::parseCommand, this);
    }

    // setup and loop methods
    void init(); // to be run during setup()
    void init(bool reset); // whether to reset state back to the default
    void update(); // to be run during loop()

    // data logging
    void setReadPeriod(int period);
    void setLogPeriod(int period);
    void startLogging();
    void stopLogging();

    // lock/unlock
    void lock(); // lock pump
    void unlock(); // unlock pump

    // commands
    int parseCommand (String command); // parse a cloud command

  // scale command callback
  private: ScaleCommandCallbackType command_callback;

};


// IMPLEMENTATION

void ScaleController::init() { init(false); }
void ScaleController::init(bool reset) {
  // initialize / restore pump state
  if (!reset){
    loadState();
  } else {
    Serial.println("INFO: resetting state back to default values");
  }
}

// loop function
void ScaleController::update() {

  // FIXME: implement
  // do the data reading and do the data logging if logs are on

}

/**** STATE STORAGE AND RELOAD ****/
void ScaleController::saveState() {
  EEPROM.put(STATE_ADDRESS, state);
  Serial.println("INFO: state saved in memory (if any updates were necessary)");
}

bool ScaleController::loadState(){
  ScaleState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  if(saved_state.version == STATE_VERSION) {
    EEPROM.get(STATE_ADDRESS, state);
    Serial.println("INFO: successfully restored state from memory (version " + String(STATE_VERSION) + ")");
    return(true);
  } else {
    // EEPROM saved state did not have the corret version
    Serial.println("INFO: could not restore state from memory, sticking with initial default");
    return(false);
  }
}

/**** LOCK AND UNLOCK ****/

void ScaleController::lock() {
  state.locked = true;
  Serial.println("INFO: locking scale");
  saveState();
}

void ScaleController::unlock() {
  state.locked = false;
  Serial.println("INFO: unlocking scale");
  saveState();
}

/**** CHANGING STATE ****/

void ScaleController::setReadPeriod(int period) {
  Serial.println("INFO: setting read period to " + String(period) + " seconds");
  state.read_period = period;
  saveState();
}

void ScaleController::setLogPeriod(int period) {
  Serial.println("INFO: setting log period to " + String(period) + " seconds");
  state.log_period = period;
  saveState();
}

void ScaleController::startLogging() {
  Serial.println("INFO: starting data logging");
  state.logging = true;
  saveState();
}

void ScaleController::stopLogging() {
  Serial.println("INFO: stopping data logging");
  state.logging = false;
  saveState();
}

/****** WEB COMMAND PROCESSING *******/

// using char array pointers instead of String to make sure we don't get memory leaks here
void ScaleController::extractCommandParam(char* param) {
  int space = strcspn(command.buffer, " ");
  strncpy (param, command.buffer, space);
  param[space] = 0;
  if (space == strlen(command.buffer)) {
    command.buffer[0] = 0;
  } else {
    for(int i = space+1; i <= strlen(command.buffer); i+=1) {
      command.buffer[i-space-1] = command.buffer[i];
    }
  }
}

void ScaleController::assignCommandMessage() {
  // takes the remainder of the command buffer and assigns it to the message
  strncpy(command.msg, command.buffer, sizeof(command.msg));
}

int ScaleController::parseCommand(String command_string) {

  command = ScaleCommand(command_string);
  extractCommandParam(command.variable);
  strcpy(command.type, TYPE_EVENT); // default
  strcpy(command.value, ""); // reset
  strcpy(command.units, ""); // reset
  strcpy(command.msg, ""); // reset

  int ret_val = CMD_RET_SUCCESS;

  // locking
  if (strcmp(command.variable, CMD_UNLOCK) == 0) {
    assignCommandMessage();
    unlock();
  } else if (strcmp(command.variable, CMD_LOCK) == 0) {
    assignCommandMessage();
    lock();
  } else if (state.locked) {
    // pump is locked and command is not lock/unlock
    strcpy(command.variable, ERROR_LOCKED);
    ret_val = CMD_RET_ERR_LOCKED;
  } else {
    // regular commands
    // FIXME: implement
    // NOTE: may let logging start/stop despite state otherwise locked/unlocked?
    // or instead, make sure that when data logging starts/stops there always is a log start/stop happening
  }

  // highlight if it's an error
  if (ret_val < 0) {
    strcpy(command.type, TYPE_ERROR);
    command_string.toCharArray(command.msg, sizeof(command.msg));
  }

  // command reporting callback
  if (command_callback) {
    command_callback(*this);
  }

  return(ret_val);
}
