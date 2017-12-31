#pragma once


// scale state
#define STATE_VERSION    7 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location
class DeviceState {
  public:
    const int version = STATE_VERSION;
    bool locked = false; // whether state is locked
    bool state_logging = false; // whether state is logged (whenever there is a change)
    bool data_logging = false; // whether data is logged
    int timezone = 0; // what time zone the device is in

    DeviceState() {};
    DeviceState(int timezone, bool locked, bool state_logging, bool data_logging) :
      timezone(timezone), locked(locked), state_logging(state_logging), data_logging(data_logging) {
        Time.zone(timezone);
      };

    // safe state to EEPROM --> implement in derived class (call parent for info message)
    virtual void save(){};

    // load state from EEPROM --> implement in derived class
    virtual bool load(){
      changeTimezone(timezone);
    };

    // locking
    bool changeLocked(bool on) {
      bool changed = on != locked;
      locked = on;

      if (changed) {
        save();
        on ? Serial.println("INFO: locking device") : Serial.println("INFO: unlocking device");
      } else {
        on ? Serial.println("INFO: device already locked") : Serial.println("INFO: device already unlocked");
      }
      return(changed);
    }

    // state log
    bool changeStateLogging (bool on) {
      bool changed = on != state_logging;
      state_logging = on;

      if (changed) {
        save();
        on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
      } else {
        on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
      }
      return(changed);
    }

    // time zone
    bool changeTimezone (int tz) {
      bool changed = tz != timezone;
      timezone = tz;

      if (changed) {
        Time.zone(timezone);
        save();
        Serial.print("INFO: timezone changed to " + String(timezone));
      } else {
        Serial.print("INFO: timezone unchanged (" + String(timezone) + ")");
      }
      time_t time = Time.now();
      Serial.println(Time.format(time, ", time: %Y-%m-%d %H:%M:%S"));
      return(changed);
    }

};
