HardwareBLESerial
=================

An Arduino library for Nordic Semiconductors' proprietary [UART/Serial Port Emulation over BLE](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v14.0.0/ble_sdk_app_nus_eval.html) protocol, using [ArduinoBLE](https://www.arduino.cc/en/Reference/ArduinoBLE).

Features:

* **Byte-oriented I/O**: `HardwareBLESerial::available`, `HardwareBLESerial::peek`, `HardwareBLESerial::read`, `HardwareBLESerial::write`.
* **Line-oriented I/O**: `HardwareBLESerial::availableLines`, `HardwareBLESerial::peekLine`, `HardwareBLESerial::readLine`.
* **Flexible printing**: `HardwareBLESerial::print`, `HardwareBLESerial::println` (supports `char *`, `char`, `int64_t`, `uint64_t`, `double`).

Since the BLE standard doesn't include a standard for UART, there are multiple proprietary standards available. Nordic's protocol seems to be the most popular and seems to have the best software support. This library exposes a `Serial`-like interface, but without any of the blocking calls (e.g., `Serial.readBytesUntil`), and supports additional line-oriented features such as `peekLine` and `readLine`.

Generally speaking, BLE is not designed for UART-style communication, and this will consume more power than a design centered around individual BLE characteristics. However, you may find BLE UART useful for:

