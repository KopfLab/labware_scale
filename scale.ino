#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// enable/disable sub-systems
// #define ENABLE_DISPLAY

// time sync
#include "TimeSync.h";
TimeSync* ts = new TimeSync();

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
  /* timezone */                 -6, // Mountain Daylight Time -6 / Mountain Standard Time -7
  /* locked */                false,
  /* state_logging */         true,
  /* data_logging */          false,
  /* data_logging_period */      70 // in seconds
);

// scale controller
ScaleController* scale = new ScaleController(
  /* reset pin */         A5,
  /* pointer to state */  state
);

// state information & function to update the user interface(s) based on changes in the state
//char state_information[600] = "";

char logging_lcd[7];
char logging_web[20];
char locked_lcd[6];
char locked_web[20];

// user interface update
void update_user_interface () {
  Serial.println("INFO: GUI update callback");
  // lcd
  /*
  #ifdef ENABLE_DISPLAY
    lcd.print_line(1, "Status: " + String(status_lcd) + " (MS" + String(ms_mode_lcd) + ")");
    lcd.print_line(2, "Speed: " + String(rpm_lcd) + " rpm");
    lcd.print_line(3, "Dir: " + String(dir_lcd) + " " + String(locked_lcd));
  #endif
  */
}

// callback function for commands
void report_command () {
  Serial.println("INFO: command callback: " +
    String(scale->getCommand()->type) + " " +
    String(scale->getCommand()->variable) + " " +
    String(scale->getCommand()->value));
  update_user_interface();
}

// callback function for device name
void report_name() {
  #ifdef ENABLE_DISPLAY
    lcd.print_line(4, "Name: " + String(scale->name));
  #endif
}

// using system threading to speed up restart after power out
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// setup
void setup() {

  // serial
  Serial.begin(9600);

  delay(3000);

  // time sync
  ts->init();

  // scale
  Serial.println("INFO: initialize scale");
  scale->setCommandCallback(report_command);
  scale->setNameCallback(report_name);
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
  update_user_interface();

  // connect device to cloud and register for listeners
  Serial.println("INFO: connecting to cloud");
  Particle.connect();

}

// loop
void loop() {
  #ifdef ENABLE_DISPLAY
    lcd.update();
  #endif
  ts->update();
  scale->update();
}
