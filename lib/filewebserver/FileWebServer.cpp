#include "FileWebServer.h"

#include <SPIFFS.h>
#include <FS.h>
#include "esp_spiffs.h"
#include <sstream>
#include <SD.h>

void FileWebServer::returnOK() {
  server.send(200, "text/plain", "");
}

void FileWebServer::returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

void FileWebServer::return404() {
  String message = "Error \n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.print(message);
}

void FileWebServer::handleFileUpload() {

  String dir = server.arg(0);
  HTTPUpload &upload = server.upload();
  
  File uploadFile;
  String path = dir + upload.filename;
  // Serial.println(path);
  if (upload.status == UPLOAD_FILE_START) {
    checkAndRemove(path.c_str());
    // if (SPIFFS.exists(path)) {
    //   SPIFFS.remove(path);
    // }
    uploadFile = open(path.c_str(), FILE_WRITE);
    // SPIFFS.open(path, FILE_WRITE);
    Serial.print("Upload: START, filename: ");
    Serial.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    Serial.print("Upload: WRITE, Bytes: ");
    Serial.println(upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.print("Upload: END, Size: ");
    Serial.println(upload.totalSize);
  }
  // returnOK();
}

void FileWebServer::deleteRecursive(String path) {
  File file = open((char*)path.c_str());//SPIFFS.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    remove(path.c_str());
    // SPIFFS.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      remove(entryPath.c_str());
    //   SPIFFS.remove((char *)entryPath.c_str());
    }
    yield();
  }

    rmdir(path.c_str());
//   SPIFFS.rmdir((char *)path.c_str());
  file.close();
}

void FileWebServer::handleDelete() {
  if (!server.hasArg("path")) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg("path");
  if (path == "/" || !fileExists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void FileWebServer::handleCreate() {
  if (server.args() == 0) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg("path");
  if (path == "/" || fileExists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    File file = open((char *)path.c_str(), FILE_WRITE);
    //SPIFFS.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      file.write(0);
      file.close();
    }
  } else {
    SPIFFS.mkdir((char *)path.c_str());
  }
  returnOK();
}

void FileWebServer::printDirectory() {
    Serial.println("printDirectory");
  if (!server.hasArg("dir")) {
    return returnFail("BAD ARGS");
  }

  String path = server.arg(0);
  Serial.println("list path=" + path);
  if (path != "/" && !fileExists((char *)path.c_str())) {
    return returnFail("BAD PATH");
  }

  File dir = SD.open(path.c_str());//open(path.c_str());
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }

  dir.rewindDirectory();

  std::string str;
  str += "[";
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    std::string output;
    if (cnt > 0) {
      output = ',';
    }

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    str += output;
    entry.close();
  }
  str += "]";
  server.setContentLength(str.length());
  server.send(200, "text/json", str.c_str());
  dir.close();
}

void FileWebServer::loadIndexFile(const std::string& path) {
  size_t totalBytes = getFsTotalBytes();
  size_t usedBytes = getFsUsedBytes();
  Serial.print("总大小: ");
  Serial.print(totalBytes);
  Serial.print("字节 ");
  Serial.print("已用：");
  Serial.println(usedBytes);

  File file = SPIFFS.open(path.c_str(), "r");
  if (!file) {
      Serial.println("open spiffs file index.html failure");
      return;
  }

  std::string& content = this->spiffsIndexFileStr;
  uint8_t buffer[128] = {0};
  while(file.available()) {
      int count = file.read(buffer, 128);
      std::string s = std::string((char*)buffer, count);
      content.append(s);
      memset(buffer, 0, 128);
  }
  int index = content.find("{{%total%}}");
  Serial.println(index);

  if (index != std::string::npos) {
    std::string str = formatSpace(totalBytes);
    content = content.replace(index, strlen("{{%total%}}"), str);   
  } else {
    Serial.println(index);
  }

  index = content.find("{{%used%}}");
  if (index != std::string::npos) {
    std::string str = formatSpace(usedBytes);
    content = content.replace(index, strlen("{{%used%}}"), str);
  } else {
    Serial.println("not found string");
  }
  // return content;
}

void FileWebServer::handleIndex() {
    // Serial.println(spiffsIndexFileStr.c_str());
    // String content = String(spiffsIndexFileStr.c_str());
    // Serial.println(spiffsIndexFileStr.c_str());
    // server.send(200, "text/html; charset=utf-8", content);
    // loadIndexFile("/index.htm");
    // server.sendContent(content);
    
    server.send(200, "text/html; charset=utf-8", this->spiffsIndexFileStr.c_str());
}

void FileWebServer::handleNotFound() {
  if (loadFile(server.uri().c_str())) {
    return;
  }
  return404();
}

