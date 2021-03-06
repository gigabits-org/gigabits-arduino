/*
 *  This sketch demonstrates using the Gigabits device library
 *  with a MKR1000 and HCPA-5V-U3 I2C Sensor
 */
#define GIGABITS_MAX_CALLBACKS 10
#include <SPI.h>
#include <WiFi101.h>
#include <Gigabits.h>
#include <DHT.h>

char ssid[] = "ssid";
char pass[] = "pass";
char devKey[] = "devkey";
char secret[] = "secret";

#include <Wire.h>
// HCPA-5V-U3 I2C address is 0x28(40)
#define Addr 0x28

WiFiClient net;
//WiFiSSLClient net;
Gigabits gigabits;

unsigned long lastMillis = 0;

void setup() {

  Serial.begin(115200);
  Serial.println("Connecting..");
  WiFi.begin(ssid, pass);
  Serial.println("Connected!");

  delay(300);

  gigabits.addCommandListener(3, [](int32_t *data, size_t sz){
    for (size_t i = 0; i < sz; i++) {
      Serial.print("Received a ");Serial.println(data[i]);
    }
  });
  // gigabits.begin("devKey", "secret", net);
  // Custom endpoint
  // gigabits.begin("your_device_key", "your_device_secret", net, "api.dev.gigabits.io");
  // Custom port
  gigabits.begin(devKey, secret, net, "mqtt.gigabits.io", 1883);
  Wire.begin();
  // Start I2C transmission
  Wire.beginTransmission(Addr);
  // Send start command
  Wire.write(0x80);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(300);

}

void loop() {
  gigabits.run();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    Serial.println("Sending inside loop ...");
    Serial.flush();
    unsigned int data[4];

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Stop I2C transmission
    byte error = Wire.endTransmission();

    if (error == 0) {
      // Request 4 bytes of data
      Wire.requestFrom(Addr, 4);

      // Read 4 bytes of data
      // humidity msb, humidity lsb, cTemp msb, cTemp lsb
      if (Wire.available() == 4)
      {
        data[0] = Wire.read();
        data[1] = Wire.read();
        data[2] = Wire.read();
        data[3] = Wire.read();
      }

      // Convert the data to 14-bits
      float humidity = (((data[0] & 0x3F) * 256) + data[1]) / 16384.0 * 100.0;
      float cTemp = (((data[2] * 256) + (data[3] & 0xFC)) / 4) / 16384.0 * 165.0 - 40.0;
      float fTemp = (cTemp * 1.8) + 32;

      Serial.print("humidity: "); Serial.println(humidity);
      Serial.print("fTemp: "); Serial.println(fTemp);
      gigabits.sendRecord(1, humidity);
      gigabits.sendRecord(2, fTemp);
    } else {
      Serial.print("Error: "); Serial.println(error);
      gigabits.sendRecord(1, "Error: disconnected");
      gigabits.sendRecord(2, "Error: disconnected");
    }
  }
}
