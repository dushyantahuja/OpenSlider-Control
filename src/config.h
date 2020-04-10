#define DEVICE_NAME "OpenSliderControl"
#define GET_VARIABLE_NAME(Variable) (#Variable).cstr()

#ifndef DEBUG_PRINT
  #ifdef DEBUG
    #define DEBUG_PRINT(x)  Serial.println(x)
  #else
    #define DEBUG_PRINT(x)
  #endif
#endif

// Function Definitions

void handleNotFound(AsyncWebServerRequest *request);
void handleUpdate(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void send_control_html(AsyncWebServerRequest *request);
void controlSlider(AsyncWebServerRequest *request);


String message = "";

struct strConfig {
    int current_x;
    int current_y;
    int current_z;
    //int latitude;
    //int longitude;
}   config;

// Code from https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino

void handleUpdate(AsyncWebServerRequest *request) {
  const char* html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index){
    DEBUG_PRINT("Update");
    size_t content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_FS : U_FLASH;
#ifdef ESP8266
    Update.runAsync(true);
    if (!Update.begin(content_len, cmd)) {
#else
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    //Update.printError(Serial);
#ifdef ESP8266
  } else {
    //Serial.printf("Progress: %d%%\n", (Update.progress()*100)/Update.size());
#endif
  }

  if (final) {
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      DEBUG_PRINT("Update complete");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots");
      response->addHeader("Refresh", "15; url=/"); 
      //response->addHeader("Location", "/"); 
      response->addHeader("Connection", "close");
      request->send(response);
      Serial.flush();
      //delay(200);
      ESP.restart();
    }
  }
}
