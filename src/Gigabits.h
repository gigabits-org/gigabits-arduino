#ifndef Gigabits_H__
#define Gigabits_H__

#ifndef GIGABITS_MAX_CALLBACKS
#define GIGABITS_MAX_CALLBACKS 10
#endif

#ifndef GIGABITS_JSON_SIZE
#define GIGABITS_JSON_SIZE 500
#endif

#include "mqtt/MQTT.h"
#include <Client.h>
#include <ArduinoJson.h>

typedef void (*GigabitsCallbackIntegers)(int32_t *commands, size_t sz);
typedef void (*GigabitsCallbackDoubles)(double *commands, size_t sz);
typedef void (*GigabitsCallbackString)(String &command);

typedef struct {
  uint32_t idx;
  GigabitsCallbackIntegers intCb = nullptr;
  GigabitsCallbackDoubles dblCb = nullptr;
  GigabitsCallbackString strCb = nullptr;
} IndexCallbackMap;

class Gigabits {
  
    public:
        Gigabits();
        ~Gigabits();

        bool begin(const char *devKey, const char *devSecret, Client &net) { this->begin(devKey, devSecret, net, "mqtt.gigabits.io"); };
        bool begin(const char *devKey, const char *devSecret, Client &net, const char *endpoint) { this->begin(devKey, devSecret, net, endpoint, 8883); };
        bool begin(const char *devKey, const char *devSecret, Client &net, const char *endpoint, uint16_t port);
        bool run();
        
        void sendRecord(uint32_t idx, uint32_t value);
        void sendRecord(uint32_t idx, double value);
        void sendRecord(uint32_t idx, float value);
        void sendRecord(uint32_t idx, String value);

        void addCommandListener(uint32_t idx, GigabitsCallbackIntegers cb);
        void addCommandListener(uint32_t idx, GigabitsCallbackDoubles cb);
        void addCommandListener(uint32_t idx, GigabitsCallbackString cb);
        void messageReceived(String &topic, String &payload);
    private:
        MQTTClient client;
        unsigned long lastRun = 0;
        const char *devKey = nullptr;
        const char *devSecret = nullptr;
        void connect();
        void attachMessageHandler();
        void transmitValues();
        char txTopic[48];
        char rxTopic[48];
#ifdef GIGABITS_USE_DYNAMIC_JSON
        DynamicJsonDocument txBuf(GIGABITS_JSON_SIZE);
        DynamicJsonDocument rxBuf(GIGABITS_JSON_SIZE);
#else
        StaticJsonDocument<GIGABITS_JSON_SIZE> txBuf;
        StaticJsonDocument<GIGABITS_JSON_SIZE> rxBuf;
#endif
        uint16_t cMapIdx = 0;
        IndexCallbackMap* commandMaps[GIGABITS_MAX_CALLBACKS];
        IndexCallbackMap* getCallback(uint32_t idx);
        IndexCallbackMap* makeCommandListener(uint32_t idx);
};

#endif
