#pragma once

// controller class
class DeviceController {

  private:

    const int reset_pin;

  public:

    bool reset = false; // whether controller was started in reset mode

    // constructor
    DeviceController (int reset_pin) :
        reset_pin(reset_pin) {}

    // setup and loop methods
    void init(); // to be run during setup()
    void update(); // to be run during loop()

    virtual void restoreState(){} // restore previous state
    virtual int parseCommand (String command){} // parse a cloud command
};


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
    restoreState();
  } else {
    Serial.println("INFO: resetting state back to default values");
  }
}

void DeviceController::update() {

}
