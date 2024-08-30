#include <Arduino.h>
#include "Camera.h"
#include <WiFi.h>

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
}

void loop() {

}