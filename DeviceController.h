#pragma once
#include "DeviceState.h"

// controller class
class DeviceController {

  private:

    // reset PIN
    const int reset_pin;
    bool reset = false;

  protected:

    // command
    DeviceCommand command;

    // call back
    void (*command_callback)();

  public:

    // constructor
    DeviceController();
    DeviceController (int reset_pin) : reset_pin(reset_pin) {}

    // setup and loop methods
    void init(); // to be run during setup()
    void update(); // to be run during loop()

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // state control & persistence functions (implement in derived classes)
    virtual DeviceState* getDS() = 0; // fetch the device state pointer
    virtual void saveDS() = 0; // save device state to EEPROM
    virtual bool restoreDS() = 0; // load device state from EEPROM

    // state change functions (will work in derived classes as long as getDS() is re-implemented)
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeTimezone(int tz);

    // particle command parsing functions
    virtual void registerCommand() = 0; // register the particle function (implement in derived classes)
    DeviceCommand* getCommand(); // get pointer to the command object
    void setCommandCallback(void (*cb)()); // assign a callback function
    void callCommandCallback(); // call the callback
    bool parseLocked();
    bool parseStateLogging();
    bool parseTimezone();
};

/* SETUP & LOOP */

void DeviceController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // register particle function
  registerCommand();

  //  check for reset
  if(digitalRead(reset_pin) == HIGH) {
    reset = true;
    Serial.println("INFO: reset request detected");
  }

  // initialize / restore state
  if (!reset){
    Serial.println("INFO: trying to restore state (version " + String(STATE_VERSION) + ") from memory");
    restoreDS();
  } else {
    Serial.println("INFO: resetting state back to default values");
  }

  // startup time info
  Time.zone(getDS()->timezone);
  time_t time = Time.now();
  Serial.println(Time.format(time, "INFO: startup time: %Y-%m-%d %H:%M:%S"));
}

void DeviceController::update() {
  // nothing happens in default loop
}

/* DEVICE STATE CHANGE FUNCTIONS */

// locking
bool DeviceController::changeLocked(bool on) {
  bool changed = on != getDS()->locked;

  if (changed) {
    getDS()->locked = on;
    on ? Serial.println("INFO: locking device") : Serial.println("INFO: unlocking device");
    saveDS();
  } else {
    on ? Serial.println("INFO: device already locked") : Serial.println("INFO: device already unlocked");
  }
  return(changed);
}

// state log
bool DeviceController::changeStateLogging (bool on) {
  bool changed = on != getDS()->state_logging;

  if (changed) {
    getDS()->state_logging = on;
    on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
    saveDS();
  } else {
    on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
  }
  return(changed);
}

// time zone
bool DeviceController::changeTimezone (int tz) {
  bool changed = tz != getDS()->timezone;
  if (changed) {
    getDS()->timezone = tz;
    Serial.print("INFO: timezone changed to " + String(getDS()->timezone));
    Time.zone(getDS()->timezone);
    time_t time = Time.now();
    Serial.println(Time.format(time, ", time: %Y-%m-%d %H:%M:%S"));
    saveDS();
  } else {
    Serial.println("INFO: timezone unchanged (" + String(getDS()->timezone) + ")");
  }
  return(changed);
}

/* COMMAND PARSING FUNCTIONS */

DeviceCommand* DeviceController::getCommand() {
  return(&command);
}

void DeviceController::setCommandCallback(void (*cb)()) {
  command_callback = cb;
}

void DeviceController::callCommandCallback() {
  if (command_callback) command_callback();
}

bool DeviceController::parseLocked() {
  // decision tree
  if (command.parseVariable(CMD_LOCK)) {
    // locking
    command.assignValue();
    if (command.parseValue(CMD_LOCK_ON)) {
      command.success(changeLocked(true), true);
    } else if (command.parseValue(CMD_LOCK_OFF)) {
      command.success(changeLocked(false), true);
    }
  } else if (getDS()->locked) {
    // device is locked --> no other commands allowed
    command.errorLocked();
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseStateLogging() {
  if (command.parseVariable(CMD_STATE_LOG)) {
    // state logging
    command.assignValue();
    if (command.parseValue(CMD_STATE_LOG_ON)) {
      command.success(changeStateLogging(true), true);
    } else if (command.parseValue(CMD_STATE_LOG_OFF)) {
      command.success(changeStateLogging(false), true);
    }
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseTimezone() {
  if (command.parseVariable(CMD_TIMEZONE)) {
    // timezone
    command.assignValue();
    int tz = atoi(command.value);
    (tz >= -12 && tz <= 14) ? command.success(changeTimezone(tz)) : command.errorValue();
  }
  return(command.isTypeDefined());
}
