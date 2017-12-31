#pragma once

class TimeSync {

  private:

    int timezone = 0;
    unsigned long sync_period = 24 * 60 * 60 * 1000; // one day by default
    unsigned long last_sync = millis();

  public:

    // constructor
    TimeSync () {};
    TimeSync (int tz) : timezone(tz) {};
    void init(); // to be run during setup()
    void update(); // to be run during loop()
    void setTimezone(int tz); // set the time zone

};


void TimeSync::init() {
  last_sync = millis();
  setTimezone(timezone);
}

void TimeSync::update() {
  // time sync
  if ( (millis() - last_sync > sync_period || millis() < last_sync ) && Particle.connected()) {
      // Request time synchronization from the Particle Cloud
      Particle.syncTime();
      last_sync = millis();
      Serial.println("INFO: time sync complete");
  }
}

void TimeSync::setTimezone(int tz) {
  timezone = tz;
  Time.zone(timezone);
}
