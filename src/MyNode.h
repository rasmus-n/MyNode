#ifndef MY_NODE_H_
#define MY_NODE_H_

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "esp_http_server.h"
#include <Preferences.h>

class MyNode
{
public:
  MyNode();

  void add_topic(const char* topic, void (*handler)(const char* topic, uint8_t* payload, unsigned int length));
  void add_topic(const char* topic, void (*handler)(const char* topic, const char* payload));
  void add_topic(const char* topic, void (*handler)(const char* topic, int payload));
  void publish(const char* topic, const char* payload);
  void publish(const char* topic, int payload);
  void publish_retain(const char* topic, const char* payload);
  void publish_retain(const char* topic, int payload);

  void setup();
  void loop();

private:
  void ConfigNode();
  void reconnect();

  bool mConfigured = false;
  unsigned int m_max_retry_count = 5;

  char mMQTTServer[16];
};

extern MyNode myNode;

#endif /* MY_NODE_H_ */