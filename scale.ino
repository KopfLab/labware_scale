#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// time sync
#include "TimeSync.h";
TimeSync* ts = new TimeSync();

// debugging options
#define CLOUD_DEBUG_ON
//#define WEBHOOKS_DEBUG_ON
//#define STATE_DEBUG_ON
#define DATA_DEBUG_ON
//#define SERIAL_DEBUG_ON
//#define LCD_DEBUG_ON

// keep track of installed version
#define STATE_VERSION    8 // update whenver structure changes
#define DEVICE_VERSION  "scale 0.7.1" // update with every code update

// scale controller
#include "ScaleController.h"

// lcd
DeviceDisplay* lcd = &LCD_20x4;

// initial state of the scale
ScaleState* state = new ScaleState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_reading_period_min */   2000, // in ms
  /* data_reading_period */       5000, // in ms
  /* data_logging_period */       300, // in seconds
  /* data_logging_type */         LOG_BY_TIME, // log by time
  /* calc_rate */                 CALC_RATE_MIN
);

// scale controller
ScaleController* scale = new ScaleController(
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* baud rate */         4800,
  /* serial config */     SERIAL_8N1,
  /* error wait */        500,
  /* pointer to state */  state
);

// using system threading to speed up restart after power out
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// setup
void setup() {

  // serial
  Serial.begin(9600);

  // time sync
  ts->init();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // scale
  scale->init();

  // connect device to cloud
  Serial.println("INFO: connecting to cloud");
  Particle.connect();
}

// loop
void loop() {
  ts->update();
  scale->update();
}
