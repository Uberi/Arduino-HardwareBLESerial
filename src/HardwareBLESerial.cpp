#include "HardwareBLESerial.h"

#ifdef ARDUINO_ARCH_STM32 // NOTE: this may not be as all-inclusive as it should. Works fine for me right now though ;)
  #if defined(ARDUINO_NUCLEO_WB15CC) || defined(ARDUINO_P_NUCLEO_WB55RG) || defined(ARDUINO_STM32WB5MM_DK) // just an extra check (for my foolish future-self)
    HCISharedMemTransportClass HCISharedMemTransport;
    #if !defined(FAKE_BLELOCALDEVICE)
      BLELocalDevice BLEObj(&HCISharedMemTransport);
      BLELocalDevice& BLE = BLEObj; // this is a slight hack, to make the library happy. It's presumably a remnant of the ArduinoBLE library origins
    #endif
  #else
    #error("other STM32 BLE devices (or any devices included the STM32duinoBLE library) are not yet setup in the HardwareBLESerial library.\
            Please consult the STM32duinoBLE library for the proper setup config")
  #endif
#else
#endif

_HardwareBLESerialBase::_HardwareBLESerialBase() {
  this->numAvailableLines = 0;
  this->transmitBufferLength = 0;
  this->lastFlushTime = 0;
}

void _HardwareBLESerialBase::poll() {
  if (millis() - this->lastFlushTime > 100) {
    flush();
  } else {
    BLE.poll();
  }
}

void _HardwareBLESerialBase::end() {
  this->receiveCharacteristic.setEventHandler(BLEWritten, NULL);
  this->receiveBuffer.clear();
  flush();
}

size_t _HardwareBLESerialBase::available() {
  return this->receiveBuffer.getLength();
}

int _HardwareBLESerialBase::peek() {
  if (this->receiveBuffer.getLength() == 0) return -1;
  return this->receiveBuffer.get(0);
}

int _HardwareBLESerialBase::read() {
  int result = this->receiveBuffer.pop();
  if (result == (int)'\n') {
    this->numAvailableLines --;
  }
  return result;
}

size_t _HardwareBLESerialBase::write(uint8_t byte) {
  if (this->_transmitReady() == false) {
    return 0;
  }
  this->transmitBuffer[this->transmitBufferLength] = byte;
  this->transmitBufferLength ++;
  if (this->transmitBufferLength == sizeof(this->transmitBuffer)) {
    flush();
  }
  return 1;
}

size_t _HardwareBLESerialBase::write(uint8_t* buffer, size_t size) { // Thijs addition
  size_t written = 0;
  while(written < size) {
    if(this->_transmitReady() == false) { return(written); } // check connection
    while((this->transmitBufferLength < sizeof(this->transmitBuffer)) && (written < size)) {
      this->transmitBuffer[this->transmitBufferLength] = buffer[written]; // copy byte
      this->transmitBufferLength++; written++; // increment counters
    }
    if (this->transmitBufferLength == sizeof(this->transmitBuffer)) { flush(); } // the extra if-statement here just avoids an extra flush of a partially filled buffer
  }
  return(written);
}

void _HardwareBLESerialBase::flush() {
  if (this->transmitBufferLength > 0) {
    this->transmitCharacteristic.setValue(this->transmitBuffer, this->transmitBufferLength);
    this->transmitBufferLength = 0;
  }
  this->lastFlushTime = millis();
  BLE.poll();
}

size_t _HardwareBLESerialBase::availableLines() {
  return this->numAvailableLines;
}

size_t _HardwareBLESerialBase::peekLine(char *buffer, size_t bufferSize) {
  if (this->availableLines() == 0) {
    buffer[0] = '\0';
    return 0;
  }
  size_t i = 0;
  for (; i < bufferSize - 1; i ++) {
    int chr = this->receiveBuffer.get(i);
    if (chr == -1 || chr == '\n') {
      break;
    } else {
      buffer[i] = chr;
    }
  }
  buffer[i] = '\0';
  return i;
}

size_t _HardwareBLESerialBase::readLine(char *buffer, size_t bufferSize) {
  if (this->availableLines() == 0) {
    buffer[0] = '\0';
    return 0;
  }
  size_t i = 0;
  for (; i < bufferSize - 1; i ++) {
    int chr = this->read();
    if (chr == -1 || chr == '\n') {
      break;
    } else {
      buffer[i] = chr;
    }
  }
  buffer[i] = '\0';
  return i;
}