* **Wireless logging**: Need to test your robot in the field, but cables getting in the way? View logs and diagnostics in realtime via apps such as Bluefruit LE! Just replace your existing `Serial.println` with `bleSerial.println` and you're 90% done.
* **Command-line interfaces**: HardwareBLESerial works great with [CommandParser](https://github.com/Uberi/Arduino-CommandParser), a commandline parser by yours truly. Check out **Using CommandParser to make a BLE commandline-like interface** in the examples section!
* **Transferring binary/text data**: HardwareBLESerial papers over packet size limits and makes it easy to send large amounts of binary/text data back and forth between devices.

This library requires ArduinoBLE, and should work on all boards that ArduinoBLE works on, such as the **Arduino Nano 33 BLE**, **Arduino Nano 33 IoT**, or **Arduino MKR WiFi 1010**. Most of my testing was done on an Arduino Nano 33 BLE.

Quickstart
----------

**NOTE:** HardwareBLESerial is not yet in the Arduino Library Manager - I'm currently working on getting it included. For now, install via ZIP file!

Search for "HardwareBLESerial" in the Arduino Library Manager, and install it. Now you can try a quick example sketch:

```cpp
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
```

Download Bluefruit LE Connect on your mobile phone, then connect to your Arduino. 

This sketch demonstrates some differences between HardwareBLESerial and `Serial`:

1. You must obtain a HardwareBLESerial instance by calling `HardwareBLESerial::getInstance`.
2. You must initialize your HardwareBLESerial instance by calling `HardwareBLESerial::beginAndSetupBLE`, which is analogous to `Serial.begin`.
3. You must call `HardwareBLESerial::poll` regularly in order to synchronize state with BLE hardware, unlike `Serial` which does not need polling by the user.
4. You can use line-oriented methods to read/peek/check input for entire lines at a time. `Serial` has something similar with `Serial.readBytesUntil`, but it is harder to use and also blocks control flow.

More examples:

* [Echo each received line back to the sender](examples/Echo/Echo.ino)
* [Bridge between BLE UART and hardware serial port](examples/SerialPassthrough/SerialPassthrough.ino)
* [Using CommandParser to make a BLE commandline-like interface](examples/CommandLine/CommandLine.ino)

Apps for working with BLE UART
------------------------------

Usually you connect to BLE peripherals using a mobile phone or computer. In the BLE connection, the phone/computer is then the "BLE central device".

For BLE UART, you'll usually need a special app that supports interfacing over UART. For iOS devices, here's my review of various available options:

* [Bluefruit LE Connect](https://apps.apple.com/us/app/bluefruit-connect/id830125974): the best UART console I've tried. Scans quickly, connects quickly, and console is decently easy to use. The only complaint would be that each line that you send is truncated to 20 bytes (see note below). However, it doesn't support setting up dedicated buttons that send common commands.
* [nRF Toolbox](https://apps.apple.com/us/app/nrf-toolbox/id820906058): another app with a UART console, but a bit harder to connect to a peripheral than Bluefruit LE Connect. Does support dedicated buttons for quick access to common commands, which is nice.
* [LightBlue Explorer](https://apps.apple.com/us/app/lightblue-explorer/id557428110): like BLE Hero, it also only supports managing BLE CHaracteristics, but it is the easiest to use option 
* [nRF Connect](https://apps.apple.com/us/app/nrf-connect/id1054362403?ls=1): a good choice for managing BLE characteristics, the UI is a little bit unintuitive but it is quite a complete solution. However, it doesn't support UART that well, though in theory you can directly edit the RX/TX characteristics to interface with UART.

I also tried these but didn't find them as useful:

* [Blue - Bluetooth & developers](https://apps.apple.com/us/app/blue-bluetooth-developers/id1458096008): this does work for managing BLE characteristics, but doesn't have a UART console and overall has less features than nRF Connect, though it is easier to use. However, LightBlue Explorer has a better UI.
* [BLE Hero](https://apps.apple.com/ca/app/ble-hero/id1013013325): like Blue, it also only supports managing BLE characteristics, but very buggy UI and quite sluggish - refresh often doesn't work. Would not recommend.
* [BLE Terminal HM-10](https://apps.apple.com/us/app/ble-terminal-hm-10/id1398703795): this actually only supports the Texas Instruments UART protocol, not the Nordic Semiconductor one. Does not work with this library!

**NOTE:** One problem with all of the above apps that offer a UART console is that they limit the length of the message you can write to 20 bytes, often silently truncating your message at 20 bytes. The standard way to send messages longer than 20 bytes is to send them in chunks of 20 bytes with a short delay in between, but these apps currently do not implement this. As a workaround, I manually send long messages in chunks of 20 bytes.

Reference
---------

### `HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance()`

The `HardwareBLESerial` class is a singleton, so there can be at most 1 instance of this class (because a device can only have 1 UART service).

The `getInstance` method returns a reference to this singular instance of `HardwareBLESerial`.

### `bool HardwareBLESerial::beginAndSetupBLE(const char *name)`

This is exactly the same as `HardwareBLESerial::begin`, except it also initializes ArduinoBLE, sets the name of the device to `name`, and makes the Arduino visible to BLE central devices.

Returns `true` if initialization completed successfully, `false` otherwise.

Typically you would use this rather than `HardwareBLESerial::begin`, as it takes care of a small amount of ArduinoBLE setup for you - you won't need to think about ArduinoBLE at all! You would instead use `HardwareBLESerial::begin` if you need more flexibility, such as changing the ArduinoBLE initialization code.

### `void HardwareBLESerial::begin()`

Registers the BLE UART service with ArduinoBLE, and sets it as the one service whose UUID is sent out with BLE advertising packets (this ensures that BLE central devices can see right off the bat that we support BLE UART, without needing to connect to us).

This requires ArduinoBLE to be initialized beforehand. Afterwards, you must also tell ArduinoBLE to start actually broadcasting, using `BLE.advertise()`. For a helper function that does everything ArduinoBLE-related for you, see `HardwareBLESerial::beginAndSetupBLE`.

### `void HardwareBLESerial::poll()`

Performs internal BLE-related housekeeping tasks. Must be called regularly to ensure BLE state is kept up to date.

If not called often enough, we may miss some data packets being sent to us from the BLE central device - `HardwareBLESerial::poll` must be called more often than data packets are sent to us.

Exactly how often data packets are sent depends on the BLE central device. Usually for UART terminals such as Bluefruit LE Connect, each time you press "Send", it sends one data packet, and since you likely won't be sending messages more than twice a second, calling `HardwareBLESerial::poll` twice a second should be enough to ensure we don't miss anything.

### `void HardwareBLESerial::end()`

Cleans up some of the resources used by `HardwareBLESerial`. ArduinoBLE doesn't supply enough functionality to fully clean up all of the resources we used, so this method is generally not that useful.

### `size_t HardwareBLESerial::available()`

Returns the number of bytes available for reading (bytes that were already received and are waiting in the receive buffer).

### `int HardwareBLESerial::peek()`

Returns the next byte available for reading as an `int`. If no bytes are available for reading, then returns -1 instead.

### `int HardwareBLESerial::read()`

Returns the next byte available for reading as an `int`, and also consumes the byte from the receive buffer. If no bytes are available for reading, then returns -1 instead.

### `int HardwareBLESerial::write(uint8_t byte)`

Writes `byte` to the transmit buffer, where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device - that's 0 if there is no BLE central device connected, or 1 if there is.

### `void HardwareBLESerial::flush()`

Manually flush the transmit buffer, allowing all data within it to be sent out to the connected BLE central device, if there is one.

This usually doesn't need to be called, since the transmit buffer should be automatically flushed by `HardwareBLESerial::poll` if it hasn't been flushed within the last 100 milliseconds, or automatically flushed when it's full.

### `size_t HardwareBLESerial::availableLines()`

Returns the number of lines available for reading (lines that were already received and are waiting in the receive buffer).

A line is defined as a sequence of zero or more non-newline characters, followed by a newline character.

### `size_t HardwareBLESerial::peekLine(char *buffer, size_t bufferSize)`

Copies the next line available for reading into `buffer` (or the first `bufferSize - 1` characters of it, if the line doesn't fully fit). If no next line is available for reading, it simply null-terminates `buffer` so it contains an empty string. Afterwards, `buffer` is guaranteed to be null-terminated.

Returns the number of characters copied from the line, so between 0 and `bufferSize - 1`. If there was no next line, then consider that as 0 characters being copied.

### `size_t HardwareBLESerial::readLine(char *buffer, size_t bufferSize)`

Copies the next line available for reading into `buffer` (or the first `bufferSize - 1` characters of it, if the line doesn't fully fit), and also consumes that line from the receive buffer. If no next line is available for reading, it simply null-terminates `buffer` so it contains an empty string. Afterwards, `buffer` is guaranteed to be null-terminated.

Returns the number of characters copied from the line, so between 0 and `bufferSize - 1`. If there was no next line, then consider that as 0 characters being copied.

### `size_t HardwareBLESerial::print(const char *str)`, `size_t HardwareBLESerial::println(const char *str)`

Writes a null-terminated string `str` to the transmit buffer, where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device (0 if there is no BLE central device connected).

### `size_t HardwareBLESerial::print(char value)`, `size_t HardwareBLESerial::println(char value)`

Writes a single char `value` to the transmit buffer, where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device (0 if there is no BLE central device connected, 1 if there is a BLE central device connected).

### `size_t HardwareBLESerial::print(int64_t value)`, `size_t HardwareBLESerial::println(int64_t value)`

Writes a single signed 64-bit integer `value` to the transmit buffer, formatted as a decimal number, where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device (0 if there is no BLE central device connected).

### `size_t HardwareBLESerial::print(uint64_t value)`, `size_t HardwareBLESerial::println(uint64_t value)`

Writes a single unsigned 64-bit integer `value` to the transmit buffer, formatted as a non-negative decimal number, where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device (0 if there is no BLE central device connected).

### `size_t HardwareBLESerial::print(double value)`, `size_t HardwareBLESerial::println(double value)`

Writes a single double `value` to the transmit buffer, formatted as a decimal format floating point number (i.e., no exponents), where it'll queue up until the buffer is flushed (usually within 100 milliseconds).

Returns the number of bytes that were sent to the BLE central device (0 if there is no BLE central device connected).

### `if (HardwareBLESerial) { ... }`

Instances of `HardwareBLESerial` can be used as boolean values - they are truthy when a BLE central device is connected, and falsy when no BLE central devices are connected. This works even without calling `HardwareBLESerial::poll`, so you can call this in a loop by itself without `HardwareBLESerial::poll` like in the code example below.

This is often used to wait for a device to connect:

```cpp
HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();
while (!bleSerial) {
  delay(100); // on Mbed-based Arduino boards, delay() makes the board enter a low-power sleep mode
}
```

Resources
---------

This library would not be possible without these excellent resources:

* [Official Nordic Semiconductor page for their UART protocol](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v14.0.0/ble_sdk_app_nus_eval.html). This had a lot of information about rationale and design considerations.
* [Kevin Townsend's BLE UART tutorial](https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service). This was a much more digestible version of the Nordic page, making implementation a lot easier.
* [BLE UART example from the Arduino-BLEPeripheral library](https://github.com/sandeepmistry/arduino-BLEPeripheral/tree/master/examples/serial). This was useful in debugging the resulting library, especially the idea to use a regularly-flushed transmission ring buffer.