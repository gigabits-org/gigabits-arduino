/*
 * This sketch demonstrates using the Gigabits device library
 * with an ESP32 using an unencrypted connection.  The sensors are arranged on * a breadboard, so the NCD sensors are not used.  The microcontroller is:
 * HiLetgo ESP-WROOM-32 ESP32 ESP-32S Development Board 2.4GHz Dual-Mode WiFi + * Bluetooth Dual Cores Microcontroller Processor Integrated with Antenna RF
 * AMP Filter AP STA for Arduino IDE
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
#include <WiFi.h>
// Used to read DHT sensors
#include <DHTesp.h>
#include <WiFiClient.h>
#include <Gigabits.h>

// Change these!
char ssid[] = "Bark Place";
char pass[] = "31f28065b6c93ee750b211de84";
char devKey[] = "DuIVac7AsqI5wSoSEPJAGpr";
char secret[] = "N9nDy9IvX9qnQixnfiYTVyM5jLb9mGJH";

WiFiClient net;

// Define the ESP32 pins used.  The constraints for pin use are described in
// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// The mapping between physical pin number and GPIO is shown in https://forum.fritzing.org/t/esp32s-hiletgo-dev-boad-with-pinout-template/5357
// We'd like to use ADC pins.  It looks like there are lots of them, but there
// are only about two left after removing the pins used in for WiFi, the ones
// with no pullup or pulldown and the ones that didn't make it onto the package
// from the module.  The pin used by the DH22 is used as both an input and an
// output, so it gets one of the last two pins that can be an input or an
// output.  We'll add pullup resistors to 3 pins and use them as inputs.

int lightSensorPin = 36;     // physical pin 3
int gasSensorPin   = 39;     // physical pin 4
int dhtPin         = 32;     // physical pin 7.  This must be on a pin that
                             // can be either input or output.  The code that
                             // reads it uses both modes.
int hygrometerPin  = 34;     // physical pin 5

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
  }
}

DHTesp dht;
TaskHandle_t tempTastHandle = NULL;
// Reference https://github.com/beegee-tokyo/DHTesp/blob/master/examples/DHT_Multi_ESP32/DHT_Multi_ESP32.ino
void setupDHT() {
  // Set up the code that's used to read the DHT humidity and temperature
  /** Initialize DHT sensor */

  // Initialize the interface to the sensor and tell the library which sensor
  // type will be used
  dht.setup(dhtPin, DHTesp::DHT22);
}

void sendDHTData() {
  Serial.println("Sending DHT Data...");

  TempAndHumidity tempHumidity = dht.getTempAndHumidity();

  float humidity = (float)(tempHumidity.humidity);
  float fTemp = dht.toFahrenheit((float)tempHumidity.temperature);

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
  // The deocumetation says no light => small signal,
  // but it works the other way around.
  float raw_adc = (float)(4096 - intLightData);
  Serial.print("Light Sensor: "); Serial.println(raw_adc);
  gigabits.sendRecord(VISIBLE_LIGHT_SENSOR_IDX, raw_adc);
}

void setupGigabits() {
  // Port 1883 is for plain traffic
  gigabits.begin(devKey, secret, net, "mqtt.gigabits.io", 1883);
}
