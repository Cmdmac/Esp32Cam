#include <Arduino.h>
#include "Camera.h"
#include <WiFi.h>
#include "command.h"
#include <I2S.h>
#include "Audio.h"
#include "SDWebServer.h"
#include "Ws.h"

const char* ssid     = "Stark";  // 替换为您的 Wi-Fi 网络名称
const char* password = "fengzhiping,1101";  // 替换为您的 Wi-Fi 密码
const char* ws_control_url = "ws://192.168.2.153:3000/mobile/camera/control?client=esp32cam"; //Enter server adress
const char* ws_stream_url = "ws://192.168.2.153:3000/mobile/camera/stream?client=esp32cam"; //Enter server adress

Audio audio;
Camera camera;
SDWebServer sdWebServer;

Ws ws;

void onCommand(int cmd) {
  switch(cmd) {
        case START_STREAM:
            camera.starStreamHandler(ws.getStreamWs());
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

  ws.setUp(ws_control_url, ws_stream_url, onCommand);
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
  ws.loop();
  sdWebServer.loop();
// camera.starStreamHandler2(streamClient);

  // read a sample
//   int sample = I2S.read();

//   if (sample && sample != -1 && sample != 1) {
//     Serial.println(sample);
//   }
}