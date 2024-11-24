#ifndef _FILE_WEBSERVER_H_
#define _FILE_WEBSERVER_H_

#include <string>
#include <WebServer.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

#define FILEWEBSERVER_PORT 88
#define HOST "esp32sd"

#define SPI_CS 11
#define SPI_MOSI 12
#define SPI_CLK 13
#define SPI_MISO 14

#define FS SD

class FileWebServer {
    private:
        void loadIndexFile(const std::string& path);
    protected:
        /* data */
        WebServer server;
        std::string spiffsIndexFileStr;

        void returnOK();
        void returnFail(String msg);
        void return404();
        void handleFileUpload();
        void deleteRecursive(String path);
        void handleDelete();
        void handleCreate();
        void printDirectory();
        void handleIndex();
        void handleNotFound();
        void handleCreateFolder();
        void handleDownloadFile();
        bool loadFile(const char* path);


        std::string formatSpace(size_t bytes) {
          // Serial.println(bytes);
          char buffer[10];
          memset(buffer, 0, 10);
          long k = 1024;
          long m = 1024 * 1024;
          long g = 1024 * m;
          if (bytes >= k && bytes < m) {
            sprintf(buffer, "%.2lfK", (float)bytes / k);
          } else if (bytes >= m && bytes < g) {
            sprintf(buffer, "%.2lfM", (float)bytes / m);
          } else if (bytes >= g) {
            sprintf(buffer, "%.2lfG", (float)bytes / g);
          }
          return std::string(buffer);
        }

        size_t getFsTotalBytes() {
            return FS.totalBytes();
        }

        size_t getFsUsedBytes() {
            return FS.usedBytes();
        }

        File open(const char* path, const char* mode = FILE_READ) {
            // TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
            // Serial.println((int)currentTask);
            return FS.open(path, mode);
        }

        bool fileExists(const char* path) {
            return FS.exists(path);
        }

        bool checkAndRemove(const char* path) {
            if (FS.exists(path)) {
                return FS.remove(path);
            }
            return false;
        }

        bool remove(const char* path) {
            return FS.remove(path);
        }

        bool rmdir(const char* path) {
            return FS.rmdir(path);
        }

        bool mkdir(const char* path) {
            return FS.mkdir(path);
        }  

        static void loopDelegate(void *pvParameters) {
          FileWebServer* instance = static_cast<FileWebServer*>(pvParameters);
          while(1) {
            instance->loop();
          }
        }
      void loop();

    public:
        void setup();

};

#endif