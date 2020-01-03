/*
 *  This sketch demonstrates using the Gigabits device library 
 *  with an ESP32 using an unencrypted connection
 *  It reads data from the following sensors using I2C:
 *  ADC121C021 (Gas)
 *  ADC121C021 (Soil)
 *  TMD26721 (Proxy)
 *  MPL115A2 (Pressure)
 *  HCPA_5V_U3 (Temperature and Humidity)
 *  HP203B (Altitude)
 *  TSL45315 (Light)
 * 
 *  It sends commands for the following:
 *  SSD1306 (Display)
 */
// Set max callbacks to store on the stack. Defaults to 10
#define GIGABITS_MAX_CALLBACKS 20
// Set size of JSON buffers. Defaults to 500
#define GIGABITS_JSON_SIZE 1000
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Gigabits.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
// Screen size is 128x32
Adafruit_SSD1306 display(128, 32, &Wire, OLED_RESET);

// Change these!
char ssid[] = "ssid";
char pass[] = "pass";
char devKey[] = "devkey";
char secret[] = "secret";

WiFiClient net;

// I2C addresses
#define HCPA_Addr 0x28
#define PROXY_Addr 0x39
#define OLED_Addr 0x3C
#define PCA_Addr 0x40
#define ADC_Addr 0x50
#define SOIL_Addr 0x51
#define GAS_Addr 0x52
#define MPL_Addr 0x60
#define HP203_Addr 0x77
#define TSL_Addr 0x29

// Gigabits sensor indices
#define HUMIDITY_SENSOR_IDX 1
#define TEMPERATURE_SENSOR_IDX 2
#define OLED_INVERT_COMMAND_IDX 3
#define PRESSURE_SENSOR_IDX 4
#define GAS_SENSOR_IDX 5
#define SOIL_SENSOR_IDX 6
#define PROXY_SENSOR_IDX 7
#define ALTITUDE_SENSOR_IDX 8
#define LIGHT_SENSOR_IDX 9

#define ADC_IDX 10

#define PCA_CH1_IDX 21
#define PCA_CH2_IDX 22
#define PCA_CH3_IDX 23
#define PCA_CH4_IDX 24

Gigabits gigabits;

unsigned long lastMillis = 0;

// MPL pressure compensation values
float a1 = 0.0, b1 = 0.0, b2 = 0.0, c12 = 0.0;

char err_msg[] = "Error: d";

void setup() {
  Wire.begin();
  Serial.begin(115200);
  setupDisplay();
  setupHCPA();
  setupMPL();
  setupProxy();
  setupHP203();
  setupTSL();
  WiFi.begin(ssid, pass);
  Serial.println("Connecting..");
  
  delay(300);  
  
  setupGigabits();  

}

void loop() {
  // This should be called on very often
  gigabits.run();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    sendHCPAData();
    sendMPLData();
    sendGasData();
    sendProxyData();
    sendSoilData();
    sendHP203Data();
    sendTSLData();
  }

}

// Reference https://github.com/ControlEverythingCommunity/HCPA-5V-U3/blob/master/Arduino/HCPA_5V_U3.ino
void setupHCPA() {
  // Start I2C transmission
  Wire.beginTransmission(HCPA_Addr);
  // Send start command
  Wire.write(0x80);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(300);
}

