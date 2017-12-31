#pragma once

// return codes
#define CMD_RET_SUCCESS          0 // succes = 0
#define CMD_RET_WARN             1 // warnings > 0
#define CMD_RET_ERR             -1 // errors < 0
#define CMD_RET_ERR_CMD         -2 // error unknown command
#define CMD_RET_ERR_CMD_TEXT    "unknown command"
#define CMD_RET_ERR_LOCKED      -3 // error locked
#define CMD_RET_ERR_LOCKED_TEXT "locked"

// commands
#define CMD_ROOT        "device" // command root

// control
#define CMD_LOCK_OFF       0
#define CMD_LOCK_OFF_TEXT  "unlock" // device unlock: unlocks the state
#define CMD_LOCK_ON        1
#define CMD_LOCK_ON_TEXT   "lock" // device lock : locks the state until unlock is called

// message types
#define TYPE_ERROR      "error"
#define TYPE_EVENT      "event"

// command from spark cloud
class ParticleCommand {

  private:

    int timezone = 0;
    unsigned long sync_period = 24 * 60 * 60 * 1000; // one day by default
    unsigned long last_sync = millis();

  public:

    char buffer[63] = ""; //(spark.functions are limited to 63 char long call)
    char type[20];
    char variable[25] = "";
    char value[20] = "";
    char units[20] = "";
    char msg[63] = "";
    bool error = false;

    // constructors
    ParticleCommand() {};
    ParticleCommand(String& command_string) {
      command_string.toCharArray(buffer, sizeof(buffer));
    };

    // command parsing
    void extractParam(char* param, int size);
    void assignVariable();
    void assignValue();
    void assignUnits();
    void assignMessage();

};

// capture command excerpt (until next space) in param
// using char array pointers instead of String to make sure we don't get memory leaks here
// providing size to be sure to be on the safe side
void ParticleCommand::extractParam(char* param, int size) {
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
void ParticleCommand::assignVariable() {
  extractParam(variable, sizeof(variable));
}

// assigns the next extractable paramter to value
void ParticleCommand::assignValue() {
  extractParam(value, sizeof(value));
}

// assigns the next extractable parameter to units
void ParticleCommand::assignUnits() {
  extractParam(units, sizeof(units));
}

// takes the remainder of the command buffer and assigns it to the message
void ParticleCommand::assignMessage() {
  strncpy(msg, buffer, sizeof(msg));
}
