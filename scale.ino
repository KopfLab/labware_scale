#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// enable/disable sub-systems
// #define ENABLE_DISPLAY

// display
#ifdef ENABLE_DISPLAY
#include "Display.h"
Display lcd (
  /* i2c address */  0x27,
  /* lcd width */      20,
  /* lcd height */      4,
  /* message width */   7,
  /* show time */    3000);
#endif

// scale controller
#include "ScaleController.h"

// initial state of the scale
ScaleState* state = new ScaleState(
  /* timezone */           -6, // Mountain Daylight Time -6 / Mountain Standard Time -7
  /* locked */          false,
  /* logging */         false,
  /* log_period */         70, // in seconds
  /* read_period */         5 // in seconds
);

// scale controller
ScaleController* scale = new ScaleController(
  /* reset pin */         A5,
  /* pointer to state */  state
);

// state information & function to update the user interface(s) based on changes in the state
char state_information[600] = "";

char logging_lcd[7];
char logging_web[20];
char locked_lcd[6];
char locked_web[20];

// user interface update
void update_user_interface () {

  // user interface update text
  //get_scale_state_logging_info(state.logging, logging_lcd, sizeof(logging_lcd), logging_web, sizeof(logging_web));
  //get_scale_state_locked_info(state.locked, locked_lcd, sizeof(locked_lcd), locked_web, sizeof(locked_web));

  // serial (for debugging)
  Serial.println("@UI - Locked: " + String(state->locked));
  Serial.println("@UI - Logging: " + String(state->state_logging));
  Serial.println("@UI - Period: " + String(state->log_period));
  Serial.println("@UI - Read: " + String(state->read_period));
  Serial.println("@UI - Timezone: " + String(state->timezone));

/*
  Serial.println("@UI - Locked: " + String(state.locked));
  Serial.println("@UI - Logging: " + String(state.state_logging));
  Serial.println("@UI - Period: " + String(state.log_period));
  Serial.println("@UI - Read: " + String(state.read_period));
  Serial.println("@UI - Timezone: " + String(state.timezone));
*/
  // lcd
  /*
  #ifdef ENABLE_DISPLAY
    lcd.print_line(1, "Status: " + String(status_lcd) + " (MS" + String(ms_mode_lcd) + ")");
    lcd.print_line(2, "Speed: " + String(rpm_lcd) + " rpm");
    lcd.print_line(3, "Dir: " + String(dir_lcd) + " " + String(locked_lcd));
  #endif
  */

  // web
  //snprintf(state_information, sizeof(state_information),
  //  "{\"logging\":\"%s\", \"lock\":\"%s\"}", logging_web, locked_web);
}

// callback function for commands
void report_command () {
  Serial.println("HERE!!!");
  Serial.println("INFO: scale command - " +
    String(scale->getCommand()->type) + " " +
    String(scale->getCommand()->variable) + " " +
    String(scale->getCommand()->value));
  update_user_interface();
}

// device name
char device_name[20];
void name_handler(const char *topic, const char *data) {
  strncpy ( device_name, data, sizeof(device_name) );
  Serial.println("INFO: device ID " + String(device_name));
  #ifdef ENABLE_DISPLAY
    lcd.print_line(4, "ID: " + String(device_name));
  #endif
}

// using system threading to speed up restart after power out
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);
bool name_handler_registered = false;

// setup
void setup() {

  // serial
  Serial.begin(9600);

  delay(2000);

  // time // FIXME: allow for external setting of the time zone
  //Time.zone(-6); // Mountain Daylight Time -6 / Mountain Standard Time -7

  // scale
  Serial.println("INFO: initialize scale");
  scale->setCommandCallback(report_command);
  scale->init();

  // check for reset
  if (scale->wasReset()) {
    #ifdef ENABLE_DISPLAY
      lcd.print_line(1, "Resetting...");
      delay(1000);
    #endif
  }

  // user interface update
  Serial.println("INFO: updating user interface");
  //update_user_interface(scale.state);
  update_user_interface();

  // connect device to cloud and register for listeners
  Serial.println("INFO: registering spark variables and connecting to cloud");
  Particle.variable("state", state_information);
  Particle.subscribe("spark/", name_handler);
  Particle.connect();

}

// loop
void loop() {
  if (!name_handler_registered && Particle.connected()){
    // running this here becaus we're in system thread mode and it won't work until connected
    name_handler_registered = Particle.publish("spark/device/name");
    Serial.println("INFO: name handler registered");
  }

  #ifdef ENABLE_DISPLAY
    lcd.update();
  #endif
  scale->update();
}
