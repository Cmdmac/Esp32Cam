#include "Audio.h"

void Audio::setup(int sampleRate, int sampleBits)
{
  this->mSampleRate = sampleRate;
  this->mSampleBits = sampleBits;
  Serial.begin(115200);
  while (!Serial)
    ;
  I2S.setAllPins(-1, 42, 41, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, mSampleRate, mSampleBits))
  {
    Serial.println("Failed to initialize I2S!");
    while (1)
      ;
  }
  if (!SD.begin(21))
  {
    Serial.println("Failed to mount SD Card!");
    while (1)
      ;
  }
  //   recordWav();
}

void Audio::recordWav(const char *path, int recordTime)
{
  uint32_t sample_size = 0;
  uint32_t record_size = (mSampleRate * mSampleBits / 8) * recordTime;
  uint8_t *rec_buffer = NULL;
  Serial.printf("Ready to start recording ...\n");

  File file = SD.open(path, FILE_WRITE);
  // Write the header to the WAV file
  uint8_t wav_header[WAV_HEADER_SIZE];
  generateWavHeader(wav_header, record_size, mSampleRate);
  file.write(wav_header, WAV_HEADER_SIZE);

  // PSRAM malloc for recording
  rec_buffer = (uint8_t *)ps_malloc(record_size);
  if (rec_buffer == NULL)
  {
    Serial.printf("malloc failed!\n");
    while (1)
      ;
  }
  Serial.printf("Buffer: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());

  // Start recording
  esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, rec_buffer, record_size, &sample_size, portMAX_DELAY);
  if (sample_size == 0)
  {
    Serial.printf("Record Failed!\n");
  }
  else
  {
    Serial.printf("Record %d bytes\n", sample_size);
  }

  // Increase volume
  for (uint32_t i = 0; i < sample_size; i += mSampleBits / 8)
  {
    (*(uint16_t *)(rec_buffer + i)) <<= VOLUME_GAIN;
  }

  // Write data to the WAV file
  Serial.printf("Writing to the file ...\n");
  if (file.write(rec_buffer, record_size) != record_size)
    Serial.printf("Write file Failed!\n");

  free(rec_buffer);
  file.close();
  Serial.printf("The recording is over.\n");
}

void Audio::generateWavHeader(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
  // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
  uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
  uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
  const uint8_t set_wav_header[] = {
      'R', 'I', 'F', 'F',                                                  // ChunkID
      file_size, file_size >> 8, file_size >> 16, file_size >> 24,         // ChunkSize
      'W', 'A', 'V', 'E',                                                  // Format
      'f', 'm', 't', ' ',                                                  // Subchunk1ID
      0x10, 0x00, 0x00, 0x00,                                              // Subchunk1Size (16 for PCM)
      0x01, 0x00,                                                          // AudioFormat (1 for PCM)
      0x01, 0x00,                                                          // NumChannels (1 channel)
      sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
      byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24,         // ByteRate
      0x02, 0x00,                                                          // BlockAlign
      0x10, 0x00,                                                          // BitsPerSample (16 bits)
      'd', 'a', 't', 'a',                                                  // Subchunk2ID
      wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24,             // Subchunk2Size
  };
  memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}