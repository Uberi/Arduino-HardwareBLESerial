#include <HardwareBLESerial.h>

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

void setup() {
  if (!bleSerial.beginAndSetupBLE("Echo")) {
    Serial.begin(9600);
    while (true) {
      Serial.println("failed to initialize HardwareBLESerial!");
      delay(1000);
    }
  }
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
