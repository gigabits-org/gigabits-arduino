/*
 * This sketch demonstrates using the Gigabits device library
 * with an MKR1000 using an encrypted connection.  The sensors are arranged
 * on a breadboard, so the NCD sensors are not used.  The microcontroller is:
 * https://www.amazon.com/ARDUINO-MKR1000-WiFi-with-HEADERS/dp/B071LRYYHH
 *
 * It reads data from the following sensors:
 * HiLetgo LM393 3.3V-5V Soil Moisture Detect Sensor Soil Moisture
 * MQ-2 Gas LPG Butane Hydrogen lGas Sensor Detector Module
 * RobotDyn Photosensitive Light Sensor LDR with analog and digital outs
 * flashtree DHT22/AM2302 Module Digital Temperature and Humidity Sensor
 */
// Set max callbacks to store on the stack. Defaults to 10
#define GIGABITS_MAX_CALLBACKS 20
// Set size of JSON buffers. Defaults to 500
#define GIGABITS_JSON_SIZE 1000
#include <WiFi101.h>
#include "DHT.h"
#include <WiFiSSLClient.h>
#include <Gigabits.h>

// Change these!
char ssid[] = "ssid";
char pass[] = "TEMPERATURE_SENSOR_IDX";
char devKey[] = "devkey";
char secret[] = "secret";

// Define the MKR1000 pins used.  The constraints for pin use are described in
// https://components101.com/microcontrollers/arduino-mkr1000-wi-fi-board/
// The mapping between physical pin number and GPIO is shown in the same
// web page.
// We use the A0 through Aa6 pins for analog input and the D0 through D14
// bidirectional digital pins.  The digital pins are used for the DHT22.
// From https://create.arduino.cc/projecthub/charifmahmoudi/arduino-mkr1000-getting-started-08bb4a:
// Pin number	Pin Name
// 00					D0
// 01					D1
// ....
// 07         D7
// ...
// 15					A0
// 16					A1
// ....
// 21					A6

int lightSensorPin = 16;     // Pin A1
int gasSensorPin   = 17;     // Pin A2
#define DHTPIN       2
#define DHTTYPE		   DHT22
#define LEDPin       5       // Pin D5
int hygrometerPin  = 18;     // Pin A3

#include <LEDDisplay.h>       // LEDPin must be defined before this is included

// Screen size is 1 pixel
LEDDisplay display;

WiFiSSLClient net;

// Gigabits sensor indices
#define HUMIDITY_SENSOR_IDX 1
#define TEMPERATURE_SENSOR_IDX 2
#define OLED_INVERT_COMMAND_IDX 3
#define PRESSURE_SENSOR_IDX 4
#define GAS_SENSOR_IDX 5
#define SOIL_SENSOR_IDX 6
#define PROXY_SENSOR_IDX 7
#define VISIBLE_LIGHT_SENSOR_IDX 8
#define INFRARED_LIGHT_SENSOR_IDX 9

#define ADC_IDX 10

#define PCA_CH1_IDX 21
#define PCA_CH2_IDX 22
#define PCA_CH3_IDX 23
#define PCA_CH4_IDX 24

Gigabits gigabits;

unsigned long lastMillis = 0;

void setup() {
  Serial.begin(115200);

  setupDHT();
  setupGas();
  setupHygrometer();
  setupLight();
  setupDisplay();

  Serial.println("Connecting..");
  WiFi.begin(ssid, pass);
  Serial.println("Connected!");

  delay(300);

  setupGigabits();
}

void loop() {
  // This should be called on very often
  gigabits.run();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    sendDHTData();
    sendGasData();
    sendHygrometerData();
    sendLightData();
    Serial.println();
  }
}

DHT dht(DHTPIN, DHTTYPE);

void setupDHT() {
  // Set up the code that's used to read the DHT humidity and temperature.
  // See the Arduino"DHT sensor library"

  // Initialize the DHT library.
  dht.begin();
}

void sendDHTData() {
  Serial.println("Sending DHT Data...");

  float humidity = dht.readHumidity();
	float cTemp = dht.readTemperature();
	float fTemp = dht.readTemperature(true);

	// did it work?
	if (isnan(humidity) || isnan(cTemp) || isnan(fTemp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print("Humidity: ");Serial.println(humidity);
  Serial.print("fTemp: ");Serial.println(fTemp);

  gigabits.sendRecord(HUMIDITY_SENSOR_IDX, humidity);
  gigabits.sendRecord(TEMPERATURE_SENSOR_IDX, fTemp);
  return;
}

void setupGas() {
  pinMode(gasSensorPin, INPUT);
}

void sendGasData() {
  Serial.println("Sending Gas");
  float raw_adc = (float)analogRead(gasSensorPin);
  Serial.print("Gas: ");Serial.println(raw_adc);
  gigabits.sendRecord(GAS_SENSOR_IDX, raw_adc);
}

void setupHygrometer() {
  pinMode(hygrometerPin, INPUT);
}

void sendHygrometerData() {
  Serial.println("Sending Hygrometer Data");
  float raw_adc = (float)analogRead(hygrometerPin);
  Serial.print("SoilSensor: "); Serial.println(raw_adc);
  gigabits.sendRecord(SOIL_SENSOR_IDX, raw_adc);
}

void setupLight() {
  pinMode(lightSensorPin, INPUT);
}

void sendLightData() {
  Serial.println("Sending Light Data");
  int intLightData = analogRead(lightSensorPin);
  // The documentation says no light => small signal,
  // but it works the other way around.
  float raw_adc = (float)(2000 - intLightData);
  Serial.print("Light Sensor: "); Serial.println(raw_adc);
  gigabits.sendRecord(VISIBLE_LIGHT_SENSOR_IDX, raw_adc);
}

// Set up our single-LED display
void setupDisplay() {
  display.turnOnDisplay();
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, LOW);
}

void setupGigabits() {
  // Add command listener to invert our "display" (one LED) using a
  // lambda function
  gigabits.addCommandListener(OLED_INVERT_COMMAND_IDX, [](int32_t *data, size_t sz){
    // I'm trying to copy the original code.   I think it does something like data[0] == 0 => set display to 0.  data[0] == 1 => toggle display.
    if (sz > 0) {
      if (data[0] == 0) {
        display.turnOffDisplay();
      } else {
        display.toggleDisplay();
      }
      if (display.getDisplay()) {
        Serial.println("Setting LEDPin LOW");
        digitalWrite(LEDPin, LOW);
      } else {
        Serial.println("Setting LEDPin HIGH");
        digitalWrite(LEDPin, HIGH);
      }
    }

    for (size_t i = 0; i < sz; i++) {
      Serial.print("Received a ");Serial.println(data[i]);
    }
  });

  // Use the default MQTT broker name ("mqtt.gigabits.io")
  // Use the default port number 8883
  gigabits.begin(devKey, secret, net);
}
