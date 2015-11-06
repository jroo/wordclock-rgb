/***************************************************************************
 *                                                                         *
 *  W O R D C L O C K R G B   - A clock that tells the time using words.   *
 *                                                                         *
 * Hardware: Arduino with a set of individual LEDs under a                 *
 * word stencil.                                                           *
 *                                                                         *
 *                                                                         *
 *   Original Copyright (C) 2009  Doug Jackson (doug@doughq.com)           *
 *   Modifications Copyright (C) 2010 Scott Bezek (scott@bezekhome.com)    *
 *   Modifications Copyright (C) 2015 Joshua Ruihley                       *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *    it under the terms of the GNU General Public License as published by *
 *    the Free Software Foundation, either version 3 of the License, or    *
 *    (at your option) any later version.                                  *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *    GNU General Public License for more details.                         *
 *                                                                         *
 *    You should have received a copy of the GNU General Public License    *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 *                                                                         *
 ***************************************************************************
 * 
 * Revision History
 * 
 * Date         By          What
 * 20001025     DRJ         Initial Creation of Arduino Version 
 *                              - based on Wordclock.c - from PIC version
 * 20100124     Scott Bezek Changed LED pinout, added brightness control,
 *                              changed buttons to hour/minute increment 
 * 20150802     JR          added four LEDs for minute display, changed
 *                          brightness setting trigger to a button, added
 *                          "IT IS" and removed "Minutes" from display.
 */
 
#include <Time.h>  
#include <Wire.h>  
#include <DS1307RTC.h> 
#include <Adafruit_NeoPixel.h>

// Matrix types defined at https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library
// progressive: Row Major Progressive
// zigzag: Row Major Zigzag
#define MATRIX_TYPE "progressive"

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(256, 6, NEO_GRB + NEO_KHZ800);

struct coords {
  int y;
  int x;
};


#define NUMROWS 16
#define NUMCOLS 16

coords ITIS[] = {{2,2},{2,3},{2,5},{2,6}};
coords QUARTER[] = {{3,2},{3,4},{3,5},{3,6},{3,7},{3,8},{3,9},{3,10}};
coords TWENTY[] =  {{4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}};
coords MFIVE[] = {{4,8}, {4,9}, {4,10}, {4,11}};
coords HALF[] = {{5,2}, {5,3}, {5,4}, {5,5}};
coords MTEN[] = {{5,7}, {5,8}, {5,9}};
coords TO[] = {{5,11}, {5,12}}; 
coords PAST[] = {{6,2}, {6,3}, {6,4}, {6,5}}; //   {int 6,[2,3,4,5]}
coords NINE[] = {{6,9}, {6,10}, {6,11}, {6,12}}; //    {int 6,[9,10,11,12]}
coords ONE[] = {{7,2}, {7,3}, {7,4}}; //     {int 7,[2,3,4]}
coords SIX[] = {{7,5}, {7,6}, {7,7}}; //     {int 7,[5,6,7]}
coords THREE[] = {{7,8}, {7,9}, {7,10}, {7,11}, {7,12}}; //   {int 7,[8,9,10,11,12]}
coords FOUR[] = {{8,2}, {8,3}, {8,4}, {8,5}}; //    {int 8,[2,3,4,5]}
coords HFIVE[] = {{8,6}, {8,7}, {8,8}, {8,9}}; //   {int 8,[6,7,8,9]}
coords TWO[] = {{8,10}, {8,11}, {8,12}}; //     {int 8,[10,11,12]}
coords EIGHT[] = {{9,2}, {9,3}, {9,4}, {9,5}, {9,6}}; //   {int 9,[2,3,4,5,6]}
coords ELEVEN[] = {{9,7}, {9,8}, {9,9}, {9,10}, {9,11}, {9,12}}; //  {int 9,[7,8,9,10,11,12]}
coords SEVEN[] = {{10,2}, {10,3}, {10,4}, {10,5}, {10,6}}; //   {int 10,[2,3,4,5,6]}
coords TWELVE[] = {{10,7}, {10,8}, {10,9}, {10,10}, {10,11}, {10,12}}; //  {int 10,[7,8,9,10,11,12]}
coords HTEN[] = {{11,2}, {11,3}, {11,4}}; //    {int 11,[2,3,4]}
coords OCLOCK[] = {{11,7}, {11,8}, {11,9}, {11,10}, {11,11}, {11,12}}; //  {int 11,[7,8,9,10,11,12]}
coords MIN1[] = {{0,0}}; //    {int 1, [1]}
coords MIN2[] = {{0,15}}; //    {int 15, [1]}
coords MIN3[] = {{15,0}}; //    {int 15, [15]}
coords MIN4[] = {{15,15}}; //    {int 1, [15]}

