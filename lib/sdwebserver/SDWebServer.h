#ifndef _SDWEBSERVER_H_
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <SD.h>

#define SDWEBSERVER_PORT 81

class SDWebServer
{
private:
    /* data */
    WebServer server;
    bool hasSD = false;
    File uploadFile;

    void returnOK();
    void returnFail(String msg);
    bool loadFromSdCard(String path);
    void handleFileUpload();
    void deleteRecursive(String path);
    void handleDelete();
    void handleCreate();
    void printDirectory();
    void handleNotFound();

public:
    // SDWebServer(/* args */);
    // ~SDWebServer();

    void setup();
    void loop();
};

// SDWebServer::SDWebServer(/* args */)
// {
// }

// SDWebServer::~SDWebServer()
// {
// }

#endif