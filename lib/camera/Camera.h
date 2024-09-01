#ifndef _CAMERA_H
#include <sensor.h>
#include <ArduinoWebsockets.h>
using namespace websockets;
class Camera {
    public:
        Camera();
        void setUp();
        void startStreamServer();
        void increaseJPGQuality();
        void decreaseJPGQuality();
        void setJPGQuality(int quality);
        void increaseFrameSize();
        void decreaseFrameSize();
        void setFrameSize(int);
        void starStreamHandler(WebsocketsClient ws);
        void stopStreamHandler();

    private:
        char* allocateMemory(char* aPtr, size_t aSize);
        
        sensor_t *pSensor;

};
#endif