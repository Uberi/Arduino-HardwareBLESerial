#include <HardwareBLESerial.h>
#include <CommandParser.h>

// ADVANCED EXAMPLE: demonstration of how HardwareBLESerial can work together with CommandParser to provide a commandline interface over BLE (NOTE: requires CommandParser to be installed separately)

/////////////
// Globals //
/////////////

// HardwareBLESerial instance
HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

// CommandParser instance
typedef CommandParser<> MyCommandParser;
MyCommandParser parser;

/////////////////////////////////////
// CommandParser command callbacks //
/////////////////////////////////////

int positionX = 0, positionY = 0;

void cmd_move(MyCommandParser::Argument *args, char *response) {
  positionX = args[0].asInt64;
  positionY = args[1].asInt64;
  Serial.print("MOVING "); Serial.print(args[0].asInt64); Serial.print(" "); Serial.println(args[1].asInt64);
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "moved to %d, %d", positionX, positionY);
}

void cmd_jump(MyCommandParser::Argument *args, char *response) {
  Serial.println("JUMPING!");
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "jumped at %d, %d", positionX, positionY);
}

void cmd_say(MyCommandParser::Argument *args, char *response) {
  Serial.print("SAYING "); Serial.println(args[0].asString);
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "said %s at %d, %d", args[0].asString, positionX, positionY);
}

//////////////////
// Main program //
//////////////////

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!bleSerial.beginAndSetupBLE("CommandLine")) {
    while (true) {
      Serial.println("failed to initialize HardwareBLESerial!");
      delay(1000);
    }
  }

  parser.registerCommand("move", "ii", &cmd_move); // two int64_t arguments
  Serial.println("Registered command: move X Y");
  parser.registerCommand("jump", "", &cmd_jump); // no arguments
  Serial.println("Registered command: jump");
  parser.registerCommand("say", "s", &cmd_say); // one string argument
  Serial.println("Registered command: say \"SOMETHING\"");
}

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  // read and process incoming commands
  while (bleSerial.availableLines() > 0) {
    // read line
    char line[128];
    bleSerial.readLine(line, 128);

    // process line with CommandParser
    char response[MyCommandParser::MAX_RESPONSE_SIZE]; parser.processCommand(line, response);
    bleSerial.println(response);
  }
  delay(500);
}
