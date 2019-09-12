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
  * @file FFT.h
  *
  * Class for the Fast Fourier Transformation for the spectrum analyser.
  */

#include <driver/adc.h>
#include <arduinoFFT.h>

#define SAMPLES              512 // Must be a power of 2
#define SAMPLING_FREQUENCY 40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude            400 // Depending on your audio source level, you may need to increase this value

/** 
  * Class with all the helper functions for the spectrum analyser.
  */
class MyFft
{
protected:
   std::mutex   mutex_;              //!< Mutex for multithreading synchronisations.
   arduinoFFT   FFT;                 //!< Fast fourier transformation library
   unsigned int sampling_period_us;  //!< Timeframe of the sampling
   double       vImag[SAMPLES];      //!< Imag values of the analysis

public:
   double       vReal[SAMPLES];      //!< Real values of the analysis

protected:
   void analogTest();
   void fftTest   ();
   
public:
   MyFft();

   void   begin  ();
   void   sample (bool demo);
   double calcAvg(int from, int to);
};

/** Constructor/Destructor */
MyFft::MyFft()
   : FFT(arduinoFFT())
{
   sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

/** Initialize the FFT helper class. Configures the analog input chanel */
void MyFft::begin()
{
   std::lock_guard<std::mutex> lock(mutex_);
   
   adc1_config_width(ADC_WIDTH_BIT_12);
   adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

   for (int i = 0; i < SAMPLES; i++) {
      vReal[i] = 0;
      vImag[i] = 0;   
   }
}

/** Helper function for debugging the sample data. */ 
void MyFft::analogTest()
{
   double min = 100000.0;
   double max = 0.0; 
   double avg = 0.0;
   for (int i = 0; i < SAMPLES; i++) { 
      //Serial.println(String(i) + " : " + String(vReal[i]));
      avg += vReal[i];
      if (vReal[i] < min) {
         min = vReal[i];
      }
      if (vReal[i] > max) {
         max = vReal[i];
      }
   }
   avg = avg / SAMPLES;
   Serial.println("Amp: " + String(max - min) + " Min: " + String(min) + " Max: " + String(max) + " Avg: " + String(avg));
}

/** Calculate the average of the sample adata. */
double MyFft::calcAvg(int from, int to)
{
   double dRet = 0.0;
   
   for (int i = from; i <= to; i++) {
      dRet += vReal[i];
   }
   return dRet / (to - from + 1);
}

/** Debugging the calculated spectrum data */
void MyFft::fftTest()
{
   double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

   Serial.println("Peak: " + String(peak));
   Serial.println("  2 -  2: " + String(calcAvg( 2,   2) / amplitude));
   Serial.println("  3 -  5: " + String(calcAvg( 3,   5) / amplitude));
   Serial.println("  6 -  9: " + String(calcAvg( 6,   9) / amplitude));
   Serial.println(" 10 - 13: " + String(calcAvg(10,  13) / amplitude));
   Serial.println(" 14 - 16: " + String(calcAvg(14,  16) / amplitude));
   Serial.println(" 17 - 20: " + String(calcAvg(17,  20) / amplitude));
   Serial.println(" 21 - 25: " + String(calcAvg(21,  25) / amplitude));
   Serial.println("");
   delay(5000);
}

/** Sample the microphone input and do the fast fourier analysis. */
void MyFft::sample(bool demo)
{
   std::lock_guard<std::mutex> lock(mutex_);

   unsigned long newTime = micros();
   unsigned long oldTime = newTime;
   
   for (int i = 0; i < SAMPLES; i++) {
      newTime = micros() - oldTime;
      oldTime = newTime;

      if (demo) {
         vReal[i] = random(1014); // Analog: 0 - 4096
      } else {
         vReal[i] = analogRead(A0);
         //vReal[i] = adc1_get_raw(ADC1_CHANNEL_0); 
      }
      vImag[i] = 0;

      while (micros() < (newTime + sampling_period_us))  { 
         // waiting
      }
   }
   
   // analogTest();
   
   FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
   FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
   FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

   //fftTest();
}
