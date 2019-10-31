# Gigabits

## Setup

```c++
// Set number of callbacks to store. Defaults to 10
#define GIGABITS_MAX_CALLBACKS 10

#include <Gigabits.h>

void setup() {
  // Start gigabits with the device key and a Client type object
  gigabits.begin("yourdevicekey", net);

  // Add a listener to call when commands are received
  gigabits.addCommandListener(1, [](int32_t *data, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
      int32_t command = data[i]
      // Do something with the command
    }
  }
}

void loop() {
  // Call this frequently
  gigabits.run()
}
```
