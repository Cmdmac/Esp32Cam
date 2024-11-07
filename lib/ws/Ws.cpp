#include "Ws.h"

void Ws::setUp(String ws_control_url, String ws_stream_url, CommandCallback commandCallback) {
    this->commandCallback = commandCallback;
    client.onMessage([&](WebsocketsMessage message) { onControlMessageCallback(message); });
    client.onEvent([&](WebsocketsEvent event, String data) { onControlEventsCallback(event, data); });
    client.connect(ws_control_url);

    streamClient.onMessage([&](WebsocketsMessage message) { onStreamMessageCallback(message); });
    streamClient.onEvent([&](WebsocketsEvent event, String data) { onStreamEventsCallback(event, data); });
    streamClient.connect(ws_stream_url);

}

WebsocketsClient& Ws::getControlWs() {
    return client;
}

WebsocketsClient& Ws::getStreamWs() {
    return streamClient;
}

void Ws::onControlMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
    JsonDocument doc;
    deserializeJson(doc, message.data());
    int cmd = doc["command"];
    if (commandCallback != NULL) {
        commandCallback(cmd);
    }
    // switch(cmd) {
    //     case START_STREAM:
    //         camera.starStreamHandler(streamClient);
    //         break;
    //     case STOP_STREAM:
    //     break;
    //     case INCREASE_JPG_QUALITY:                                 
    //         camera.increaseFrameSize();
    //     break;
    //     case DECREASE_JPG_QUALITY:
    //         camera.decreaseFrameSize();
    //     break;
    //     case INCREASE_FRAME_SIZE:
    //     break;
    //     case DECREASE_FRAME_SIZE:
    //     break;
    //     default:
    //     break;
    // }
}

void Ws::loop() {
    client.poll();
    streamClient.poll();
}

void Ws::onControlEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Control Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Control Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Control Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Control Got a Pong!");
    }
}

void Ws::onStreamMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void Ws::onStreamEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Stream Connnection Opened");
        // WSInterfaceString s = "123456dads";
        // streamClient.sendBinary(s);
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Stream Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Stream Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Stream Got a Pong!");
    }
}