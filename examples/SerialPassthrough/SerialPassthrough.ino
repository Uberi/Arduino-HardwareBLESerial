#include <HardwareBLESerial.h>

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!bleSerial.beginAndSetupBLE("SerialPassthrough")) {
    while (true) {
      Serial.println("failed to initialize HardwareBLESerial!");
      delay(1000);
    }
  }

  // wait for a central device to connect
  while (!bleSerial);

  Serial.println("HardwareBLESerial central device connected!");
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
