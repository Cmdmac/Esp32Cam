#include "esp32-hal.h"
#include "camera.h"
// #define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
#define CAMERA_MODEL_XIAO_ESP32S3
#include "esp_camera.h"
#include "camera_pins.h"
#include <Arduino.h>
#include <freertos/portmacro.h>
#include <WebServer.h>
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
    config.frame_size = FRAMESIZE_VGA;//FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    // for larger pre-allocated frame buffer.
    if(psramFound()){
      Serial.println("psramFound");
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      Serial.println("frame_size = FRAMESIZE_HVGA");
        // Limit the frame size when PSRAM is not available
        config.frame_size = FRAMESIZE_VGA;
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
  return aPtr;
}


//cache
int memcache_size = 4096;
int data_size = 0;
char* fbCache = NULL;
portMUX_TYPE xSemaphore;
bool is_sending = false;
void Camera::starStreamHandler2(WebsocketsClient client) {
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];
    static int64_t last_frame = 0;


    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    while (true)
    {
        last_frame = esp_timer_get_time();
        fb = esp_camera_fb_get();
        if (!fb)
        {
            res = ESP_FAIL;
        }
        else
        {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
            if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
                }
        if (res == ESP_OK)
        {
            portENTER_CRITICAL(&xSemaphore);
            if (!is_sending) {
                size_t s = fb->len;
                // res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
                if (s > memcache_size) {
                    memcache_size = s * 4 / 3;
                    Serial.println("allocateMemory");
                    fbCache = allocateMemory(fbCache, memcache_size);
                }

                //  Copy current frame into local buffer
                char* b = (char*) fb->buf;
                memcpy(fbCache, b, s);
                data_size = s;
                
            }
            portEXIT_CRITICAL(&xSemaphore);
            // client.sendBinary((const char*)fb->buf, fb->len);

        }

        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            break;
        }
        int64_t fr_end = esp_timer_get_time();
         int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        Serial.print("MJPG:");Serial.print((uint32_t)(_jpg_buf_len));Serial.print("-");
        Serial.print((uint32_t)frame_time);Serial.print("ms");Serial.print("-");
        Serial.print(1000.0 / (uint32_t)frame_time);Serial.print("fps");
        Serial.println();
        // Serial.print("avg");Serial.print(avg_frame_time);
        // Serial.print("avgfps");Serial.print(1000.0 / avg_frame_time);
    }
}

void Camera::sendCache(WebsocketsClient client) {
    while(true) {
        portENTER_CRITICAL(&xSemaphore);
        is_sending = true;
        if (fbCache != NULL) {
            client.sendBinary((const char*)fbCache, data_size);
        }
        is_sending = false;
        portEXIT_CRITICAL(&xSemaphore);
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
    // ws.sendBinary(bufForWs, bufFowWsLen);
    ifb++;
    ifb &= 1;  // this should produce 1, 0, 1, 0, 1 ... sequence
    // portEXIT_CRITICAL(&xSemaphore);

  }
}

void Camera::stopStreamHandler() {
    
}
#define APP_CPU 1
#define PRO_CPU 0
// ===== rtos task handles =========================
// Streaming is implemented with 3 tasks:
TaskHandle_t handle_Mjpeg;   // handles client connections to the webserver
TaskHandle_t handle_Cam;     // handles getting picture frames from the camera and storing them locally
TaskHandle_t handle_Stream;  // actually streaming frames to all connected clients
// frameSync semaphore is used to prevent streaming buffer as it is replaced with the next frame
SemaphoreHandle_t frameSync = NULL;
// Queue stores currently connected clients to whom we are streaming
QueueHandle_t streamingClients;
// We will try to achieve 25 FPS frame rate
const int FPS = 1000;
// We will handle web client requests every 50 ms (20 Hz)
const int WSINTERVAL = 100;
// Commonly used variables:
volatile size_t camSize;    // size of the current frame, byte
volatile char* camBuf;      // pointer to the current frame

