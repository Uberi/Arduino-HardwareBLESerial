#include <HardwareBLESerial.h>

// #define PLAY_HOST // if defined, this STM will act as BLE host (central) and look for a counterpart (peripheral) to connect to

#ifdef PLAY_HOST
  HardwareBLESerialHost &bleSerial = HardwareBLESerialHost::getInstance();
#else
  HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();
#endif

bool wasConnected = false; // for debugging

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
  if(bleSerial) {
    if(!wasConnected) {
      wasConnected=true; // only do this once
      Serial.print("connected! to: ");
      #ifdef PLAY_HOST
        Serial.println(bleSerial.peripheral.address());
      #else
        Serial.println(BLE.central().address());
      #endif
    }
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
  } else if(wasConnected) { wasConnected=false; Serial.println("disconnected!"); } // indicate disconnection
}
