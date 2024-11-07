#ifndef _WS_H_
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
using namespace websockets;

typedef std::function<void(int)> CommandCallback;

class Ws {

    public:
        void setUp(String ws_control_url, String ws_stream_url, CommandCallback commandCallback);
        void loop();
        WebsocketsClient& getControlWs();
        WebsocketsClient& getStreamWs();

    private:
        WebsocketsClient client;
        WebsocketsClient streamClient;
        void onControlMessageCallback(WebsocketsMessage message);
        void onControlEventsCallback(WebsocketsEvent event, String data);
        void onStreamMessageCallback(WebsocketsMessage message);
        void onStreamEventsCallback(WebsocketsEvent event, String data);

        CommandCallback commandCallback;
    
};
#endif