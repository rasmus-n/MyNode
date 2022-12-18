#ifndef MQTT_HANDLER_H_
#define MQTT_HANDLER_H_

#include "Arduino.h"

class cMQTTHandler
{
  public:
    cMQTTHandler(const char* topic, void (*handler)(const char* topic, uint8_t* payload, unsigned int length))
    {
      strcpy(mTopic, topic);
      mHandler = handler;
    };
    cMQTTHandler(const char* topic, void (*handler)(const char* topic, const char* payload))
    {
      strcpy(mTopic, topic);
      mHandlerString = handler;
    };
    cMQTTHandler(const char* topic, void (*handler)(const char* topic, int payload))
    {
      strcpy(mTopic, topic);
      mHandlerInt = handler;
    };

    const char* topic() {return mTopic;};

    void exe (const char* topic, uint8_t* payload, unsigned int length)
    {
      char str[length+1];
      if(mHandler != NULL)
      {
        mHandler(topic, payload, length);
      }
      if(mHandlerString != NULL)
      {
        strncpy(str, (char*) payload, length);
        str[length] = 0;
        mHandlerString(topic, str);
      };
      if(mHandlerInt != NULL)
      {
        strncpy(str, (char*) payload, length);
        str[length] = 0;
        mHandlerInt(topic, atoi(str));
      };
    };

  private:
    void (*mHandler)(const char* topic, uint8_t* payload, unsigned int length) = NULL;
    void (*mHandlerString)(const char* topic, const char* payload) = NULL;
    void (*mHandlerInt)(const char* topic, int payload) = NULL;
    char mTopic[32];
};

#endif