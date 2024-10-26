#ifndef _AUDIO_H_
#include <I2S.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// make changes as needed
#define RECORD_TIME   20  // seconds, The maximum value is 240
#define WAV_FILE_NAME "arduino_rec"

// do not change for best
#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define WAV_HEADER_SIZE 44
#define VOLUME_GAIN 4

class Audio {
    public:
        void setup(int sampleRate, int sampleBits);
        void recordWav(const char* path, int recordTime);

    private:
        void generateWavHeader(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate);
        int mSampleRate;
        int mSampleBits;
};
#endif