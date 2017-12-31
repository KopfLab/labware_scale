#pragma once
#include "DeviceState.h"

// controller class
#define STATE_INFO_MAX_CHAR   600 // how long is the state information maximally
class DeviceController {

  private:

    // reset PIN
    const int reset_pin;
    bool reset = false;

    // device info
    bool name_handler_registered = false;
    char state_information_buffer[STATE_INFO_MAX_CHAR-2];

    // call backs
    void (*name_callback)();
    void (*command_callback)();

  protected:

    // command
    DeviceCommand command;

  public:

    // constructor
    DeviceController();
    DeviceController (int reset_pin) : reset_pin(reset_pin) {}

    // setup and loop methods
    void init(); // to be run during setup()
    void update(); // to be run during loop()

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // device name
    char name[20];
    void captureName(const char *topic, const char *data);
    void setNameCallback(void (*cb)()); // assign a callback function

    // state information
    char state_information[STATE_INFO_MAX_CHAR];
    void updateStateInformation();
    virtual void assembleStateInformation();
    void addToStateInformation(char* info);
    void addToStateInformation(char* info, char* sep);

    // state control & persistence functions (implement in derived classes)
    virtual DeviceState* getDS() = 0; // fetch the device state pointer
    virtual void saveDS() = 0; // save device state to EEPROM
    virtual bool restoreDS() = 0; // load device state from EEPROM

    // state change functions (will work in derived classes as long as getDS() is re-implemented)
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);
    bool changeTimezone(int tz);

    // particle command parsing functions
    DeviceCommand* getCommand(); // get pointer to the command object
    void setCommandCallback(void (*cb)()); // assign a callback function
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand () = 0; // parse a cloud command
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseTimezone();
};

/* SETUP & LOOP */

void DeviceController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // register particle functions
  Serial.println("INFO: registering device cloud variables");
  Particle.subscribe("spark/", &DeviceController::captureName, this);
  Particle.function(CMD_ROOT, &DeviceController::receiveCommand, this);
  Particle.variable(STATE_VARIABLE, state_information);

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

  // state information
  updateStateInformation();
}

void DeviceController::update() {

  // name capture
  if (!name_handler_registered && Particle.connected()){
    name_handler_registered = Particle.publish("spark/device/name");
    Serial.println("INFO: name handler registered");
  }

}

/* DEVICE NAME */

void DeviceController::captureName(const char *topic, const char *data) {
  strncpy ( name, data, sizeof(name) );
  Serial.println("INFO: device name " + String(name));
  if (name_callback) name_callback();
}

void DeviceController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

/* STATE INFORMATION */

void DeviceController::updateStateInformation() {

  Serial.print("INFO: updating state information: ");
  state_information_buffer[0] = 0; // reset buffer
  assembleStateInformation();
  snprintf(state_information, sizeof(state_information), "{%s}", state_information_buffer);
  Serial.println(state_information);
}

void DeviceController::addToStateInformation(char* info) {
  addToStateInformation(info, ",");
}

void DeviceController::addToStateInformation(char* info, char* sep) {
  if (state_information_buffer[0] == 0) {
    strncpy(state_information_buffer, info, sizeof(state_information_buffer));
  } else {
    snprintf(state_information_buffer, sizeof(state_information_buffer),
        "%s%s%s", state_information_buffer, sep, info);
  }
}

void DeviceController::assembleStateInformation() {
  char pair[60];
  getStateTimezoneText(getDS()->timezone, pair, sizeof(pair), false); addToStateInformation(pair);
  getStateLockedText(getDS()->locked, pair, sizeof(pair), false); addToStateInformation(pair);
  getStateStateLoggingText(getDS()->state_logging, pair, sizeof(pair), false); addToStateInformation(pair);
  getStateDataLoggingText(getDS()->data_logging, pair, sizeof(pair), false); addToStateInformation(pair);
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

// data log
bool DeviceController::changeDataLogging (bool on) {
  bool changed = on != getDS()->data_logging;

  if (changed) {
    getDS()->data_logging = on;
    on ? Serial.println("INFO: data logging turned on") : Serial.println("INFO: data logging turned off");
    saveDS();
  } else {
    on ? Serial.println("INFO: data logging already on") : Serial.println("INFO: data logging already off");
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

bool DeviceController::parseLocked() {
  // decision tree
  if (command.parseVariable(CMD_LOCK)) {
    // locking
    command.assignValue();
    if (command.parseValue(CMD_LOCK_ON)) {
      command.success(changeLocked(true));
    } else if (command.parseValue(CMD_LOCK_OFF)) {
      command.success(changeLocked(false));
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
      command.success(changeStateLogging(true));
    } else if (command.parseValue(CMD_STATE_LOG_OFF)) {
      command.success(changeStateLogging(false));
    }
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseDataLogging() {
  if (command.parseVariable(CMD_DATA_LOG)) {
    // state logging
    command.assignValue();
    if (command.parseValue(CMD_DATA_LOG_ON)) {
      command.success(changeDataLogging(true));
    } else if (command.parseValue(CMD_DATA_LOG_OFF)) {
      command.success(changeDataLogging(false));
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

/****** WEB COMMAND PROCESSING *******/

int DeviceController::receiveCommand(String command_string) {

  // load, parse and finalize command
  command.load(command_string);
  command.assignVariable();
  parseCommand();
  command.finalize();

  // state information
  if (command.ret_val >= CMD_RET_SUCCESS && command.ret_val != CMD_RET_WARN_NO_CHANGE) {
    updateStateInformation();
  }

  // command reporting callback
  if (command_callback) command_callback();

  // return value
  return(command.ret_val);
}
