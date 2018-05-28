#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// enable/disable sub-systems
// #define ENABLE_DISPLAY

// time sync
#include "TimeSync.h";
TimeSync* ts = new TimeSync();

// scale controller
#include "ScaleController.h"

// lcd
DeviceDisplay* lcd = &LCD_20x4;

// initial state of the scale
ScaleState* state = new ScaleState(
  /* locked */                false,
  /* state_logging */         true,
  /* data_logging */          false,
  /* data_logging_period */   300 // in seconds
);

// scale controller
ScaleController* scale = new ScaleController(
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* baud rate */         4800,
  /* serial config */     SERIAL_8N1,
  /* request wait */      5000, // FIXME: maybe move to state
  /* error wait */        500,
  /* report digits */     3, // FIXME: maybe move to state
  /* pointer to state */  state
);

// user interface
char lcd_buffer[21];

void update_gui_state() {
  // lcd
  if (lcd) {
    // state updates
    if (state->data_logging)
      getStateDataLoggingPeriodText(state->data_logging_period, lcd_buffer, sizeof(lcd_buffer), true);
    else
      strcpy(lcd_buffer, "off");
    lcd->print_line(4, "Log: " + String(lcd_buffer));
  }
}

void update_gui_data() {
  // lcd
  if (lcd) {
    // running data
    if (scale->serialIsManual()) {
      // manual mode (show latest)
      if (scale->data[0].n > 0)
        Time.format(Time.now() - scale->data[0].data_time, "%Y-%m-%d %H:%M:%S %Z").toCharArray(lcd_buffer, sizeof(lcd_buffer));
      else
        strcpy(lcd_buffer, "Time: no data yet");
    } else {
      // automatic mode (show average)
      if (scale->data[0].n > 0)
        getDataDoubleText("Avg", scale->data[0].value, scale->data[0].units, scale->data[0].n, lcd_buffer, sizeof(lcd_buffer), PATTERN_KVUN_SIMPLE, scale->data[0].digits);
      else
        strcpy(lcd_buffer, "Avg: no data yet");
    }
    lcd->print_line(2, String(lcd_buffer));

    // latest data
    if (scale->data[0].newest_value_valid)
      getDataDoubleText("Last", scale->data[0].newest_value, scale->data[0].units, lcd_buffer, sizeof(lcd_buffer), PATTERN_KVU_SIMPLE, 1);
    else
      strcpy(lcd_buffer, "Last: no data yet");
    lcd->print_line(3, String(lcd_buffer));
  }
}

// callback function for commands
void report_command () {
  Serial.println("INFO: command callback: " +
    String(scale->command.type) + " " +
    String(scale->command.variable) + " " +
    String(scale->command.value));
  update_gui_state();
  update_gui_data();
}

// callback function for data
void report_data() {
  update_gui_data();
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
  scale->setDataCallback(report_data);
  scale->init();

  // connect device to cloud
  Serial.println("INFO: connecting to cloud");
  Particle.connect();

  // initial user interface update
  update_gui_state();
  update_gui_data();

}

// loop
void loop() {
  ts->update();
  scale->update();
}
