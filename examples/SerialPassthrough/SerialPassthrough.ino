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

  // whatever is written to BLE UART appears in the Serial Monitor
  while (bleSerial.available() > 0) {
    Serial.write(bleSerial.read());
  }

  // whatever is written in Serial Monitor appears in BLE UART
  while (Serial.available() > 0) {
    bleSerial.write(Serial.read());
  }
}
