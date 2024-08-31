#include <Arduino.h>
#include "Camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
using namespace websockets;

WebsocketsClient client;
WebsocketsClient streamClient;
const char* ssid     = "Stark";  // 替换为您的 Wi-Fi 网络名称
const char* password = "fengzhiping,1101";  // 替换为您的 Wi-Fi 密码
const char* ws_control_url = "ws://192.168.2.153:3000/mobile"; //Enter server adress
const char* ws_stream_url = "ws://192.168.2.153:3000/mobile/camera/stream"; //Enter server adress


Camera camera;
void onControlMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
    JsonDocument doc;
    deserializeJson(doc, message.data());
    if (doc["command"] == 10) {
      camera.increaseFrameSize();
      Serial.println("increaseFrameSize");
    } else if (doc["command"] == 20) {
      camera.decreaseFrameSize();
      Serial.println("decreaseFrameSize");
    }
}

void onControlEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Control Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Control Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Control Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Control Got a Pong!");
    }
}

void onStreamMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onStreamEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Stream Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Stream Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Stream Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Stream Got a Pong!");
    }
}

void setup() {
  Serial.begin(9600);
  
  camera.setUp();

  WiFi.begin("Stark", "fengzhiping,1101");

  Serial.println("Connecting to WiFi...");
  while (WiFi.status()!= WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  camera.startStreamServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  client.onMessage(onControlMessageCallback);
  client.onEvent(onControlEventsCallback);
  client.connect(ws_control_url);

  streamClient.onMessage(onStreamMessageCallback);
  streamClient.onEvent(onStreamEventsCallback);
  streamClient.connect(ws_stream_url);
}

void loop() {
  client.poll();
  streamClient.poll();
}