void sendHCPAData() {
  Serial.println("Sending HCPA...");
  unsigned int data[4];

  // Start I2C transmission
  Wire.beginTransmission(HCPA_Addr);
  // Stop I2C transmission
  byte error = Wire.endTransmission();

  if (error == 0) {
    // Request 4 bytes of data
    Wire.requestFrom(HCPA_Addr, 4);
  
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
    
    gigabits.sendRecord(HUMIDITY_SENSOR_IDX, humidity);
    gigabits.sendRecord(TEMPERATURE_SENSOR_IDX, fTemp);  
  } else {
    gigabits.sendRecord(HUMIDITY_SENSOR_IDX, err_msg);
    gigabits.sendRecord(TEMPERATURE_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/MPL115A2/blob/master/Arduino/MPL115A2.ino
void setupMPL() {

  unsigned int data[8];
  
  for (int i = 0; i < 8; i++) {
    // Start I2C Transmission
    Wire.beginTransmission(MPL_Addr);
    // Select data register
    Wire.write(4 + i);
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(MPL_Addr, 1);

    // Read 1 byte of data
    // A10 msb, A10 lsb, Bb1 msb, Bb1 lsb, B2 msb, B2 lsb, C12 msb, C12 lsb
    if (Wire.available() == 1)
    {
      data[i] = Wire.read();
    }
  }

  // Convert the data to floating points
  a1 = ((data[0] * 256.0) + data[1]) / 8.0;
  b1 = ((data[2] * 256) + data[3]);
  if (b1 > 32767)
  {
    b1 -= 65536;
  }
  b1 = b1 / 8192.0;
  b2 = ((data[4] * 256) + data[5]);
  if (b2 > 32767)
  {
    b2 -= 65536;
  }
  b2 = b2 / 16384.0;
  c12 = ((data[6] * 256.0 + data[7]) / 4.0) / 4194304.0;
  
  delay(300);
}

void sendMPLData() {
  Serial.println("Sending Pressure");
  unsigned int data[4];
  // Start I2C Transmission
  Wire.beginTransmission(MPL_Addr);
  // Send Pressure measurement command
  Wire.write(0x12);
  // Start conversion
  Wire.write(0x00);
  // Stop I2C Transmission
  Wire.endTransmission();
  delay(300);

  // Start I2C Transmission
  Wire.beginTransmission(MPL_Addr);
  // Select data register
  Wire.write(0x00);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Request 4 bytes of data
  Wire.requestFrom(MPL_Addr, 4);

  // Read 4 bytes of data
  // pres msb, pres lsb, temp msb, temp lsb
  if (Wire.available() == 4)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();

    // Convert the data to 10-bits
    int pres = (data[0] * 256 + (data[1] & 0xC0)) / 64;
    int temp = (data[2] * 256 + (data[3] & 0xC0)) / 64;

    // Calculate pressure compensation
    float presComp = a1 + (b1 + c12 * temp) * pres + b2 * temp;

    // Convert the data
    float pressure = (65.0 / 1023.0) * presComp + 50.0;
    float cTemp = (temp - 498) / (-5.35) + 25.0;
    float fTemp = cTemp * 1.8 + 32.0;

    gigabits.sendRecord(PRESSURE_SENSOR_IDX, pressure);
  } else {
    gigabits.sendRecord(PRESSURE_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/ADC121C021/blob/master/Arduino/ADC121C021.ino
void sendGasData() {
  Serial.println("Sending Gas");
  unsigned int data[2];
  // Start I2C Transmission
  Wire.beginTransmission(GAS_Addr);
  // Calling conversion result register, 0x00(0)
  Wire.write(0x00);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Request 2 bytes of data
  Wire.requestFrom(GAS_Addr, 2);
  
  // Read 2 bytes of data
  // raw_adc msb, raw_adc lsb
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    
    // Convert the data to 12 bits
    uint32_t raw_adc = ((data[0] & 0x0F) * 256) + data[1];
    gigabits.sendRecord(GAS_SENSOR_IDX, raw_adc);
  } else {
    gigabits.sendRecord(GAS_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/TMD26721/blob/master/Arduino/TMD26721.ino
void setupProxy() {
  // Start I2C Transmission
  Wire.beginTransmission(PROXY_Addr);
  // Select enable register
  Wire.write(0x00 | 0x80);
  // Set power on, proximity and wait enabled
  Wire.write(0x0D);
  // Stop I2C Transmission
  Wire.endTransmission();
  delay(500);

  // Start I2C Transmission
  Wire.beginTransmission(PROXY_Addr);
  // Select proximity time control register
  Wire.write(0x02 | 0x80);
  // time = 2.73 ms
  Wire.write(0xFF);
  // Stop I2C Transmission
  Wire.endTransmission();

  // Start I2C Transmission
  Wire.beginTransmission(PROXY_Addr);
  // Select proximity time register
  Wire.write(0x0F | 0x80);
  // Proximity uses Ch0 diode
  Wire.write(0x20);
  // Stop I2C Transmission
  Wire.endTransmission();
  delay(300);
}

void sendProxyData() {
  Serial.println("Sending proxy");
  unsigned int data[2];
  
  // Start I2C Transmission
  Wire.beginTransmission(PROXY_Addr);
  // Select data register
  Wire.write(0x18 | 0x80);
  // Stop I2C Transmission
  Wire.endTransmission();
  
  // Request 2 bytes of data
  Wire.requestFrom(PROXY_Addr, 2);
  
  // Read 2 bytes of data
  // proximity lsb, proximity msb
  if(Wire.available() == 2) {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data
    uint32_t proximity = data[1] * 256 + data[0];

    gigabits.sendRecord(PROXY_SENSOR_IDX, proximity);

  } else {
    gigabits.sendRecord(PROXY_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/ADC121C021/blob/master/Arduino/ADC121C021.ino
void sendSoilData() {
  Serial.println("Sending Soil");
  unsigned int data[2];
  // Start I2C Transmission
  Wire.beginTransmission(SOIL_Addr);
  // Calling conversion result register, 0x00(0)
  Wire.write(0x00);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Request 2 bytes of data
  Wire.requestFrom(SOIL_Addr, 2);
  
  // Read 2 bytes of data
  // raw_adc msb, raw_adc lsb
  if(Wire.available() == 2) {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data to 12 bits
    uint32_t raw_adc = ((data[0] & 0x0F) * 256) + data[1];  
    gigabits.sendRecord(SOIL_SENSOR_IDX, raw_adc);
  } else {
    gigabits.sendRecord(SOIL_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/HP203B/blob/master/Arduino/HP203B.ino
void setupHP203() {
  // Start I2C transmission
  Wire.beginTransmission(HP203_Addr);
  // Send OSR and channel setting command
  Wire.write(0x40 | 0x04 | 0x01);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(300);
}

void sendHP203Data() {
  Serial.println("Sending altitude");
  unsigned int data[3];

  // Start I2C transmission
  Wire.beginTransmission(HP203_Addr);
  // Select data register
  Wire.write(0x31);
  // Stop I2C transmission
  Wire.endTransmission();

  // Request 3 bytes of data
  Wire.requestFrom(HP203_Addr, 3);

  // Read 3 bytes of data
  // altitude msb, altitude csb, altitude lsb
  if (Wire.available() == 3) {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    
    // Convert the data to 20-bits
    float altitude = (((data[0] & 0x0F) * 65536) + (data[1] * 256) + data[2]) / 100.00;

    gigabits.sendRecord(ALTITUDE_SENSOR_IDX, altitude);

  } else {
    gigabits.sendRecord(ALTITUDE_SENSOR_IDX, err_msg);
  }
}

// Reference https://github.com/ControlEverythingCommunity/TSL45315/blob/master/Arduino/TSL45315.ino
void setupTSL() {
  // Start I2C Transmission
  Wire.beginTransmission(TSL_Addr);
  // Select control register
  Wire.write(0x80);
  // Normal operation
  Wire.write(0x03);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Start I2C Transmission
  Wire.beginTransmission(TSL_Addr);
  // Select configuration register
  Wire.write(0x81);
  // Multiplier 1x, Tint : 400ms
  Wire.write(0x00);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(300);  
}

void sendTSLData() {
  Serial.println("Sending luminance");
  unsigned int data[2];
  // Start I2C Transmission
  Wire.beginTransmission(TSL_Addr);
  // Select data register
  Wire.write(0x84);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Request 2 bytes of data
  Wire.requestFrom(TSL_Addr, 2);
  // Read 2 bytes of data
  // luminance lsb, luminance msb
  if(Wire.available() == 2) {
    data[0] = Wire.read();
    data[1] = Wire.read();
    
    // Convert the data
    float luminance = data[1] * 256 + data[0];
    gigabits.sendRecord(LIGHT_SENSOR_IDX, luminance);
  } else {
    gigabits.sendRecord(LIGHT_SENSOR_IDX, err_msg);
  }
  

}

// Reference https://github.com/adafruit/Adafruit_SSD1306/tree/master/examples/ssd1306_128x32_i2c
void setupDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_Addr)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
}

void setupGigabits() {
    // Add command listener to invert OLED display using a lambda function
  gigabits.addCommandListener(OLED_INVERT_COMMAND_IDX, [](int32_t *data, size_t sz){
    if (sz > 0) {
      display.invertDisplay(data[0]);
    }

    for (size_t i = 0; i < sz; i++) {
      Serial.print("Received a ");Serial.println(data[i]);
    }
  });

  // Port 1883 is for plain traffic
  gigabits.begin(devKey, secret, net, "mqtt.gigabits.io", 1883);
}