static unsigned long msTick =0;  // the number of Millisecond Ticks since we last 
                                 // incremented the second counter

// hardware constants
int HourButtonPin=4;
int hourButtonDown = 0;

int MinuteButtonPin=5;
int minuteButtonDown = 0;

int brightness = 32;

int pushStart = 0;
int longPressDelay = 400; //time in millisecnods considered to be a long press of a button
int inLongPress = 0; //true if long press is happening

time_t t;


void setup()
{
  // initialize the hardware	
  pinMode(MinuteButtonPin, INPUT); 
  pinMode(HourButtonPin, INPUT);
  
  digitalWrite(MinuteButtonPin, HIGH);  //set internal pullup
  digitalWrite(HourButtonPin, HIGH); //set internal pullup

  //initialize system time from rtc
  Serial.begin(9600);
  Serial.println("getting time from RTC");
  setTime(RTC.get());   // the function to get the time from the RTC
  Serial.println("moving on");
  msTick=millis();      // Initialise the msTick counter
  displaytime();        // display the current time

  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'
}

void loop(void)
{  
  //selftest(); //uncomment to run in test mode
  
    // heart of the timer - keep looking at the millisecond timer on the Arduino
    // and increment the seconds counter every 1000 ms
    if ( millis() - msTick >999) {
        msTick=millis();
        // Flash the onboard Pin13 Led so we know something is happening!
        digitalWrite(13,HIGH);
        delay(100);
        digitalWrite(13,LOW);    
    }
    
    //test to see if we need to increment the time counters
    if (second()==0) 
    {
      displaytime();
      delay(1000);
    }
    
    //check to see if buttons are being pressed
    checkHourButton();
    checkMinuteButton();
    //checkBrightnessButton();
}