_HardwareBLESerialBase::operator bool() {
  return BLE.connected();
}

void _HardwareBLESerialBase::onReceive(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < min(size, sizeof(this->receiveBuffer)); i++) {
    this->receiveBuffer.add(data[i]);
    if (data[i] == '\n') {
      this->numAvailableLines ++;
    }
  }
}

/////////////////////////////////////////////////////////////////////////// slave (default) specific: ///////////////////////////////////////////

bool HardwareBLESerial::beginAndSetupBLE(const char *name) {
  if (!BLE.begin()) { return false; }
  BLE.setLocalName(name);
  BLE.setDeviceName(name);
  this->begin();
  BLE.advertise();
  return true;
}

void HardwareBLESerial::begin() {
  BLE.setAdvertisedService(uartService);
  uartService.addCharacteristic(receiveCharacteristic);
  uartService.addCharacteristic(transmitCharacteristic);
  receiveCharacteristic.setEventHandler(BLEWritten, HardwareBLESerial::onBLEWritten);
  BLE.addService(uartService);
}

bool HardwareBLESerial::_transmitReady() { return(this->transmitCharacteristic.subscribed()); }

void HardwareBLESerial::onBLEWritten(BLEDevice central, BLECharacteristic characteristic) {
  HardwareBLESerial::getInstance().onReceive(characteristic.value(), characteristic.valueLength());
}

/////////////////////////////////////////////////////////////////////////// host specific: ///////////////////////////////////////////

bool HardwareBLESerialHost::beginAndSetupBLE(const char *name) {
  if (!BLE.begin()) { return false; }
  BLE.setLocalName(name);
  BLE.setDeviceName(name);
  BLE.setEventHandler(BLEDiscovered, HardwareBLESerialHost::onBLEDiscovered);
  BLE.setEventHandler(BLEDisconnected, HardwareBLESerialHost::onBLEDisconnected);
  // BLE.setEventHandler(BLEConnected, HardwareBLESerialHost::onBLEConnected);
  BLE.scanForUuid(uartService_UUID); // start looking for BLE devices that advertise the UART service
  return true;
}

// void printPeripheralStats(BLEDevice peripheral) { // debug
//   if(!peripheral.connected()) { Serial.println("perihperal not connected"); return; }
//   // if(!peripheral.discovered()) { Serial.println("perihperal not discovered"); return; }
//   //// print all the BLE stuff of the connected device:
//   Serial.print("serviceCount: "); Serial.println(peripheral.serviceCount());
//   for(uint8_t i=0; i<peripheral.serviceCount(); i++) {
//     BLEService service = peripheral.service(i);
//     Serial.print("service UUID: "); Serial.println(service.uuid());
//     Serial.print("  characteristicCount: "); Serial.println(service.characteristicCount());
//     for(uint8_t j=0; j<service.characteristicCount(); j++) {
//       BLECharacteristic characteristic = service.characteristic(j);
//       Serial.print("    characteristic UUID: "); Serial.println(characteristic.uuid());
//       Serial.print("      valueLength: "); Serial.println(characteristic.valueLength());
//       Serial.print("      valueSize: "); Serial.println(characteristic.valueSize());
//       Serial.print("      properties: 0b"); Serial.println(characteristic.properties(), BIN);
//       Serial.print("      canRead: "); Serial.println(characteristic.canRead());
//       Serial.print("      canWrite: "); Serial.println(characteristic.canWrite());
//       Serial.print("      canSubscribe: "); Serial.println(characteristic.canSubscribe());
//       Serial.print("      canUnsubscribe: "); Serial.println(characteristic.canUnsubscribe());
//       Serial.print("      written: "); Serial.println(characteristic.written());
//       Serial.print("      descriptorCount: "); Serial.println(characteristic.descriptorCount());
//       Serial.print("      peripheral hasCharacteristic: "); Serial.println(peripheral.hasCharacteristic(characteristic.uuid()));
//       Serial.print("      service hasCharacteristic: "); Serial.println(service.hasCharacteristic(characteristic.uuid()));
//       for(uint8_t k=0; k<characteristic.descriptorCount(); k++) {
//         BLEDescriptor descriptor = characteristic.descriptor(k);
//         Serial.print("    descriptor UUID: "); Serial.println(descriptor.uuid());
//         Serial.print("      valueLength: "); Serial.println(descriptor.valueLength());
//         Serial.print("      valueSize: "); Serial.println(descriptor.valueSize());
//       }
//     }
//   }
//   //// NOTE: you can also just get abstract characteristics, instead of going through the classes, but i don't love that
//   // Serial.print("characteristicCount: "); Serial.println(peripheral.characteristicCount());
//   // for(uint8_t j=0; j<peripheral.characteristicCount(); j++) {
//   //   BLECharacteristic characteristic = peripheral.characteristic(j);
//   //   Serial.print("  characteristic UUID: "); Serial.println(characteristic.uuid());
//   // }
// }

