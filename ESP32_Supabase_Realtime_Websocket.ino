#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient_Generic.h>


// Wi-Fi credentials
const char* ssid = "<WIFI_SSID>";
const char* password = "<WIFI_PASS>";

// Supabase WebSocket URL and API Key
const char* host = "<YOUR_PROJECT.supabase.co>";
const char* API_KEY = "<SUPABASE_API_KEY>";

const char* tableName = "<YOUR_TABLE_NAME>";
String subscribeJson = String("{\"type\":\"subscribe\",\"topic\":\"realtime:public:") + tableName + String("\",\"event\":\"phx_join\",\"payload\":{},\"ref\":\"1\"}");
String API_URL = String("https://") + host + String("/rest/v1/") + tableName + String("?method=eq.nowReal");


unsigned int refCounter = 1;
unsigned int lastHeartBeat = millis();
unsigned int lastPatchTest = millis();

WebSocketsClient webSocket;

void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Configure WebSocket
  String url = "/realtime/v1/websocket?apikey=" + String(API_KEY) + "&vsn=1.0.0";
  webSocket.beginSSL(host, 443, url.c_str(), "", "arduino");
  
  webSocket.onEvent(webSocketEvent);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      String url = "/realtime/v1/websocket?apikey=" + String(API_KEY) + "&vsn=1.0.0";
      webSocket.beginSSL(host, 443, url.c_str(), "", "arduino");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected");
      // Subscribe table
      webSocket.sendTXT(subscribeJson.c_str());
      break;
    case WStype_TEXT:
      Serial.printf("Message Received: %s\n", payload);
      parseMessage(payload);
      break;
    case WStype_ERROR:
      Serial.println("WebSocket Error");
      break;
    default:
      break;
  }
}

void parseMessage(uint8_t *payload) {
  JsonDocument doc;
  deserializeJson(doc, payload);
  const char* event = doc["event"];
  if(event && strcmp(event, "UPDATE") == 0) {
    const char* table = doc["payload"]["table"];
    const char* type = doc["payload"]["type"];
    JsonObject record = doc["payload"]["record"];

    /*
    // use your column name
    Serial.print("isWaterHeat:");
    Serial.println(record["isWaterHeat"] ? "true" : "false");
    */
  }
}

void sendHeartbeat() {
  if (webSocket.isConnected()) {
    String heartbeatMessage = String("{\"topic\":\"phoenix\",\"event\":\"heartbeat\",\"payload\":{},\"ref\":\"") + ++refCounter + "\"}";
    webSocket.sendTXT(heartbeatMessage);
    Serial.println("Heartbeat sent: " + heartbeatMessage);
  }
}


void sendPatchRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(API_URL);

    http.addHeader("apikey", API_KEY);
    http.addHeader("Authorization", "Bearer " + String(API_KEY));
    http.addHeader("Content-Type", "application/json");

    // use your column name
    String payload = "{\"isWaterHeat\": true}";

    int httpResponseCode = http.sendRequest("PATCH", payload);

    if (httpResponseCode > 0) {
      Serial.println("PATCH Request Sent Successfully");
    } else {
      Serial.println("Error in sending PATCH request");
      Serial.println("HTTP Response Code: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void loop() {
  webSocket.loop();

  // send heartbeat every 30 seconds
  if (millis() - lastHeartBeat > 30000) {
    lastHeartBeat = millis();
    sendHeartbeat();
  }
  
  /*
  // data patch test
  if (millis() - lastPatchTest > 45000) {
    lastPatchTest = millis();
    Serial.println("test PATCH Request");
    sendPatchRequest();
  }
  */
}
