#include "HardwareBLESerial.h"

HardwareBLESerial::HardwareBLESerial() {
  this->numAvailableLines = 0;
  this->transmitBufferLength = 0;
  this->lastFlushTime = 0;
}

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

void HardwareBLESerial::poll() {
  if (millis() - this->lastFlushTime > 100) {
    flush();
  } else {
    BLE.poll();
  }
}

void HardwareBLESerial::end() {
  this->receiveCharacteristic.setEventHandler(BLEWritten, NULL);
  this->receiveBuffer.clear();
  flush();
}

size_t HardwareBLESerial::available() {
  return this->receiveBuffer.getLength();
}

int HardwareBLESerial::peek() {
  if (this->receiveBuffer.getLength() == 0) return -1;
  return this->receiveBuffer.get(0);
}

int HardwareBLESerial::read() {
  int result = this->receiveBuffer.pop();
  if (result == (int)'\n') {
    this->numAvailableLines --;
  }
  return result;
}

size_t HardwareBLESerial::write(uint8_t byte) {
  if (this->transmitCharacteristic.subscribed() == false) {
    return 0;
  }
  this->transmitBuffer[this->transmitBufferLength] = byte;
  this->transmitBufferLength ++;
  if (this->transmitBufferLength == sizeof(this->transmitBuffer)) {
      flush();
  }
  return 1;
}

void HardwareBLESerial::flush() {
  if (this->transmitBufferLength > 0) {
    this->transmitCharacteristic.setValue(this->transmitBuffer, this->transmitBufferLength);
    this->transmitBufferLength = 0;
  }
  this->lastFlushTime = millis();
  BLE.poll();
}

size_t HardwareBLESerial::availableLines() {
  return this->numAvailableLines;
}

size_t HardwareBLESerial::peekLine(char *buffer, size_t bufferSize) {
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

size_t HardwareBLESerial::readLine(char *buffer, size_t bufferSize) {
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

size_t HardwareBLESerial::print(const char *str) {
  if (this->transmitCharacteristic.subscribed() == false) {
    return 0;
  }
  size_t written = 0;
  for (size_t i = 0; str[i] != '\0'; i ++) {
    written += this->write(str[i]);
  }
  return written;
}
size_t HardwareBLESerial::println(const char *str) { return this->print(str) + this->write('\n'); }

size_t HardwareBLESerial::print(char value) {
  char buf[2] = { value, '\0' };
  return this->print(buf);
}
size_t HardwareBLESerial::println(char value) { return this->print(value) + this->write('\n'); }

size_t HardwareBLESerial::print(int64_t value) {
  char buf[21]; snprintf(buf, 21, "%lld", value); // the longest representation of a uint64_t is for -2^63, 20 characters plus null terminator
  return this->print(buf);
}
size_t HardwareBLESerial::println(int64_t value) { return this->print(value) + this->write('\n'); }

size_t HardwareBLESerial::print(uint64_t value) {
  char buf[21]; snprintf(buf, 21, "%llu", value);  // the longest representation of a uint64_t is for 2^64-1, 20 characters plus null terminator
  return this->print(buf);
}
size_t HardwareBLESerial::println(uint64_t value) { return this->print(value) + this->write('\n'); }

size_t HardwareBLESerial::print(double value) {
  char buf[319]; snprintf(buf, 319, "%f", value); // the longest representation of a double is for -1e308, 318 characters plus null terminator
  return this->print(buf);
}
size_t HardwareBLESerial::println(double value) { return this->print(value) + this->write('\n'); }

HardwareBLESerial::operator bool() {
  return BLE.connected();
}

void HardwareBLESerial::onReceive(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < min(size, sizeof(this->receiveBuffer)); i++) {
    this->receiveBuffer.add(data[i]);
    if (data[i] == '\n') {
      this->numAvailableLines ++;
    }
  }
}

void HardwareBLESerial::onBLEWritten(BLEDevice central, BLECharacteristic characteristic) {
  HardwareBLESerial::getInstance().onReceive(characteristic.value(), characteristic.valueLength());
}
