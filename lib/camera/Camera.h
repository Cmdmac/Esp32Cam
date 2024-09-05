#ifndef _CAMERA_H
#include <sensor.h>
#include <ArduinoWebsockets.h>
using namespace websockets;
class Camera {
    public:
        Camera();
        void setUp();
        void startStreamServer();
        void startStreamServer2();
        void increaseJPGQuality();
        void decreaseJPGQuality();
        void setJPGQuality(int quality);
        void increaseFrameSize();
        void decreaseFrameSize();
        void setFrameSize(int);
        void starStreamHandler(WebsocketsClient ws);
        void stopStreamHandler();
        void starStreamHandler2(WebsocketsClient client);
        void sendCache(WebsocketsClient client);

        static char* allocateMemory(char* aPtr, size_t aSize);
    private:
        
        sensor_t *pSensor;

};
#endif