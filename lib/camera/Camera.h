#ifndef _CAMERA_H

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

    private:
        sensor_t *pSensor;

};
#endif