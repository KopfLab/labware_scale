// scale state
#define STATE_VERSION    2 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location
struct DeviceState {
  const int version = STATE_VERSION;
  bool locked; // whether settings are locked
  bool logging; // whether web logging is on or off

  DeviceState() {};
  DeviceState(bool locked, bool logging) : logging(logging), locked(locked) {};

  void save(); // safe state to EEPROM
};


void DeviceState::save() {
  EEPROM.put(STATE_ADDRESS, this);
  Serial.println("INFO: state saved in memory (if any updates were necessary)");
}