void ledsoff(void) {
  //turn all leds off
  for(int i=0; i<256; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
}

int getIndex(int y, int x) {
  //given an x,y coordinate, return the pixel number on the light strip
  int index;
  if (MATRIX_TYPE == "progressive") {
      index = y * 16 + x;
  } else {
    if (y == 0)
    {
      index = 15 - x;
    }
    else if (y % 2 != 0)
    {
      index = y * 16 + x;
    }
    else
    {
      index = (y * 16 + 15) - x;
    }
  } 
  return index;
}

void DisplayWord(coords word_info[], String timeword) {
  // Now we write the actual values to the hardware
  for (int i=0; i<sizeof(word_info); i++) {
    strip.setPixelColor(getIndex(word_info[i].y, word_info[i].x), strip.Color(128, 0, 0));
  }
}

/*
void selftest(void){
  //cycle through each word and display it
  Serial.print("TEST");

  ledsoff(); ITIS; MTEN; HALF; QUARTER; TWENTY; MFIVE; PAST; TO; ONE; TWO; THREE; FOUR; HFIVE; SIX; SEVEN; EIGHT; NINE; HTEN; ELEVEN; TWELVE; OCLOCK; MIN1; MIN2; MIN3; MIN4; WriteLEDs(); delay(2000); 
  
  ledsoff(); ITIS; WriteLEDs(); delay(1000); 
  ledsoff(); MTEN; WriteLEDs(); delay(1000); 
  ledsoff(); HALF; WriteLEDs(); delay(1000); 
  ledsoff(); QUARTER; WriteLEDs(); delay(1000); 
  ledsoff(); TWENTY; WriteLEDs(); delay(1000); 
  ledsoff(); MFIVE; WriteLEDs(); delay(1000); 
  ledsoff(); PAST; WriteLEDs(); delay(1000); 
  ledsoff(); TO; WriteLEDs(); delay(1000); 
  ledsoff(); ONE; WriteLEDs(); delay(1000); 
  ledsoff(); TWO; WriteLEDs(); delay(1000); 
  ledsoff(); THREE; WriteLEDs(); delay(1000); 
  ledsoff(); FOUR; WriteLEDs(); delay(1000); 
  ledsoff(); HFIVE; WriteLEDs(); delay(1000); 
  ledsoff(); SIX; WriteLEDs(); delay(1000); 
  ledsoff(); SEVEN; WriteLEDs(); delay(1000); 
  ledsoff(); EIGHT; WriteLEDs(); delay(1000); 
  ledsoff(); NINE; WriteLEDs(); delay(1000); 
  ledsoff(); HTEN; WriteLEDs(); delay(1000); 
  ledsoff(); ELEVEN; WriteLEDs(); delay(1000); 
  ledsoff(); TWELVE; WriteLEDs(); delay(1000); 
  ledsoff(); OCLOCK; WriteLEDs(); delay(1000);
  ledsoff(); MIN1; WriteLEDs(); delay(1000); 
  ledsoff(); MIN2; WriteLEDs(); delay(1000); 
  ledsoff(); MIN3; WriteLEDs(); delay(1000); 
  ledsoff(); MIN4; WriteLEDs(); delay(1000);

  ledsoff(); MIN1; WriteLEDs(); Serial.println("MIN1"); delay(1000); 
  ledsoff(); MIN2; WriteLEDs(); Serial.println("MIN2"); delay(1000); 
  ledsoff(); MIN3; WriteLEDs(); Serial.println("MIN3"); delay(1000); 
  ledsoff(); MIN4; WriteLEDs(); Serial.println("MIN4"); delay(1000);
  ledsoff(); delay(1000);
 }
 */

void displayHour(int offset) {
      switch (hourFormat12()+offset) {
        case 1: 
          DisplayWord(ONE, "One");
          break;
        case 2: 
          DisplayWord(TWO, "Two");
          break;
        case 3: 
          DisplayWord(THREE, "Three");
          break;
        case 4: 
          DisplayWord(FOUR, "Four");
          break;
        case 5: 
          DisplayWord(HFIVE, "Five"); 
          break;
        case 6: 
          DisplayWord(SIX, "Six");
          break;
        case 7: 
          DisplayWord(SEVEN, "Seven");
          break;
        case 8: 
          DisplayWord(EIGHT, "Eight"); 
          break;
        case 9: 
          DisplayWord(NINE, "Nine");
          break;
        case 10: 
          DisplayWord(HTEN, "Ten"); 
          break;
        case 11: 
          DisplayWord(ELEVEN, "Eleven"); 
          break;
        case 12: 
          DisplayWord(TWELVE, "Twelve");
          break;
         case 13:
          DisplayWord(ONE, "One");
          break;
    }
}

void displaytime(void){

  digitalClockDisplay();
  ledsoff();
  
  DisplayWord(ITIS, "It Is");

  // now we display the appropriate minute counter
  if ((minute()>4) && (minute()<10)) { 
    DisplayWord(MFIVE, "Five");
  } 
  if ((minute()>9) && (minute()<15)) { 
    DisplayWord(MTEN, "Ten");
  }
  if ((minute()>14) && (minute()<20)) {
    DisplayWord(QUARTER, "Quarter"); 
  }
  if ((minute()>19) && (minute()<25)) { 
    DisplayWord(TWENTY,"Twenty");
  }
  if ((minute()>24) && (minute()<30)) { 
    DisplayWord(TWENTY, "Twenty");
    DisplayWord(MFIVE, "Five");
  }  
  if ((minute()>29) && (minute()<35)) {
    DisplayWord(HALF, "Half");
  }
  if ((minute()>34) && (minute()<40)) { 
    DisplayWord(TWENTY, "Twenty"); 
    DisplayWord(MFIVE, "Five"); 
  }  
  if ((minute()>39) && (minute()<45)) { 
    DisplayWord(TWENTY, "Twenty");
  }
  if ((minute()>44) && (minute()<50)) {
    DisplayWord(QUARTER, "Quarter");
  }
  if ((minute()>49) && (minute()<55)) { 
    DisplayWord(MTEN, "Ten");
  } 
  if (minute()>54) { 
    DisplayWord(MFIVE, "Five");
  }

  if ((minute()<5))
  {
    displayHour(0);
    DisplayWord(OCLOCK, "O'Clock");
  }
  else
    if ((minute() < 35) && (minute() >4))
    {
      DisplayWord(PAST, "Past");
      displayHour(0);
    }
    else
    {
      // if we are greater than 34 minutes past the hour then display
      // the next hour, as we will be displaying a 'to' sign
      DisplayWord(TO, "To");
      displayHour(1);
    }
    
    //display individual minutes
    switch(minute() % 5) {
      case 0:
        break;
      case 1:
        DisplayWord(MIN1, ".");
        break;
      case 2:
        DisplayWord(MIN1, ".");
        DisplayWord(MIN2, ".");
        break;
      case 3:
        DisplayWord(MIN1, ".");
        DisplayWord(MIN2, ".");
        DisplayWord(MIN3, ".");
        break;
      case 4:
        DisplayWord(MIN1, ".");
        DisplayWord(MIN2, ".");
        DisplayWord(MIN3, ".");
        DisplayWord(MIN4, ".");
        break;
   }
   strip.show();
}


/*
void decreaseBrightness(void) {
   //if brightness isn't on lowest setting, cut in half. 
   //otherwise return to full brightness
   if (brightness == 1) {
     brightness = 255;
   } else {
     brightness = int(brightness/2);
   }
   analogWrite(PWMPin, brightness);
   Serial.println("Brightness set to ");
   Serial.println(brightness);
 }
 */
 
void checkHourButton() {
   //set hours based on hour button behavior
   
    if (digitalRead(HourButtonPin) == 0 && hourButtonDown == 0) {
      //hour button pushed
      hourButtonDown = 1; //hour button is being pressed
      pushStart = millis(); //remember time that button was first pressed
    }
    
    if (digitalRead(HourButtonPin) == 1 && hourButtonDown == 1) {
      //hour button released: if released from a long press, do nothing.
      //if released from a 'short' press, increase the hour.
      if (!inLongPress) {
        adjustTime(3600);
        RTC.set(now());
        displaytime();
      }
      //reset button status
      hourButtonDown = 0; //indicate that hour button is no longer pressed
      inLongPress = 0; //indicate that button is no longer in a long press
    }
    
    if (hourButtonDown && ((millis() - pushStart) > longPressDelay)) {
      //if hour button is in a long press, increase hour every 200ms
      inLongPress = 1;
      if (millis() % 200 == 0) {
        adjustTime(3600);
        RTC.set(now());
        displaytime();
      }
    }
}

void checkMinuteButton() {
  
  //set minutes based on minute button behavior
  
  if (digitalRead(MinuteButtonPin) == 0 && minuteButtonDown == 0) {
    //minute button pushed
    minuteButtonDown = 1; //minute button is being pressed
    pushStart = millis(); //mark time that button was pushed
  }
  
  if (digitalRead(MinuteButtonPin) == 1 && minuteButtonDown == 1) {
    //minute button released
    if (!inLongPress) {
      adjustTime(60);
      RTC.set(now());
      displaytime();
    }

    //reset button status
    minuteButtonDown = 0; //minute button is no longer being pressed
    inLongPress = 0; //minute button is no longer in a long press
  }
  
  if (minuteButtonDown && ((millis() - pushStart) > longPressDelay)) {
    //if minute button is in a long press, increase minute every 200ms
    inLongPress = 1;
    if (millis() % 200 == 0) {
      adjustTime(60);
      RTC.set(now());
      displaytime();
    }
  }  
}

/*
void checkBrightnessButton() {
  //set brightness based on brightness button behavior
  if (digitalRead(BrightnessButtonPin) == 0) {
    brightnessButtonDown = 1;
  }
  if (digitalRead(BrightnessButtonPin) == 1 && brightnessButtonDown == 1) {
    brightnessButtonDown = 0;
    Serial.println("Brightness button released");
    decreaseBrightness();
    displaytime();
  }
}
*/

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