// ==== RTOS task to grab frames from the camera =========================
void camCB(void* pvParameters) {

  TickType_t xLastWakeTime;

  //  A running interval associated with currently desired frame rate
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / FPS);

  // Mutex for the critical section of swithing the active frames around
  portMUX_TYPE xSemaphore = portMUX_INITIALIZER_UNLOCKED;

  //  Pointers to the 2 frames, their respective sizes and index of the current frame
  char* fbs[2] = { NULL, NULL };
  size_t fSize[2] = { 0, 0 };
  int ifb = 0;

  //=== loop() section  ===================
  xLastWakeTime = xTaskGetTickCount();

    camera_fb_t *fb = NULL;

  for (;;) {

    //  Grab a frame from the camera and query its size
    // cam.run();
    if (fb)
        //return the frame buffer back to the driver for reuse
        esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();

    size_t s = fb->len;

    //  If frame size is more that we have previously allocated - request  125% of the current frame space
    if (s > fSize[ifb]) {
      fSize[ifb] = s * 4 / 3;
      Serial.println("allocateMemory");
      fbs[ifb] = (char*) malloc(fSize[ifb]);;//Camera::allocateMemory(fbs[ifb], fSize[ifb]);
    }

    //  Copy current frame into local buffer
    char* b = (char*) fb->buf;
    memcpy(fbs[ifb], b, s);

    //  Let other tasks run and wait until the end of the current frame rate interval (if any time left)
    taskYIELD();
    // vTaskDelayUntil(&xLastWakeTime, xFrequency);

    //  Only switch frames around if no frame is currently being streamed to a client
    //  Wait on a semaphore until client operation completes
    xSemaphoreTake( frameSync, portMAX_DELAY );

    //  Do not allow interrupts while switching the current frame
    portENTER_CRITICAL(&xSemaphore);
    camBuf = fbs[ifb];
    camSize = s;
    ifb++;
    ifb &= 1;  // this should produce 1, 0, 1, 0, 1 ... sequence
    portEXIT_CRITICAL(&xSemaphore);

    //  Let anyone waiting for a frame know that the frame is ready
    xSemaphoreGive( frameSync );

    //  Technically only needed once: let the streaming task know that we have at least one frame
    //  and it could start sending frames to the clients, if any
    xTaskNotifyGive( handle_Stream );

    //  Immediately let other (streaming) tasks run
    taskYIELD();

    //  If streaming task has suspended itself (no active clients to stream to)
    //  there is no need to grab frames from the camera. We can save some juice
    //  by suspedning the tasks
    if ( eTaskGetState( handle_Stream ) == eSuspended ) {
      vTaskSuspend(NULL);  // passing NULL means "suspend yourself"
    }
  }
}


// ==== STREAMING ======================================================
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

WebServer server(80);

// ==== Handle connection request from clients ===============================
void handleJPGSstream(void)
{
  //  Can only acommodate 10 clients. The limit is a default for WiFi connections
  if ( !uxQueueSpacesAvailable(streamingClients) ) return;


  //  Create a new WiFi Client object to keep track of this one
  WiFiClient* client = new WiFiClient();
  *client = server.client();

  //  Immediately send this client a header
  client->write(HEADER, hdrLen);
  client->write(BOUNDARY, bdrLen);

  // Push the client to the streaming queue
  xQueueSend(streamingClients, (void *) &client, 0);

  // Wake up streaming tasks, if they were previously suspended:
  if ( eTaskGetState( handle_Cam ) == eSuspended ) vTaskResume( handle_Cam );
  if ( eTaskGetState( handle_Stream ) == eSuspended ) vTaskResume( handle_Stream );
}


