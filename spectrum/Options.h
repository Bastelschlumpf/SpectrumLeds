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
  * @file Options.h
  *
  * Configuration data with load and save to the SPIFFS.
  */

#define OPTION_FILE_NAME "/options.txt" //!< Option file name.

#define MODUS_MIN        0
#define MODUS_MICROPHONE 0
#define MODUS_DEMO       1
#define MODUS_TEST       2
#define MODUS_TEXT       3
#define MODUS_MAX        3

/** 
  * Class with the complete configuration data of the programm.
  * It can load and save the data in a ini file format to the SPIFFS
  * as key value pairs 'key=value' line by line.
  */
class MyOptions
{
public:
   bool   onOff;         //!< Is the modul on or off?
   long   modus;         //!< Switch between the different modus.
   long   maxModus;      //!< Maximum modus enum value.
   long   sensitivity;   //!< Sensitifity 0 - 100
   long   level;         //!< Level       0 - 100
   long   brightness;    //!< Brigtness   0 - 100 
   String scrollText;    //!< Scroll-Text content.
   int    scrollTextPos; //!< Current scrolltext position.

public:
   MyOptions();

   bool load();
   bool save();
};

/* ******************************************** */

MyOptions::MyOptions()
   : onOff(true)
   , modus(1)
   , maxModus(5)
   , sensitivity(50)
   , brightness(100)
   , level(0)
   , scrollTextPos(0)
{
   scrollText = "This is a scrolling text sample to dislay on the led stripes.";
}

/** Load the key-value pairs from the option file into the option values. */
bool MyOptions::load()
{
   bool ret  = false;
   File file = SPIFFS.open(OPTION_FILE_NAME, "r");

   if (!file) {
      Serial.println(F("Failed to read options file"));
   } else {
      ret = true;
      while (file.available() && ret) {
         String line = file.readStringUntil('\n');
         int    idx  = line.indexOf('=');

         if (idx == -1) {
            Serial.println((String) F("Wrong option entry: ") + line);
            ret = false;
         } else {
            String key    = line.substring(0, idx);
            String value  = line.substring(idx + 1);
            long   lValue = atol(value.c_str());
            double fValue = atof(value.c_str());

            value.replace("\r", "");
            value.replace("\n", "");
            Serial.println((String) F("Load option '") + key + F("=") + value + F("'"));

            if (key == F("onOff")) {
               onOff = lValue;
            } else if (key == F("modus")) {
               modus = lValue;
            } else if (key == F("sensitivity")) {
               sensitivity = lValue;
            } else if (key == F("level")) {
               level = fValue;
            } else if (key == F("brightness")) {
               brightness = fValue;
            } else {
               Serial.println((String) F("Wrong option entry: ") + line);
               ret = false;
            }
         }
      }
      file.close();
   }
   if (ret) {
      Serial.println(F("Settings loaded"));
   }
   return ret;
}

/** Save all the options as key-value pair to the option file. */
bool MyOptions::save()
{
   // return true; // Disabled for testing
   
   File file = SPIFFS.open(OPTION_FILE_NAME, "w+");

   if (!file) {
      Serial.println("Failed to write options file");
   } else {
      file.println((String) F("onOff=")       + String(onOff));
      file.println((String) F("modus=")       + String(modus));
      file.println((String) F("sensitivity=") + String(sensitivity));
      file.println((String) F("level=")       + String(level));
      file.println((String) F("brightness=")  + String(brightness));
      file.close();
      Serial.println(F("Settings saved"));
      return true;
   }
   return false;
}
