# Gigabits

## Setup

### Install Arduino IDE

[Arduino install guide](https://www.arduino.cc/en/Guide/HomePage)

### Install Gigabits Library

Search for gigabits in the library manager or pull this repo into your libary folder.

[How to install a library](https://www.arduino.cc/en/guide/libraries)

### Integrate into device application

Gigabits connects devices to the cloud using sensor indices. Create your project on [app.gigabits.io](https://app.gigabits.io)
and make sure to match up the indices.

## Example

```c++
// Set number of callbacks to store. Defaults to 10
#define GIGABITS_MAX_CALLBACKS 10

#include <WiFi.h>
#include <Gigabits.h>

WiFiClient net;

void setup() {
  WiFi.begin("ssid", "pass");

  // Add a listener to call when commands are received over Sensor Index 1
  gigabits.addCommandListener(1, [](int32_t *data, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
      int32_t command = data[i]
      // Do something with the command
    }
  }

  // Start gigabits with the device key and a Client type object
  gigabits.begin("yourdevicekey", net);
}

void loop() {
  // Call this frequently
  gigabits.run()

  // Send data to Sensor Index 2
  gigabits.sendRecord(2, 10);
}
```
