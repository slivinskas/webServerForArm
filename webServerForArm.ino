#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Servo.h>

Servo myservo;
int servosPinsQuantity = 4;
int servoPin[] =  {15, 13, 14, 12};
StaticJsonDocument<256> doc;

ESP8266WiFiMulti WiFiMulti;
#define SSIDName "nood"
#define PASS "naujas16"

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

void setup() {
  USE_SERIAL.begin(115200);
  //USE_SERIAL.setDebugOutput(true);

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP(SSIDName, PASS);
  //  USE_SERIAL.println("Jungiamasi prie: "+ SSIDName);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(2000);
    USE_SERIAL.print(".");
  }

  webSocket.onEvent(webSocketEvent);
  webSocket.begin();
  if (MDNS.begin("esp8266")) {
    USE_SERIAL.println("MDNS responder started");
  }

  SPIFFS.begin();
  server.begin();
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });
  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  myservo.attach(servoPin[0]);
  USE_SERIAL.println("IP: " + WiFi.localIP());
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  USE_SERIAL.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  USE_SERIAL.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void moveHand(int n, int servoNo) {
  myservo.detach();
  myservo.attach(servoNo);  //the pin for the servo control
  if (n >= 500) {
    USE_SERIAL.printf("writing Microseconds: %d\n", n);
    myservo.writeMicroseconds(n);
  } else {
    USE_SERIAL.printf(" writing Angle: %d\n", n);
    myservo.write(n);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String outputToClient;
  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[%u] Atsijungta!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Prisijungta from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Connected");
        Serial.println("Klientui: ");
        for (int j = 0; j < servosPinsQuantity; ++j) {
          doc["servo"] = j;
          myservo.detach();
          myservo.attach(servoPin[j]);
          doc["position"] = myservo.read();
          serializeJson(doc, outputToClient);
          USE_SERIAL.println(outputToClient);
          webSocket.sendTXT(num, outputToClient);
          outputToClient= "";
        }
      }
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[%u] Gauta: %s\n", num, payload);
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        USE_SERIAL.println("Nepavyko isparsinti JSON");
        USE_SERIAL.println(error.c_str());
      } else {
        
        if (doc["position"] > 180) {
          doc["position"] = 180;
        }
        if (doc["position"] < 0) {
          doc["position"] = 0;
        }
        int servoNo = doc["servo"];
        if (servoNo >= servosPinsQuantity || servoNo < 0) {
          servoNo = 0;
        }
        USE_SERIAL.printf("Duomenys %d ir %d \n",servoPin[servoNo],(int)doc["position"]);
        moveHand(doc["position"], servoPin[servoNo]);
      }
      break;
  }
}

void loop() {
  webSocket.loop();
  server.handleClient();
}
