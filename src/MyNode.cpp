#include "MyNode.h"

#include "cMQTTHandler.h"
#include <vector>

WiFiClient mWifi;
PubSubClient mMqtt(mWifi);
Preferences mPref;
httpd_handle_t httpd = NULL;

void mqtt_callback(const char* topic, uint8_t* payload, unsigned int length);
static esp_err_t cmd_handler(httpd_req_t *req);
static esp_err_t root_handler(httpd_req_t *req);

std::vector<cMQTTHandler> mHandlers{};

MyNode myNode;

MyNode::MyNode()
{
}

void MyNode::setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  mPref.begin("cfg", true);

  String hostname;
  String broker;
  hostname = mPref.getString("hostname", WiFi.getHostname());
  broker = mPref.getString("broker", "piport");
  Serial.println(hostname);
  Serial.println(broker);
  strncpy(mMQTTServer, broker.c_str(), 16);
  mPref.end();

  WiFi.setHostname(hostname.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  unsigned long timeout = millis() + 20000;
  unsigned long nextDot = millis() + 500;
  unsigned long now;

  while (WiFi.status() != WL_CONNECTED)
  {
    now = millis();
    if (now > nextDot)
    {
      Serial.print(".");
      nextDot = now + 500;
    }
    
    if (now > timeout)
    {
      ConfigNode();
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print(WiFi.localIP());

  ArduinoOTA.begin();

  mMqtt.setServer(mMQTTServer, 1883);
  mMqtt.setCallback(mqtt_callback);
  mConfigured = true;
}

void MyNode::loop()
{
  static long int lastMQTTReconnect = 0;
  long int now = millis();

  if (mConfigured && !mMqtt.connected() && ((now - lastMQTTReconnect) > 5000)) {
    reconnect();
    lastMQTTReconnect = now;
  }

  ArduinoOTA.handle();
  mMqtt.loop();
}

void MyNode::reconnect()
{
  static unsigned int reconnect_count = 0;
  Serial.print("Attempting MQTT connection...");

  if (mMqtt.connect(WiFi.getHostname())) {
    Serial.println("connected");
    reconnect_count = 0;

    Serial.println("Subscribing to:");
    for (auto it = begin(mHandlers); it != end(mHandlers); ++it)
    {
      Serial.println(it->topic());
      mMqtt.subscribe(it->topic());
      mMqtt.loop();
      yield();
    }
  }
  else
  {
    Serial.println("Failed");
    reconnect_count++;
    if (reconnect_count > m_max_retry_count)
    {
      reconnect_count = 0;
      ConfigNode();
    }
  }
}

void MyNode::add_topic(const char* topic, void (*handler)(const char* topic, uint8_t* payload, unsigned int length))
{
  mHandlers.push_back(cMQTTHandler{topic, handler});
}

void MyNode::add_topic(const char* topic, void (*handler)(const char* topic, const char* payload))
{
  mHandlers.push_back(cMQTTHandler{topic, handler});
}

void MyNode::add_topic(const char* topic, void (*handler)(const char* topic, int payload))
{
  mHandlers.push_back(cMQTTHandler{topic, handler});
}

void MyNode::publish(const char* topic, const char* payload)
{
  mMqtt.publish(topic, payload);
}

void MyNode::publish(const char* topic, int payload)
{
  char n[8];
  mMqtt.publish(topic, itoa(payload, n, 10));
}

void MyNode::publish_retain(const char* topic, const char* payload)
{
  mMqtt.publish(topic, payload, true);
}

void MyNode::publish_retain(const char* topic, int payload)
{
  char n[8];
  mMqtt.publish(topic, itoa(payload, n, 10), true);
}

void MyNode::ConfigNode()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  mPref.begin("cfg");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WiFi.getHostname(), "secret1234");
  Serial.println(WiFi.softAPIP());

  httpd_uri_t root_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = root_handler,
      .user_ctx  = NULL
  };

    httpd_uri_t cmd_uri = {
      .uri       = "/set",
      .method    = HTTP_GET,
      .handler   = cmd_handler,
      .user_ctx  = NULL
  };

  Serial.printf("Starting web server on port: '%d'\n", config.server_port);

  if (httpd_start(&httpd, &config) == ESP_OK)
  {
      httpd_register_uri_handler(httpd, &root_uri);
      httpd_register_uri_handler(httpd, &cmd_uri);
  }
}

void mqtt_callback(const char* topic, uint8_t* payload, unsigned int length)
{
  for (auto it = begin(mHandlers); it != end(mHandlers); ++it)
  {
    if(strcmp(it->topic(), topic) == 0)
    {
//      Serial.println(topic);
      it->exe(topic, payload, length);
    }
  }
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
  Serial.println("set");

  char*  buf;
  size_t buf_len;
  char ssid[32] = {0,};
  char pw[32] = {0,};
  char hostname[32] = {0,};
  char broker[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
      buf = (char*)malloc(buf_len);
      if(!buf){
          httpd_resp_send_500(req);
          return ESP_FAIL;
      }
      if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
          if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
              httpd_query_key_value(buf, "pw", pw, sizeof(pw)) == ESP_OK &&
              httpd_query_key_value(buf, "hostname", hostname, sizeof(pw)) == ESP_OK &&
              httpd_query_key_value(buf, "broker", broker, sizeof(broker)) == ESP_OK){
          } else {
              free(buf);
              httpd_resp_send_404(req);
              return ESP_FAIL;
          }
      } else {
          free(buf);
          httpd_resp_send_404(req);
          return ESP_FAIL;
      }
      free(buf);
  } else {
      httpd_resp_send_404(req);
      return ESP_FAIL;
  }
  Serial.println(ssid);
  Serial.println(pw);
  Serial.println(hostname);
  Serial.println(broker);

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  esp_err_t res = httpd_resp_send(req, NULL, 0);

  mPref.putString("hostname", hostname);
  mPref.putString("broker", broker);
  mPref.end();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  ESP.restart();

  return res;
}

static esp_err_t root_handler(httpd_req_t *req)
{
  Serial.println("root");
  char response[512];
  char* p = response;
  
  p+=sprintf(p, "<html>");
  p+=sprintf(p, "<meta content='width=device-width, initial-scale=1' name='viewport'/>");
  p+=sprintf(p, "<h1>HEJ</h1>");
  p+=sprintf(p, "<form action='/set' method='get'>");
  p+=sprintf(p, "SSID: <input type='text' name='ssid' value='%s'><br>", WiFi.SSID().c_str());
  p+=sprintf(p, "PW: <input type='text' name='pw'><br>");
  p+=sprintf(p, "Hostname: <input type='text' name='hostname' value='%s'><br>", WiFi.getHostname());
  p+=sprintf(p, "Broker: <input type='text' name='broker' value='%s'><br>", mPref.getString("broker", "piport").c_str());
  p+=sprintf(p, "<input type='submit' value='Submit'><br>");
  p+=sprintf(p, "</form>");
  p+=sprintf(p, "</html>");

  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, response, strlen(response));
}

