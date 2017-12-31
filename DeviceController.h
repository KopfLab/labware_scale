#pragma once

// controller class
class DeviceController {

  private:

    // reset PIN
    const int reset_pin;
    bool reset = false;

    // state (re-implement in derived classes if state structure changes)
    DeviceState state;
    DeviceState *ds = &state;

  public:

    // constructor
    DeviceController (int reset_pin) :
        reset_pin(reset_pin) {}

    // setup and loop methods
    void init(); // to be run during setup()
    void update(); // to be run during loop()

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // state control functions (re-implement in derived classes if state structure changes)
    virtual DeviceState* getDS(); // fetch the device state pointer (overwrite in derive class)
    virtual void saveDS(); // save device state to EEPROM (implement in derived class)
    virtual bool restoreDS(); // load device state from EEPROM

    // state change functions (will work in derived classes as long as getDS() is re-implemented)
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeTimezone(int tz);

    virtual int parseCommand (String command){} // parse a cloud command
};

/* SETUP & LOOP */

void DeviceController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  //  check for eset
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

/* DEVICE STATE FUNCTIONS */

DeviceState* DeviceController::getDS() {
  return(ds);
}

void DeviceController::saveDS() {
  EEPROM.put(STATE_ADDRESS, state);
  Serial.println("INFO: device state saved in memory (if any updates were necessary)");
}

bool DeviceController::restoreDS(){
  DeviceState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  bool recoverable = saved_state.version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, state);
    Serial.println("INFO: successfully restored device state from memory (version " + String(STATE_VERSION) + ")");
  } else {
    Serial.println("INFO: could not restore device state from memory (found version " + String(saved_state.version) + "), sticking with initial default");
    saveDS();
  }
  return(recoverable);
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