bool HardwareBLESerialHost::_initConnection(BLEDevice periph) {
  this->peripheral = periph;
  if(!this->peripheral.connect()) { Serial.println("debug never connected"); BLE.scanForUuid(uartService_UUID); return(false); } // try connection (NOTE: subject to debugging)
  if(!this->peripheral.discoverAttributes()) { this->peripheral.disconnect(); return(false); } // try attribute discovery
  // printPeripheralStats(this->peripheral); // debug
  // //// verify whether it has the right services and characteristics (before retrieving them)
  // if(!this->peripheral.hasService(uartService_UUID)) { this->peripheral.disconnect(); return(false); } // check if it has the attribute (it must, because of the way the scanning works)
  // BLEService uartService = this->peripheral.service(uartService_UUID); // temporary object, just for checking whether it has the characteristics
  // if(!uartService.hasCharacteristic(receiveCharacteristic_UUID)) { this->peripheral.disconnect(); return(false); } // check if it has a receiveCharacteristic
  // if(!uartService.hasCharacteristic(transmitCharacteristic_UUID)) { this->peripheral.disconnect(); return(false); } // check if it has a transmitCharacteristic
  //// retrieve characteristics and validate them
  this->transmitCharacteristic = this->peripheral.characteristic(receiveCharacteristic_UUID); // the slave's receive is this object's transmit
  if(!this->transmitCharacteristic) { this->peripheral.disconnect(); return(false); } // check if characteristic was retrieved
  if(!this->transmitCharacteristic.canWrite()) { this->peripheral.disconnect(); return(false); } // check if the characteristic has the right properties
  this->receiveCharacteristic = this->peripheral.characteristic(transmitCharacteristic_UUID); // the slave's transmit is this object's receive
  if(!this->receiveCharacteristic) { this->peripheral.disconnect(); return(false); } // check if characteristic was retrieved
  if(!this->receiveCharacteristic.canSubscribe()) { this->peripheral.disconnect(); return(false); } // check if the characteristic has the right properties
  //// initialize the receive side
  if(!this->receiveCharacteristic.subscribe()) { this->peripheral.disconnect(); return(false); } // try subscribe (Note: does not appear to be stricly necessary)
  this->receiveCharacteristic.setEventHandler(BLEWritten, HardwareBLESerialHost::onBLEWritten); // set the ISR
  return(this->peripheral.connected()); // if it made it here, the the connection is good
}

bool HardwareBLESerialHost::_transmitReady() { return(BLE.connected()); } // usually true (no subscription status to check at least)

void HardwareBLESerialHost::onBLEWritten(BLEDevice periph, BLECharacteristic characteristic) {
  HardwareBLESerialHost::getInstance().onReceive(characteristic.value(), characteristic.valueLength());
}

void HardwareBLESerialHost::onBLEDiscovered(BLEDevice periph) {
  BLE.stopScan(); // stop looking once a peripheral is found
  HardwareBLESerialHost::getInstance()._initConnection(periph); // try to initialize connection with peripheral (and if it fails, it will call)
  //// NOTE: if _initConnection() fails, onBLEDisconnected() will be called and scanning will restart
}

void HardwareBLESerialHost::onBLEDisconnected(BLEDevice periph) {
  BLE.scanForUuid(uartService_UUID); // start looking for a peripheral again
  Serial.println("debug disconnected");
}

// void HardwareBLESerialHost::onBLEConnected(BLEDevice periph) { // not needed (for now?)
//   HardwareBLESerialHost::getInstance().
// }