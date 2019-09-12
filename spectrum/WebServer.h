/*
   Copyright (C) 2019 SFini

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
  * @file WebServer.h
  *
  * Webserver Interface to communicate via esp8266 in station or access-point mode. 
  * Works with the SPIFFS (.html, .css, .js).
  */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

/* Put your SSID & Password */
const char* ssid = "Spectrum";  // Enter SSID here
const char* password = "";      // Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

/**
  * MyWebServer helper class for making an access point and web-server.
  */
class MyWebServer
{
protected:
   static std::mutex     mutex_;  //!< Mutex for multithreading synchronisations.
   static AsyncWebServer server;  //!< Webserver with asynchron call functionality.
   static MyOptions     *options; //!< Reference to the global options.
   
protected:
   static void handleOptions       (AsyncWebServerRequest *request);
   static void handleOnOff         (AsyncWebServerRequest *request);
   static void handleModus         (AsyncWebServerRequest *request);
   static void handleSetSensitivity(AsyncWebServerRequest *request);
   static void handleSetLevel      (AsyncWebServerRequest *request);
   static void handleSetBrightness (AsyncWebServerRequest *request);
   static void handleData          (AsyncWebServerRequest *request);

public:
   MyWebServer(MyOptions &options);
   ~MyWebServer();

   void begin();
   void handleClient();
};

std::mutex     MyWebServer::mutex_;
AsyncWebServer MyWebServer::server(80);
MyOptions     *MyWebServer::options = NULL;

/** Constructor/Destructor */
MyWebServer::MyWebServer(MyOptions &opt)
{
   options = &opt;
}

MyWebServer::~MyWebServer()
{
   options = NULL;
}

/** Starts the web server with all the callback functions. */
void MyWebServer::begin()
{
   WiFi.softAP(ssid, password);
   WiFi.softAPConfig(local_ip, gateway, subnet);
   delay(1000);

   server.serveStatic("/", SPIFFS, "/").setDefaultFile("Main.html");

   server.on("/Options.xml",    [] (AsyncWebServerRequest *request) { handleOptions(request);        });
   server.on("/OnOff",          [] (AsyncWebServerRequest *request) { handleOnOff(request);          });
   server.on("/Modus",          [] (AsyncWebServerRequest *request) { handleModus(request);          });
   server.on("/SetSensitivity", [] (AsyncWebServerRequest *request) { handleSetSensitivity(request); });
   server.on("/SetLevel",       [] (AsyncWebServerRequest *request) { handleSetLevel(request);       });
   server.on("/SetBrightness",  [] (AsyncWebServerRequest *request) { handleSetBrightness(request);  });
   server.on("/data.json",      [] (AsyncWebServerRequest *request) { handleData(request);           });
  
   server.begin();
   Serial.println("HTTP server started");
}

/** Nothing needed at the moment. */
void MyWebServer::handleClient()
{
}

/** Sends the option values to the web interface. */
void MyWebServer::handleOptions(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleOptions");
   
   String opt;
   String onOffText = options->onOff ? "On" : "Off";
   String modusText;

   if (options->modus == MODUS_MICROPHONE) {
      modusText = "Mikrophone Modus";
   } else if (options->modus == MODUS_DEMO) {
      modusText = "Demo Modus";
   } else if (options->modus == MODUS_TEST) {
      modusText = "Test Modus";
   } else if (options->modus == MODUS_TEXT) {
      modusText = "Text Modus";
   } else {
      modusText = "Fehler";
   }

   opt += "<?xml version=\"1.0\"?>\r\n";
   opt += "<root>\r\n";
   opt += "   <onoff>"       + String(onOffText)            + "</onoff>\r\n";
   opt += "   <modus>"       + String(modusText)            + "</modus>\r\n";
   opt += "   <sensitifity>" + String(options->sensitivity) + "</sensitifity>\r\n";
   opt += "   <brightness>"  + String(options->brightness)  + "</brightness>\r\n";
   opt += "</root>\r\n";

   request->send(200, "text/xml", opt);
}

/** Switch on/off command from the web interface. */
void MyWebServer::handleOnOff(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleOnOff");
   
   options->onOff = ! options->onOff;
   options->save();

   request->send(200, "text/plain", "ok");
}

/** Modus commdn from the web interface. */
void MyWebServer::handleModus(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleModus");
   
   options->modus++;
   if (options->modus > MODUS_MAX) {
      options->modus = MODUS_MIN;
   }
   options->save();

   request->send(200, "text/plain", "ok");
}

/** Sensitivity changed from the web interface. */
void MyWebServer::handleSetSensitivity(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleSetSensitivity");

   if (request->hasParam("value")) {
      AsyncWebParameter *p = request->getParam("value");

      options->sensitivity = atoi(p->value().c_str());
      options->save();
   }
   
   request->send(200, "text/plain", "ok");
}

/** Level changed from the web interface. */
void MyWebServer::handleSetLevel(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleSetLevel");

   if (request->hasParam("value")) {
      AsyncWebParameter *p = request->getParam("value");
      
      options->level = atoi(p->value().c_str());
      options->save();
   }
   request->send(200, "text/plain", "ok");
}

/** Brightness changed from the web interface. */
void MyWebServer::handleSetBrightness(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleSetBrightness");

   if (request->hasParam("value")) {
      AsyncWebParameter *p = request->getParam("value");
      
      options->brightness = atoi(p->value().c_str());
      options->save();
   }
   request->send(200, "text/plain", "ok");
}

/** Sends the result from the spectrum analysis to the web interface. */
void MyWebServer::handleData(AsyncWebServerRequest *request)
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   Serial.println("handleData");

   String json;
   MyFft  fft;

   fft.begin();
   fft.sample(false);

   json += "[";
   for (int i = 2; i < 50; i++) {
      int dsize = (int) (fft.vReal[i] / amplitude);

      dsize = dsize * options->sensitivity / 50;
      dsize = dsize * (1.0 + (i - 25) / 25.0 * (50 - options->level) / 50.0);

      if (dsize > 20) {
         dsize = 20;
      }
      if (i > 2) {
         json += ","; 
      }
      json += "{\"x\":" + String(i) + ",\"y\":"+ String(dsize) + "}";
   }
   json += "]";
 
   request->send(200, "application/json", json);
}
