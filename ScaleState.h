#include "DeviceState.h"

// scale state
struct ScaleState : DeviceState {
  int log_period; // period between logs
  int read_period; // period between reads

  ScaleState() {};
  ScaleState(bool locked, bool logging, int log_period, int read_period) :
    DeviceState(locked, logging), log_period(log_period), read_period(read_period) {};
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
