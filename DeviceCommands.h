#pragma once

// return codes
#define CMD_RET_UNDEFINED     -100 // undefined behavior
#define CMD_RET_SUCCESS          0 // succes = 0
#define CMD_RET_WARN             1 // warnings > 0
#define CMD_RET_ERR             -1 // errors < 0
#define CMD_RET_ERR_TEXT        "undefined error"
#define CMD_RET_ERR_CMD         -2 // error unknown command
#define CMD_RET_ERR_CMD_TEXT    "invalid command"
#define CMD_RET_ERR_LOCKED      -3 // error locked
#define CMD_RET_ERR_LOCKED_TEXT "locked"
#define CMD_RET_ERR_TZ          -4 // invalid timezone
#define CMD_RET_ERR_TZ_TEXT     "invalid timezone"

// command log types
#define CMD_LOG_TYPE_UNDEFINED        "undefined"
#define CMD_LOG_TYPE_ERROR            "error"
#define CMD_LOG_TYPE_STATE_CHANGED    "state changed"
#define CMD_LOG_TYPE_STATE_UNCHANGED  "state unchanged"

// commands
#define CMD_ROOT            "device" // command root

// control
#define CMD_LOCK            "lock" // device "lock on/off" : locks/unlocks the device
  #define CMD_LOCK_ON         "on"
  #define CMD_LOCK_OFF        "off"

// logging
#define CMD_STATE_LOG       "state-log" // device "state-log on/off" : locks/unlocks the device
  #define CMD_STATE_LOG_ON     "on"
  #define CMD_STATE_LOG_OFF    "off"


// timezone
#define CMD_TIMEZONE        "tz"

// command from spark cloud
#define CMD_MAX_CHAR 63 //(spark.functions are limited to 63 char long call)
class DeviceCommand {

  public:

    // command message
    char command[CMD_MAX_CHAR];
    char buffer[CMD_MAX_CHAR];
    char variable[25];
    char value[20];
    char units[20];
    char msg[CMD_MAX_CHAR];

    // command outcome
    char type[20];
    int ret_val;

    // constructors
    DeviceCommand() {};
    DeviceCommand(String& command_string) {
      command_string.toCharArray(command, sizeof(command));
      strcpy(buffer, command);
      strcpy(value, "");
      strcpy(units, "");
      strcpy(msg, "");

      strcpy(type, CMD_LOG_TYPE_UNDEFINED);
      ret_val = CMD_RET_UNDEFINED;
    };

    // command parsing
    void extractParam(char* param, int size);
    void assignVariable();
    void assignValue();
    void assignUnits();
    void assignMessage();
    bool parseVariable(char* cmd);
    bool parseValue(char* cmd);

    // command status
    void success(bool state_changed);
    void success(bool state_changed, bool capture_message);
    void warning(); // not implemented yet
    void error(int code, char* text);
    void error();
    void errorLocked();
    void finalize();
};

/****** COMMAND STATUS *******/

void DeviceCommand::success(bool state_changed) { success(state_changed, false); }
void DeviceCommand::success(bool state_changed, bool capture_message) {
  ret_val = CMD_RET_SUCCESS;
  if (state_changed) {
    strcpy(type, CMD_LOG_TYPE_STATE_CHANGED);
  } else {
    strcpy(type, CMD_LOG_TYPE_STATE_UNCHANGED);
  }
  if (capture_message) {
    assignMessage();
  }
}

void DeviceCommand::error(int code, char* text) {
  ret_val = code;
  strncpy(variable, text, sizeof(variable));
  strcpy(type, CMD_LOG_TYPE_ERROR);
  strcpy(msg, command); // store entire command in message
}

void DeviceCommand::error() {
  error(CMD_RET_ERR, CMD_RET_ERR_TEXT);
}

void DeviceCommand::errorLocked() {
  error(CMD_RET_ERR_LOCKED, CMD_RET_ERR_LOCKED_TEXT);
}

// finalize the command
void DeviceCommand::finalize() {
  if (ret_val == CMD_RET_UNDEFINED) {
    error(CMD_RET_ERR_CMD, CMD_RET_ERR_CMD_TEXT);
  }
}

/****** COMMAND PARSING *******/

// capture command excerpt (until next space) in param
// using char array pointers instead of String to make sure we don't get memory leaks here
// providing size to be sure to be on the safe side
void DeviceCommand::extractParam(char* param, int size) {
  int space = strcspn(buffer, " ");
  // size safety check
  if (space < size) {
    strncpy (param, buffer, space);
    param[space] = 0;
  } else {
    strncpy (param, buffer, size);
    param[size] = 0;
  }
  // clean up buffer
  if (space == strlen(buffer)) {
    buffer[0] = 0;
  } else {
    for(int i = space+1; i <= strlen(buffer); i+=1) {
      buffer[i-space-1] = buffer[i];
    }
  }
}

// assigns the next extractable parameter to variable
void DeviceCommand::assignVariable() {
  extractParam(variable, sizeof(variable));
}

// assigns the next extractable paramter to value
void DeviceCommand::assignValue() {
  extractParam(value, sizeof(value));
}

// assigns the next extractable parameter to units
void DeviceCommand::assignUnits() {
  extractParam(units, sizeof(units));
}

// takes the remainder of the command buffer and assigns it to the message
void DeviceCommand::assignMessage() {
  strncpy(msg, buffer, sizeof(msg));
}

// check if variable has the specific value
bool DeviceCommand::parseVariable(char* cmd) {
  if (strcmp(variable, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

// check if variable has the specific value
bool DeviceCommand::parseValue(char* cmd) {
  if (strcmp(value, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}
