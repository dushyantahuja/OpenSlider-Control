#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>
#include <FastLED.h>
#include <ESPAsyncWiFiManager.h>
//#include <ESP8266mDNS.h>
#include <SPIFFSEditor.h>


AsyncWebServer httpServer(80);
DNSServer dns;

//#define DEBUG

#include <NTPClient.h>

// NTP Servers:

WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 360000); //19800

#include <ArduinoJson.h>

WiFiClient client;
HTTPClient http;

#include "Page_Admin.h"
#include "config.h"


void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial.begin(250000);
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    //delay(10000);
    Serial.println("G92 X0 Y0 Z0");         // Set Zero at start
    Serial.println("G91");                  // Set Relative Positioning
    DEBUG_PRINT("Wifi Setup Initiated");
    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    AsyncWiFiManager wifiManager(&httpServer,&dns);
    wifiManager.setTimeout(180);
    if(!wifiManager.autoConnect(DEVICE_NAME)) {
      delay(3000);
      ESP.reset();
      delay(5000);
      }
    DEBUG_PRINT("Wifi Setup Completed");
    //MDNS.begin(DEVICE_NAME);
    //MDNS.addService("http", "tcp", 80);

    // Admin page
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send_P(200,"text/html", PAGE_AdminMainPage ); 
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz","text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/control.html", HTTP_GET, send_control_html);
    httpServer.on("/command", HTTP_POST, controlSlider);
    httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
    httpServer.on("/doUpdate", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
    );
    httpServer.addHandler(new SPIFFSEditor("admin","admin"));
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    //MDNS.addService("http", "tcp", 80);
    //timeClient.begin();
    //timeClient.update();
    wdt_enable(WDTO_2S);
}

uint8_t step = 0;

void loop() {
  //timeClient.update();
  //MDNS.update();
  EVERY_N_SECONDS(1){
    if(config.timelapseOn) timelapseControl();
  }
}


void handleNotFound(AsyncWebServerRequest *request){
  //message+= "Time: ";
  //message+= String(timeClient.getHours()) + ":" + String(timeClient.getMinutes())+ ":" + String(timeClient.getSeconds()) + "\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i=0; i<request->params(); i++){
    AsyncWebParameter* p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
  request->send(404, "text/plain", message);
  Serial.println(message);
  message = "";
}

void send_control_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0 ){
    AsyncWebParameter* p = request->getParam("XEnd");
    int XEnd = p->value().toInt();
    p = request->getParam("YEnd");
    int YEnd = p->value().toInt();
    p = request->getParam("ZEnd");
    int ZEnd = p->value().toInt();
    p = request->getParam("Min");
    config.time = p->value().toInt()*60;        //Convert to Seconds
    p = request->getParam("Sec");
    config.time += p->value().toInt();
    config.xMove = ((double)XEnd / config.time);
    config.yMove = ((double)YEnd / config.time);
    config.zMove = ((double)ZEnd / config.time);
    config.timelapseOn = true;
    config.timelapsecount = 0;
    Serial.println("G92 X0Y0Z0");
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/control.html", "text/html");
  //response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void controlSlider(AsyncWebServerRequest *request){
  AsyncWebParameter* p = request->getParam(0);
  Serial.println("G91");
  Serial.println( p->value());
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "");
  request->send(response);
  //message+= p->value();
}

void timelapseControl(){
    if(++config.timelapsecount > config.time -1) config.timelapseOn = false;
    String command = "G1 X" + String(config.xMove) + " Y" + String(config.yMove) + " Z" + String(config.zMove);
    Serial.println(command);
}