void FileWebServer::handleCreateFolder() {
  if (!server.hasArg("dir")) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg("dir");
//   if (SPIFFS.exists(path)) {
//     server.send(500, "text/json", "{\"code\":0, \"msg\":\"folder exist!\"}");
//     return;
//   }
  if (fileExists(path.c_str())) {
    server.send(500, "text/json", "{\"code\":0, \"msg\":\"folder exist!\"}");
    return;
  }
  if (mkdir(path.c_str())) {
    server.send(200, "text/json", "{\"code\":1}");
  } else {
    server.send(500, "text/json", "{\"code\":0}");
  }
}

void FileWebServer::handleDownloadFile() {
  if (!server.hasArg("path")) {
    return returnFail("BAD ARGS");
  }
    
  String path = server.arg(0);

  File dataFile = open(path.c_str());
  if (dataFile.isDirectory()) {
    return404();
  }

  if (!dataFile) {
      return404();
  }

  std::stringstream ss;
  
  ss << "attachment;filename=" << dataFile.name();

  server.sendHeader("Content-Disposition", ss.str().c_str());
  if (server.streamFile(dataFile, "application/octet-stream") != dataFile.size()) {
      Serial.println("Sent less data than expected!");
      returnFail("Sent less data than expected!");
  }

  dataFile.close();
}

bool FileWebServer::loadFile(const char* path) {
    String p = path;
    String dataType = "text/plain";
    if (p.endsWith("/")) {
        p += "index.htm";
    }

    if (p.endsWith(".src")) {
        p = p.substring(0, p.lastIndexOf("."));
    } else if (p.endsWith(".htm")) {
        dataType = "text/html";
    } else if (p.endsWith(".css")) {
        dataType = "text/css";
    } else if (p.endsWith(".js")) {
        dataType = "application/javascript";
    } else if (p.endsWith(".png")) {
        dataType = "image/png";
    } else if (p.endsWith(".gif")) {
        dataType = "image/gif";
    } else if (p.endsWith(".jpg")) {
        dataType = "image/jpeg";
    } else if (p.endsWith(".ico")) {
        dataType = "image/x-icon";
    } else if (p.endsWith(".xml")) {
        dataType = "text/xml";
    } else if (p.endsWith(".pdf")) {
        dataType = "application/pdf";
    } else if (p.endsWith(".zip")) {
        dataType = "application/zip";
    }

    if (!fileExists(p.c_str())) {
      return false;
    }

    File dataFile = open(p.c_str());
    if (dataFile.isDirectory()) {
        p += "/index.htm";
        dataType = "text/html";
        dataFile = open(p.c_str());
    }

    if (!dataFile) {
        return false;
    }

    if (server.hasArg("download")) {
        dataType = "application/octet-stream";
    }

    if (server.streamFile(dataFile, dataType) != dataFile.size()) {
        Serial.println("Sent less data than expected!");
    }

    dataFile.close();
    return true;
}

// void test() {
//   File file = SD.open("/", "r");
//     if (file.isDirectory()) {
//       Serial.println("is dir");
//     }
// }

void FileWebServer::setup(void) {

  if (MDNS.begin(HOST)) {
    MDNS.addService("http", "tcp", FILEWEBSERVER_PORT);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to sdwebserver http://");
    Serial.print(HOST);
    Serial.print(".local:");
    Serial.print(FILEWEBSERVER_PORT);
    Serial.println();
  }
  
  server.on("/", HTTP_GET, [&]() { handleIndex(); });
  server.on("/list", HTTP_GET,[&]() { printDirectory(); });
  // server.on("/list", HTTP_GET, test);
  server.on("/delete", HTTP_POST, [&]() { handleDelete(); });
  server.on("/edit", HTTP_PUT, [&]() { handleCreate(); });
  server.on("/upload", HTTP_POST, [&]() { returnOK(); }, [&]() { handleFileUpload(); });
  server.on("/createFolder", HTTP_POST, [&]() { handleCreateFolder(); });
  server.on("/download", HTTP_GET, [&]() { handleDownloadFile(); });
  server.onNotFound([&]() { handleNotFound(); });

  server.begin(FILEWEBSERVER_PORT);
  Serial.println("FileWebServer started");

  #ifdef FS
    // #ifdef SD
      if (SD.begin(21)) {
        Serial.println("SD Card initialized.");
        // hasSD = true;
      }
    // #endif
  #endif

  if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS 初始化失败");
      return;
  }

    loadIndexFile("/index.htm");

    // Serial.println(spiffsIndexFileStr.c_str());
    xTaskCreatePinnedToCore(&FileWebServer::loopDelegate, "FileWebServerTask", 4096, this, 1, NULL, 0);
}

void FileWebServer::loop(void) {
  server.handleClient();
  delay(1);  //allow the cpu to switch to other tasks
}