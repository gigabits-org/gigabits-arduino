#include "Gigabits.h"

Gigabits::Gigabits() {}

Gigabits::~Gigabits() {
  for (int i=0; i < cMapIdx; i++) {
      free(commandMaps[i]);
  }
}

void Gigabits::connect() {

    while (!client.connect(devKey, "gigabits", "gigabits")) {
        delay(1000);
    }

    client.subscribe(rxTopic);
}

void Gigabits::attachMessageHandler() {
    client.onMessage([this](String &topic, String &payload){

      DeserializationError error = deserializeJson(this->rxBuf, payload);
      // Json failed to parse
      if (error) {
        return;
      }
  
      uint32_t sensorIndex = this->rxBuf["si"];
      const char* argType = this->rxBuf["at"];
  
      auto cb = getCallback(sensorIndex);
      // Callback not defined
      if (cb == nullptr) {
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
          
        // Callback not defined
        } else {
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

        // Callback not defined
        } else {
          return;
        }
      }

      // Report back the command received
      String ret;
      serializeJson(arr, ret);
      this->sendRecord(sensorIndex, ret);
  });
}

// Connects to the Gigabits API
bool Gigabits::begin(const char *inDevKey, Client &net) {
    txBuf.to<JsonObject>();
    devKey = inDevKey;
    sprintf(txTopic, "device/%s/records", devKey);
    sprintf(rxTopic, "server/%s/command", devKey);
    
    client.begin(F("api.dev.gigabits.io"), net);
    attachMessageHandler();

    connect();    
}

// Run mqtt client at most every 10ms
// Keeps the client connected
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

// Unload the tx buffer over mqtt and clear it
void Gigabits::transmitValues() {
  JsonObject txObj = txBuf.as<JsonObject>();      

  if (txObj.size() > 0) {
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
