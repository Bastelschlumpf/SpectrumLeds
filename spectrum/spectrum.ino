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
  * @file Spectrum.ino 
  *
  * Main file with setup() and loop() functions
  */

#include <Wire.h>
#include <SPI.h>
#include <FastLED.h>
#include <SPIFFS.h>
#include <mutex>
#include "Utils.h"
#include "Font7x5.h"
#include "FFT.h"
#include "Options.h"
#include "WebServer.h"

#define     LEDS_X    7           //!< Leds in X direction
#define     LEDS_Y   20           //!< Leds in Y direction
#define     LED_PIN  12           //!< Led connector pin on esp32

int         peak[LEDS_X];         //!< Array to store the last peak values.
double      peakColor = 0.0;      //!< Helper for rotating the peak color.
CRGB        leds[LEDS_X][LEDS_Y]; //!< LED RGB two dimensional array.

MyFft       fft;                  //!< Fast fourier helper class

MyOptions   options;              //!< Global options.
                             
MyWebServer webServer(options);   //!< Asynchron webserver


/** Move the peak values down. */
void peakLower()
{   
   for (int band = 0; band < LEDS_X; band++) {
      if (peak[band] > 0) {
         peak[band]--;
      }
   }
}

/** Calculate the led color corresponding to the spectrum value. */
CRGB LedColor(int WheelPos) 
{
   WheelPos = 255 - WheelPos;
   
   if (WheelPos < 85) {
      return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
   }
   if (WheelPos < 170) {
      WheelPos -= 85;
      return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
   }
   WheelPos -= 170;
   
   return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/** Helper function to debug the led content. */
void printLeds()
{
   Serial.println("");
   Serial.println("Leds");
   for (int y = LEDS_Y - 1; y >= 0; y--) {
      for (int x = 0; x < LEDS_X; x++) {
         Serial.printf("%03d%03d%03d ", leds[x][y].red, leds[x][y].green, leds[x][y].blue);
      }
      Serial.println("");
   }
}

/** Set all the leds to black if we switch off. */
void switchOff()
{
   for (int x = 0; x < LEDS_X; x++) {
      for (int y  = 0; y < LEDS_Y; y++) {
         leds[x][y] = CRGB::Black;
      }
   }
   FastLED.show();
}

/** Display the band value on the led strip. */
void displayBand(int band, int dsize)
{
   dsize = dsize * options.sensitivity / 50;
   dsize = dsize * (1.0 + (band - 3) / 3.0 * (50 - options.level) / 50.0);
   
   if (band > LEDS_X - 1) {
      return;   
   }
   
   if (dsize < 0) {
      dsize = 0;
   }
   if (dsize >= LEDS_Y - 1) {
      dsize = LEDS_Y - 1;
   }
   
   for (int y = 0; y < LEDS_Y; y++) {
      if (y <= dsize) {
         leds[band][y] = LedColor(y * 10);
      } else {
         leds[band][y] = CRGB::Black;
      }
   }
   if (dsize > peak[band]) {
      peak[band] = dsize;
   }
   leds[band][peak[band]] = LedColor(peakColor); 
}

/** Microphone modus: Sample the sound, calculate the spectrum and 
  * show the spectrum band values on the stripes.
  */
void doModusMicrophone(bool demo)
{
   fft.sample(demo);

   peakColor += 0.5;
   if (peakColor > 255) {
      peakColor = 0.0;
   }
   
   // Don't use sample 0 and only first SAMPLES/2 are usable. 
   displayBand(0, fft.calcAvg( 2,   2) / amplitude); 
   displayBand(1, fft.calcAvg( 3,   5) / amplitude); 
   displayBand(2, fft.calcAvg( 6,   9) / amplitude); 
   displayBand(3, fft.calcAvg(10,  13) / amplitude); 
   displayBand(4, fft.calcAvg(14,  16) / amplitude); 
   displayBand(5, fft.calcAvg(17,  20) / amplitude); 
   displayBand(6, fft.calcAvg(21,  25) / amplitude); 

   //printLeds();
   FastLED.setBrightness(options.brightness * 255 / 100);
   FastLED.show();
   peakLower();
}

/** Test modus: Show static values on the stripes with the options.
 *  Sensitivity, brightness and level.
 */
void doModusTest()
{
   for (int x = 0; x < LEDS_X; x++) {
      for (int y  = 0; y < LEDS_Y; y++) {
         if (y < options.level * LEDS_Y / 100) {
            leds[x][y] = LedColor(y * options.sensitivity);   
         } else {
            leds[x][y] = CRGB::Black;
         }
      }
   }
   FastLED.setBrightness(options.brightness * 255 / 100);
   FastLED.show();
}

/** Calculate the pixel value of the current scrolltext position. */
CRGB getGlyphPixel(String &scrollText, int scrollTextPos, int row)
{
   int  idx   = scrollTextPos / 5;
   int  c     = scrollText[idx];
   int  col   = scrollTextPos % 5;
   char glyph = Font7x5[c][6 - row];
   bool p     = false;

   switch(col) {
   case 0: p = glyph & 0b10000; break;
   case 1: p = glyph & 0b01000; break;
   case 2: p = glyph & 0b00100; break;
   case 3: p = glyph & 0b00010; break;
   case 4: p = glyph & 0b00001; break;
   }
   if (p) {
      return CRGB::Purple;
   } else {
      return CRGB::Black;
   }      
}

/** Scrolltext modus */
void doModusText()
{
   for (int x = 0; x < LEDS_X; x++) {
      for (int y  = 0; y < LEDS_Y; y++) {
         if (y >= 10 && y <= 16) {
            leds[x][y] = getGlyphPixel(options.scrollText, options.scrollTextPos + x, y - 10);
         } else {
            leds[x][y] = CRGB::Black;
         }
      }
   }
   FastLED.setBrightness(options.brightness * 255 / 100);
   FastLED.show();

   options.scrollTextPos++;
   if (options.scrollTextPos >= options.scrollText.length() * 5) {
      options.scrollTextPos = 0;
   }
   /* Debugging
   for (int y  = 0; y < LEDS_Y; y++) {
      for (int x = 0; x < LEDS_X; x++) {
         if (leds[x][y] == (CRGB) CRGB::Black) {
            Serial.print(".");
         } else {
            Serial.print("*");
         }
      }
      Serial.println("");
   }
   Serial.println("");
   */
   delay(30);
}

/** Initialize all the program components. */
void setup() 
{
   Serial.begin(115200);
   delay(2000);

   SPIFFS.begin();
   options.load();

   fft.begin();

   webServer.begin();
   
   FastLED.addLeds<WS2812B, LED_PIN, RGB>((CRGB *) leds, LEDS_X * LEDS_Y);
}

/** Main loop of the program. */
void loop() 
{
   //Serial.println("loop");

   webServer.handleClient();

   if (!options.onOff) {
      switchOff();
   } else {
      if (options.modus == MODUS_MICROPHONE) {
         doModusMicrophone(false);
      } else if (options.modus == MODUS_DEMO) {
         doModusMicrophone(true);
      } else if (options.modus == MODUS_TEST) {
         doModusTest();
      } else if (options.modus == MODUS_TEXT) {
         doModusText();
      }
   }

   yield();
   delay(50);
}
