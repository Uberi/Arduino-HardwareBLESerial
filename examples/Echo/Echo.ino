#include <BLESerial.h>

BLESerial &bleSerial = BLESerial::getInstance();

void setup() {
  if (!bleSerial.beginAndSetupBLE("Echo")) {
    while (true); // failed to initialize BLESerial
  }

  // wait for a central device to connect
  while (!bleSerial);

  bleSerial.println("BLESerial device connected!");
}

void loop() {
  // this must be called regularly to perform BLE updates
  bleSerial.poll();

  while (bleSerial.availableLines() > 0) {
    bleSerial.print("You said: ");
    char line[128]; bleSerial.readLine(line, 128);
    bleSerial.println(line);
  }
  delay(500);
}
