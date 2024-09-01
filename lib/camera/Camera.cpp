#include "esp32-hal.h"
#include "camera.h"
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
#include "esp_camera.h"
#include "camera_pins.h"
#include <Arduino.h>


void startCameraServer();


Camera::Camera() : pSensor(NULL) {

}

void Camera::setUp() {

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;//FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    // for larger pre-allocated frame buffer.
    if(psramFound()){
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        // Limit the frame size when PSRAM is not available
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    pSensor = s;
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);        // flip it back
        s->set_brightness(s, 1);   // up the brightness just a bit
        s->set_saturation(s, -2);  // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    // if (config.pixel_format == PIXFORMAT_JPEG) {
    //     s->set_framesize(s, FRAMESIZE_QVGA);
    // }

    #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    #endif

    #if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
    #endif
}

void Camera::startStreamServer() {
    ::startCameraServer();
}

void Camera::increaseJPGQuality() {
    int quality = this->pSensor->status.quality;
    quality = (quality + 4) % 64 + 4;
    this->pSensor->set_quality(this->pSensor, quality);
}

void Camera::decreaseJPGQuality() {
    int quality = this->pSensor->status.quality;
    quality = (quality - 4) % 64 + 4;
    this->pSensor->set_quality(this->pSensor, quality);
}

void Camera::setJPGQuality(int quality) {
    if (quality > 0 && quality < 64) {
        this->pSensor->set_quality(this->pSensor, quality);
    }
}

void Camera::increaseFrameSize() {
    framesize_t frameSize = this->pSensor->status.framesize;
    frameSize = (framesize_t)(frameSize % FRAMESIZE_INVALID + 1);
    this->pSensor->set_framesize(this->pSensor, frameSize);
}

void Camera::decreaseFrameSize() {
    framesize_t frameSize = this->pSensor->status.framesize;
    frameSize = (framesize_t)(frameSize % FRAMESIZE_INVALID - 1);
    if (frameSize <= 0) {
        frameSize = (framesize_t)1;
    }
    this->pSensor->set_framesize(this->pSensor, frameSize);
}

void Camera::setFrameSize(int frameSize) {
    if (this->pSensor->pixformat == PIXFORMAT_JPEG) {
        this->pSensor->set_framesize(this->pSensor, (framesize_t)frameSize);
    }
}

// ==== Memory allocator that takes advantage of PSRAM if present =======================
char* Camera::allocateMemory(char* aPtr, size_t aSize) {

  //  Since current buffer is too smal, free it
  if (aPtr != NULL) free(aPtr);


  size_t freeHeap = ESP.getFreeHeap();
  char* ptr = NULL;

  // If memory requested is more than 2/3 of the currently free heap, try PSRAM immediately
  if ( aSize > freeHeap * 2 / 3 ) {
    if ( psramFound() && ESP.getFreePsram() > aSize ) {
      ptr = (char*) ps_malloc(aSize);
    }
  }
  else {
    //  Enough free heap - let's try allocating fast RAM as a buffer
    ptr = (char*) malloc(aSize);

    //  If allocation on the heap failed, let's give PSRAM one more chance:
    if ( ptr == NULL && psramFound() && ESP.getFreePsram() > aSize) {
      ptr = (char*) ps_malloc(aSize);
    }
  }
}

// ==== RTOS task to grab frames from the camera =========================
void Camera::starStreamHandler(WebsocketsClient ws) {

  //  Pointers to the 2 frames, their respective sizes and index of the current frame
  char* fbs[2] = { NULL, NULL };
  size_t fSize[2] = { 0, 0 };
  int ifb = 0;
  camera_fb_t* fb = NULL;
  for (;;) {

    //  Grab a frame from the camera and query its size
    if (fb)
        esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();
    if (fb == NULL) {
        break;
    }
    size_t s = fb->len;

    //  If frame size is more that we have previously allocated - request  125% of the current frame space
    if (s > fSize[ifb]) {
      fSize[ifb] = s * 4 / 3;
      fbs[ifb] = allocateMemory(fbs[ifb], fSize[ifb]);
    }

    //  Copy current frame into local buffer
    char* b = (char*) fb->buf;
    memcpy(fbs[ifb], b, s);

    //  Do not allow interrupts while switching the current frame
    // portENTER_CRITICAL(&xSemaphore);
    char* bufForWs = fbs[ifb];
    int bufFowWsLen = s;
    // send buffer
    ws.sendBinary(bufForWs, bufFowWsLen);
    ifb++;
    ifb &= 1;  // this should produce 1, 0, 1, 0, 1 ... sequence
    // portEXIT_CRITICAL(&xSemaphore);

  }
}

void Camera::stopStreamHandler() {
    
}