// return codes
#define CMD_RET_SUCCESS        0 // succes = 0
#define CMD_RET_ERR           -1 // errors < 0
#define CMD_RET_ERR_CMD       -2 // error unknown command
#define ERROR_CMD             "unknown command"
#define CMD_RET_ERR_LOCKED    -3 // error locked
#define ERROR_LOCKED          "locked"

// commands
#define CMD_ROOT        "scale" // command root

// control
#define CMD_LOCK        "lock" // scale lock : locks the state until unlock is called
#define CMD_UNLOCK      "unlock" // scale unlock: unlocks the state

// reading & logging
#define CMD_READ_FREQ   "read-period" // scale read-period number [mgs] : number of seconds between each data read
#define CMD_LOG_FREQ    "log-period" // scale log-period number [msg] : number of seconds between each data logging (if log is on)
#define CMD_LOG         "log" // scale log on/off [msg] : turn data logging on/off
  #define CMD_LOG_ON      "on"
  #define CMD_LOG_OFF     "off"

// message types
// FIXME: is this still correct?
#define TYPE_ERROR      "error"
#define TYPE_EVENT      "event"
#define TYPE_SET        CMD_SET

// command from spark cloud
// FIXME: is this still correct?
struct ScaleCommand {
  char buffer[63]; //(spark.functions are limited to 63 char long call)
  char type[20];
  char variable[25];
  char value[20];
  char units[20];
  char msg[63];
  ScaleCommand() {};
  ScaleCommand(String& command_string) {
    command_string.toCharArray(buffer, sizeof(buffer));
  };
};
