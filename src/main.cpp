#include <Arduino.h>
#include "Camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include "command.h"

#include <I2S.h>
#include "Audio.h"
#include "SDWebServer.h"
using namespace websockets;

WebsocketsClient client;
WebsocketsClient streamClient;
const char* ssid     = "Stark";  // 替换为您的 Wi-Fi 网络名称
const char* password = "fengzhiping,1101";  // 替换为您的 Wi-Fi 密码
const char* ws_control_url = "ws://192.168.2.153:3000/mobile/camera/control?client=esp32cam"; //Enter server adress
const char* ws_stream_url = "ws://192.168.2.153:3000/mobile/camera/stream?client=esp32cam"; //Enter server adress

Audio audio;
Camera camera;
SDWebServer sdWebServer;

void onControlMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
    JsonDocument doc;
    deserializeJson(doc, message.data());
    int cmd = doc["command"];
    switch(cmd) {
        case START_STREAM:
            camera.starStreamHandler(streamClient);
            break;
        case STOP_STREAM:
        break;
        case INCREASE_JPG_QUALITY:                                 
            camera.increaseFrameSize();
        break;
        case DECREASE_JPG_QUALITY:
            camera.decreaseFrameSize();
        break;
        case INCREASE_FRAME_SIZE:
        break;
        case DECREASE_FRAME_SIZE:
        break;
        default:
        break;
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
        // WSInterfaceString s = "123456dads";
        // streamClient.sendBinary(s);
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Stream Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Stream Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Stream Got a Pong!");
    }
}

void task1Function1(void* p) {

    camera.starStreamHandler2(streamClient);
}

void task1Function2(void* p) {

    camera.sendCache(streamClient);
}

void setup() {
  Serial.begin(9600);
  
  camera.setUp();

  WiFi.begin("Stark", "fengzhiping,1101");

  Serial.println("Connecting to WiFi...");
  while (WiFi.status()!= WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

//   camera.startStreamServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  client.onMessage(onControlMessageCallback);
  client.onEvent(onControlEventsCallback);
  client.connect(ws_control_url);

  streamClient.onMessage(onStreamMessageCallback);
  streamClient.onEvent(onStreamEventsCallback);
  streamClient.connect(ws_stream_url);

  camera.startStreamServer();
  sdWebServer.setup();



//  xTaskCreateStaticPinnedToCore(task1Function1, "Task1", 1024*4, NULL, 1, NULL, NULL, 1);
//   xTaskCreateStaticPinnedToCore(task1Function2, "Task2", 1024*4, NULL, 1, NULL, NULL, 1);

// start I2S at 16 kHz with 16-bits per sample
//   I2S.setAllPins(-1, 42, 41, -1, -1);
//   if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {
//     Serial.println("Failed to initialize I2S!");
//     while (1); // do nothing
//   }
    // audio.setup(SAMPLE_RATE, SAMPLE_BITS);
    // audio.recordWav("/xiao_audio_test.wav", RECORD_TIME * 2);
}

void loop() {
  client.poll();
  streamClient.poll();
  sdWebServer.loop();
// camera.starStreamHandler2(streamClient);

  // read a sample
//   int sample = I2S.read();

//   if (sample && sample != -1 && sample != 1) {
//     Serial.println(sample);
//   }
}