#include "DeviceState.h"

// scale state
class ScaleState : public DeviceState {
  public:
    bool logging =true;
    int log_period; // period between logs
    int read_period; // period between reads

    ScaleState() {};
    ScaleState(int timezone, bool locked, bool logging, int log_period, int read_period) :
      DeviceState(timezone, locked, logging, logging), log_period(log_period), read_period(read_period) {};

    // save to EEPROM
    void save() {
      EEPROM.put(STATE_ADDRESS, *this);
      Serial.println("INFO: scale state saved in memory (if any updates were necessary)");
      DeviceState::save();
    }

    // load from EEPROM
    bool load(){
      ScaleState saved_state;
      EEPROM.get(STATE_ADDRESS, saved_state);
      bool recoverable = saved_state.version == STATE_VERSION;
      if(recoverable) {
        EEPROM.get(STATE_ADDRESS, *this);
        Serial.println("INFO: successfully restored state from memory (version " + String(STATE_VERSION) + ")");
      } else {
        Serial.println("INFO: could not restore state from memory (found version " + String(saved_state.version) + "), sticking with initial default");
        save();
      }
      DeviceState::load();
      return(recoverable);
    }

};



// NOTE: size is passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)

static void get_scale_state_logging_info(bool logging, char* target_short, int size_short, char* target_long, int size_long) {
  if (logging) {
    strncpy(target_short, "logging", size_short - 1);
    strncpy(target_long, "logging started", size_long - 1);
  } else {
    strcpy(target_short, "no log");
    strncpy(target_long, "logging stopped", size_long - 1);
  }
}

static void get_scale_state_locked_info(bool locked, char* target_short, int size_short, char* target_long, int size_long) {
  if (locked) {
    strncpy(target_short, "LOCK", size_short - 1);
    strncpy(target_long, "locked", size_long - 1);
  } else {
    strcpy(target_short, "");
    strncpy(target_long, "ready", size_long - 1);
  }
}
