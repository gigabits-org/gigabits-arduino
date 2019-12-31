/*
 *  This sketch demonstrates using the Gigabits device library 
 *  with an ESP32 using an unencrypted connection
 */

#include <WiFi.h>
#include <Gigabits.h>

WiFiClient net;
Gigabits gigabits;

unsigned long lastMillis = 0;

void setup() {
  Serial.begin(115200);
  
  WiFi.begin("ssid", "pass");

  // Add a listener to call when commands are received over Sensor Index 3
  gigabits.addCommandListener(3, [](int32_t *data, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
      int32_t command = data[i];
      Serial.print("Received a: ");Serial.println(command);
      // Do something with the command
    }
  });
  Serial.println("Connecting..");
  
  // Start gigabits with the device key and a Client type object
  gigabits.begin("yourdevicekey", "yoursecret", net, "mqtt.gigabits.io", 1883);
}

void loop() {
  // Call this frequently
  gigabits.run();

  if (millis() - lastMillis > 10000) {
    lastMillis = millis();
    Serial.println("Sending data...");
    // Send data to Sensor Index 1
    gigabits.sendRecord(1, (uint32_t)10);
    // Send data to Sensor Index 2
    gigabits.sendRecord(2, (uint32_t)(millis() % 100));
  }
}