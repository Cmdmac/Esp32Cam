#include <Arduino.h>
#include "Camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
using namespace websockets;

WebsocketsClient client;
WebsocketsClient streamClient;
const char* ssid     = "Stark";  // 替换为您的 Wi-Fi 网络名称
const char* password = "fengzhiping,1101";  // 替换为您的 Wi-Fi 密码
const char* ws_control_url = "ws://192.168.1.4:3000/mobile/camera/control"; //Enter server adress
const char* ws_stream_url = "ws://192.168.1.4:3000/mobile/camera/stream"; //Enter server adress


void onControlMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onControlEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

void onStreamMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onStreamEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

Camera camera;
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
}