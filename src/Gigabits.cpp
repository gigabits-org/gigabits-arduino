#include "Gigabits.h"

Gigabits::Gigabits() {}

Gigabits::~Gigabits() {
  for (int i=0; i < cMapIdx; i++) {
      free(commandMaps[i]);
  }
}

void Gigabits::connect() {
    Serial.println("Connecting.");

    while (!client.connect(devKey, "gigabits", "gigabits")) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("\nconnected!");

    client.subscribe(rxTopic);
}

void Gigabits::attachMessageHandler() {
    client.onMessage([this](String &topic, String &payload){
      Serial.println("Dev key: " + String(this->devKey));
      Serial.println("incoming: " + topic + " - " + payload);
  
      DeserializationError error = deserializeJson(this->rxBuf, payload);
      if (error) {
        Serial.println("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
  
      uint32_t sensorIndex = this->rxBuf["si"];
      const char* argType = this->rxBuf["at"];
  
      auto cb = getCallback(sensorIndex);        
      if (cb == nullptr) {
        Serial.println("Callback not defined");
        return;
      }

      // Handle string command
      if (strcmp(argType,"str") == 0) {
        const char* data = this->rxBuf["c"];
        String ret = String(data);
        cb->strCb(ret);
        // Report back the string received
        this->sendRecord(sensorIndex, ret);
        return;
      }

      // Other types will come in as an array
      JsonArray arr = this->rxBuf["c"].as<JsonArray>();
      if (arr.isNull()) {
        Serial.println("Command is not array");
        return;
      }

      // Handle integer array
      if (strcmp(argType,"int") == 0) {
        if (cb->intCb) {
          int32_t *data = (int32_t*)malloc(sizeof(int32_t) * arr.size());
          int i = 0;
          for (JsonVariant v : arr) {
            data[i++] = v.as<int>();
          }
        
          cb->intCb(data, arr.size());
          free(data);
          

        } else {
          Serial.println("Callback not defined");
          return;
        }

      // Handle double array
      } else if (strcmp(argType,"dbl") == 0) {
        if (cb->dblCb) {
          double *data = (double*)malloc(sizeof(double) * arr.size());
          int i = 0;
          for (JsonVariant v : arr) {
            data[i++] = v.as<double>();
          }
          cb->dblCb(data, arr.size());
          free(data);
        } else {
          Serial.println("Callback not defined");
          return;
        }
      }

      // Report back the command received
      String ret;
      serializeJson(arr, ret);
      this->sendRecord(sensorIndex, ret);
  });
}

bool Gigabits::begin(const char *inDevKey, Client &net) {
    txBuf.to<JsonObject>();
    devKey = inDevKey;
    sprintf(txTopic, "device/%s/records", devKey);
    sprintf(rxTopic, "server/%s/command", devKey);
    
    client.begin("api.dev.gigabits.io", net);
    attachMessageHandler();

    connect();    
}

bool Gigabits::run() {
  if (millis() - lastRun > 10) {
    lastRun = millis();
    transmitValues();
        
    client.loop();

    if (!client.connected()) {
        connect();
    }
  }  

}

void Gigabits::transmitValues() {
  JsonObject txObj = txBuf.as<JsonObject>();      

  if (txObj.size() > 0) {
    Serial.println("Sending buffer");
    String toSend;
    serializeJson(txObj, toSend);
    client.publish(txTopic, toSend);
    
    // Clear tx buffer
    txBuf.to<JsonObject>();
  }
}

// Saves value to the txBuffer as an integer
void Gigabits::sendRecord(uint32_t idx, uint32_t value) {
    txBuf[String(idx)] = value;
}

// Saves value to the txBuffer as a double
void Gigabits::sendRecord(uint32_t idx, double value) {
    txBuf[String(idx)] = value;
}

// Saves value to the txBuffer as a float
void Gigabits::sendRecord(uint32_t idx, float value) {
    txBuf[String(idx)] = value;
}

// Saves value to the txBuffer as a string
void Gigabits::sendRecord(uint32_t idx, String value) {
    txBuf[String(idx)] = value;
}

// Allocates memory for a new callback map
IndexCallbackMap* Gigabits::makeCommandListener(uint32_t idx) {
  IndexCallbackMap *v = (IndexCallbackMap*)malloc(sizeof(IndexCallbackMap));
  v->idx = idx;
  v->dblCb = nullptr;
  v->strCb = nullptr;
  v->intCb = nullptr;
  return v;
}

// Add an integers callback
void Gigabits::addCommandListener(uint32_t idx, GigabitsCallbackIntegers cb) {
    if (cMapIdx == GIGABITS_MAX_CALLBACKS) {
      return;
    }
    IndexCallbackMap *v = makeCommandListener(idx);
    v->intCb = cb;
    commandMaps[cMapIdx++] = v;
}

// Add a doubles callback
void Gigabits::addCommandListener(uint32_t idx, GigabitsCallbackDoubles cb) {
    if (cMapIdx == GIGABITS_MAX_CALLBACKS) {
      return;
    }
    IndexCallbackMap *v = makeCommandListener(idx);
    v->dblCb = cb;
    commandMaps[cMapIdx++] = v;
}

// Add a string callback
void Gigabits::addCommandListener(uint32_t idx, GigabitsCallbackString cb) {
    if (cMapIdx == GIGABITS_MAX_CALLBACKS) {
      return;
    }
    IndexCallbackMap *v = makeCommandListener(idx);
    v->strCb = cb;
    commandMaps[cMapIdx++] = v;
}

// Slow search for matching callback
IndexCallbackMap* Gigabits::getCallback(uint32_t idx) {
  for (int i = 0; i < cMapIdx; i++) {
    if (commandMaps[i]->idx == idx) {
      return commandMaps[i];
    }
  }
  return nullptr;
}
