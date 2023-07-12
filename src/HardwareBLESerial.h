/*
An Arduino library for Nordic Semiconductors' proprietary "UART/Serial Port Emulation over BLE" protocol, using ArduinoBLE.

* https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v14.0.0/ble_sdk_app_nus_eval.html
* https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service
* https://github.com/sandeepmistry/arduino-BLEPeripheral/tree/master/examples/serial

Tested using UART console feature in [Adafruit Bluefruit LE Connect](https://apps.apple.com/at/app/adafruit-bluefruit-le-connect/id830125974).

modified by Thijs van Liempd in July of 2023
I added STM32 compatibility (using the STM32duinoBLE library, which is function-compatible, so it's a very minor change)
I used the Arduino Print class as a base (removing the manual implementations of print() and adding a write(buf,len) function) and changed a few things
I added a host class to enable communication between microcontrollers (tested on STM32WB55's using the STM32duinoBLE library) (Note: the base-class implementation is not the cleanest)
TODO:
- selecting which peripheral to connect to when playing host (instead of just selecting the first one found)
- multiple peripherals (multiple serials) in host mode???? (not sure whether it's possible, but it sounds fun)
- cleaner code (the base class could be cleaner)
*/

#ifndef __BLE_SERIAL_H__
#define __BLE_SERIAL_H__

#include <Arduino.h>

#ifdef ARDUINO_ARCH_STM32 // NOTE: this may not be as all-inclusive as it should. Works fine for me right now though ;)
  // #include "app_conf_custom.h" // should be done in main.cpp instead (before importing this library)
  #include <STM32duinoBLE.h>
  //// NOTE: some initialization (for compability with ArduinoBLE classes) was also added to the .cpp file
#else
  #include <ArduinoBLE.h>
#endif

#define BLE_ATTRIBUTE_MAX_VALUE_LENGTH 20
#define BLE_SERIAL_RECEIVE_BUFFER_SIZE 256

const static char* uartService_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const static char* receiveCharacteristic_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const static char* transmitCharacteristic_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

template<size_t N> class ByteRingBuffer {
  private:
    uint8_t ringBuffer[N];
    size_t newestIndex = 0;
    size_t length = 0;
  public:
    void add(uint8_t value) {
      ringBuffer[newestIndex] = value;
      newestIndex = (newestIndex + 1) % N;
      length = min(length + 1, N);
    }
    int pop() { // pops the oldest value off the ring buffer
      if (length == 0) {
        return -1;
      }
      uint8_t result = ringBuffer[(N + newestIndex - length) % N];
      length -= 1;
      return result;
    }
    void clear() {
      newestIndex = 0;
      length = 0;
    }
    int get(size_t index) { // this.get(0) is the oldest value, this.get(this.getLength() - 1) is the newest value
      if (index < 0 || index >= length) {
        return -1;
      }
      return ringBuffer[(N + newestIndex - length + index) % N];
    }
    size_t getLength() { return length; }
};

class _HardwareBLESerialBase : public Print {
  public:
    void poll();
    void end();
    size_t available();
    int peek();
    int read();
    virtual bool _transmitReady() = 0;
    size_t write(uint8_t byte);
    using Print::write; // pull in write(str) from Print
    size_t write(uint8_t* buffer, size_t size);
    void flush();

    size_t availableLines();
    size_t peekLine(char *buffer, size_t bufferSize);
    size_t readLine(char *buffer, size_t bufferSize);

    operator bool();

    _HardwareBLESerialBase();
    _HardwareBLESerialBase(_HardwareBLESerialBase const &other) = delete;  // disable copy constructor
    void operator=(_HardwareBLESerialBase const &other) = delete;  // disable assign constructor

    virtual ~_HardwareBLESerialBase() {} // empty deconstructor (to fix compiler complaints about vtable)

  protected:
    ByteRingBuffer<BLE_SERIAL_RECEIVE_BUFFER_SIZE> receiveBuffer;
    size_t numAvailableLines;

    unsigned long long lastFlushTime;
    size_t transmitBufferLength;
    uint8_t transmitBuffer[BLE_ATTRIBUTE_MAX_VALUE_LENGTH];

    // virtual BLECharacteristic receiveCharacteristic;
    // virtual BLECharacteristic transmitCharacteristic;

    void onReceive(const uint8_t* data, size_t size);

    BLECharacteristic receiveCharacteristic = BLECharacteristic(receiveCharacteristic_UUID, BLEWriteWithoutResponse, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
    BLECharacteristic transmitCharacteristic = BLECharacteristic(transmitCharacteristic_UUID, BLENotify, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
};

/////////////////////////////////////////////////////////////////////////// slave (default) specific: ///////////////////////////////////////////

class HardwareBLESerial : public _HardwareBLESerialBase {
  public:
    // singleton instance getter
    static HardwareBLESerial& getInstance() {
      static HardwareBLESerial instance; // instantiated on first use, guaranteed to be destroyed
      return instance;
    }

    // similar to begin(), but also sets up ArduinoBLE for you, which you otherwise have to do manually
    // use this for simplicity, use begin() for flexibility (e.g., more than one service)
    bool beginAndSetupBLE(const char *name);
    void begin();
    bool _transmitReady();
  private:
    static void onBLEWritten(BLEDevice central, BLECharacteristic characteristic); // i couldn't figure out how to put this in the base class without compiler complaints

    BLEService uartService = BLEService(uartService_UUID);
    // (characteristics moved to base class)
};

/////////////////////////////////////////////////////////////////////////// host specific: ///////////////////////////////////////////

class HardwareBLESerialHost : public _HardwareBLESerialBase {
  public:
    // singleton instance getter
    static HardwareBLESerialHost& getInstance() {
      static HardwareBLESerialHost instance; // instantiated on first use, guaranteed to be destroyed
      return instance;
    }

    // the only begin function (for now?). It may be possible to do multiple things as a central device, but i am weary of getting into those weeds
    bool beginAndSetupBLE(const char *name);
    // begin() was removed (for now?)

    bool _initConnection(BLEDevice periph); // called when a device is discovered
    bool _transmitReady(); // returns connection status instead of subscription status

    // String peripheralAddress() { return(peripheral.address()); } // if the peripheral member below is private/protected, give the user access to the MAC address at least
  // protected: // nah, i think the user should have access (to retrieve stuff like MAC addresses and such)
    BLEDevice peripheral; // the counterpart device

   private:
    static void onBLEWritten(BLEDevice periph, BLECharacteristic characteristic); // same as original class (but couldn't be part of base class)
    static void onBLEDiscovered(BLEDevice periph); // when a device is found (through scanning) it will automatically connect
    static void onBLEDisconnected(BLEDevice periph); // when the peripheral is disconnected, start scanning again
    // static void onBLEConnected(BLEDevice periph); // not needed (for now?)
    
    // (characteristics moved to base class)
};

#endif