// ==== Actually stream content to all connected clients ========================
void streamCB(void * pvParameters) {
  char buf[16];
  TickType_t xLastWakeTime;
  TickType_t xFrequency;

  //  Wait until the first frame is captured and there is something to send
  //  to clients
  ulTaskNotifyTake( pdTRUE,          /* Clear the notification value before exiting. */
                    portMAX_DELAY ); /* Block indefinitely. */

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    // Default assumption we are running according to the FPS
    xFrequency = pdMS_TO_TICKS(1000 / FPS);

    //  Only bother to send anything if there is someone watching
    UBaseType_t activeClients = uxQueueMessagesWaiting(streamingClients);
    if ( activeClients ) {
      // Adjust the period to the number of connected clients
      xFrequency /= activeClients;

      //  Since we are sending the same frame to everyone,
      //  pop a client from the the front of the queue
      WiFiClient *client;
      xQueueReceive (streamingClients, (void*) &client, 0);

      //  Check if this client is still connected.

      if (!client->connected()) {
        //  delete this client reference if s/he has disconnected
        //  and don't put it back on the queue anymore. Bye!
        delete client;
      }
      else {

        //  Ok. This is an actively connected client.
        //  Let's grab a semaphore to prevent frame changes while we
        //  are serving this frame
        xSemaphoreTake( frameSync, portMAX_DELAY );

        client->write(CTNTTYPE, cntLen);
        sprintf(buf, "%d\r\n\r\n", camSize);
        client->write(buf, strlen(buf));
        client->write((char*) camBuf, (size_t)camSize);
        client->write(BOUNDARY, bdrLen);

        // Since this client is still connected, push it to the end
        // of the queue for further processing
        xQueueSend(streamingClients, (void *) &client, 0);

        //  The frame has been served. Release the semaphore and let other tasks run.
        //  If there is a frame switch ready, it will happen now in between frames
        xSemaphoreGive( frameSync );
        taskYIELD();
      }
    }
    else {
      //  Since there are no connected clients, there is no reason to waste battery running
      vTaskSuspend(NULL);
    }
    //  Let other tasks run after serving every client
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}


void handleNotFound()
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

// ======== Server Connection Handler Task ==========================
void mjpegCB(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(WSINTERVAL);

  // Creating frame synchronization semaphore and initializing it
  frameSync = xSemaphoreCreateBinary();
  xSemaphoreGive( frameSync );

  // Creating a queue to track all connected clients
  streamingClients = xQueueCreate( 10, sizeof(WiFiClient*) );

  //=== setup section  ==================

  //  Creating RTOS task for grabbing frames from the camera
  xTaskCreatePinnedToCore(
    camCB,        // callback
    "cam",        // name
    4096,         // stacj size
    NULL,         // parameters
    2,            // priority
    &handle_Cam,        // RTOS task handle
    APP_CPU);     // core

  //  Creating task to push the stream to all connected clients
  xTaskCreatePinnedToCore(
    streamCB,
    "strmCB",
    4 * 1024,
    NULL, //(void*) handler,
    2,
    &handle_Stream,
    APP_CPU);

    Serial.println("start web server");
    //  Registering webserver handling routines
    server.on("/mjpeg/1", HTTP_GET, handleJPGSstream);
    //   server.on("/jpg", HTTP_GET, handleJPG);
    server.onNotFound(handleNotFound);

    //  Starting webserver
    server.begin();

  //=== loop() section  ===================
  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    server.handleClient();

    //  After every server client handling request, we let other tasks run and then pause
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void Camera::startStreamServer2() {
    // WebServer server(80);
    //  Registering webserver handling routines
//   server.on("/mjpeg/1", HTTP_GET, handleJPGSstream);
//   server.on("/jpg", HTTP_GET, handleJPG);
//   server.onNotFound(handleNotFound);

//    Starting webserver
//   server.begin();
    xTaskCreatePinnedToCore(
        mjpegCB,
        "mjpeg",
        4 * 1024,
        NULL,
        2,
        &handle_Mjpeg,
        APP_CPU);
}