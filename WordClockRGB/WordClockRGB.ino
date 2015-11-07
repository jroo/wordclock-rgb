/*
* RGB Word Clock
* https://github.com/jroo/wordclock-rgb/
* 
* Modified from the following:
*   - 2009: Doug Jackson, http://www.instructables.com/id/The-Wordclock-Grew-Up/
*   - 2010: Scott Bezek, http://scottbezek.blogspot.com/2010/07/my-first-instructable.html
*   - 2015: Josh Ruihley, https://github.com/jroo/wordclock
*
* This program is free software: you can redistribute it and/or modify  
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation, either version 3 of the License, or    
* (at your option) any later version.                                  
*
* This program is distributed in the hope that it will be useful,      
* but WITHOUT ANY WARRANTY; without even the implied warranty of       
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
* GNU General Public License for more details.                         
*                                                                         
* You should have received a copy of the GNU General Public License    
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
*/

#include <Time.h>  
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
  byte y;
  byte x;
};

struct word_info {
  String title;
  byte len;
  coords pos[9];
};

#define ITIS    {F("IT IS"), 4, { {3,2}, {3,3}, {3,5}, {3,6} } }
#define QUARTER {F("A QUARTER"), 9, { {4,2}, {4,4}, {4,5}, {4,6}, {4,7}, {4,8}, {4,9}, {4,10} } }
#define TWENTY  {F("TWENTY"), 6, { {5,2}, {5,3}, {5,4}, {5,5}, {5,6}, {5,7} } }
#define MFIVE   {F("FIVE"), 4, { {5,8}, {5,9}, {5,10}, {5,11} } }
#define HALF    {F("HALF") , 4, { {6,2}, {6,3}, {6,4}, {6,5} } }
#define MTEN    {F("TEN"), 3, { {6,7}, {6,8}, {6,9} } }
#define TO      {F("TO"), 2, { {6,11}, {6,12} } }
#define PAST    {F("PAST"), 4, { {7,2}, {7,3}, {7,4}, {7,5} } }
#define NINE    {F("NINE"), 4, { {7,9}, {7,10}, {7,11}, {7,12} } }
#define ONE     {F("ONE"), 3, { {8,2}, {8,3}, {8,4} } }
#define SIX     {F("SIX"), 3, { {8,5}, {8,6}, {8,7} } }
#define THREE   {F("THREE"), 5, { {8,8}, {8,9}, {8,10}, {8,11}, {8,12} } }
#define FOUR    {F("FOUR"), 4, { {9,2}, {9,3}, {9,4}, {9,5} } }
#define HFIVE   {F("FIVE"), 4, { {9,6}, {9,7}, {9,8}, {9,9} } }
#define TWO     {F("TWO"), 3, { {9,10}, {9,11}, {9,12} } }
#define EIGHT   {F("EIGHT"), 5, { {10,2}, {10,3}, {10,4}, {10,5}, {10,6} } }
#define ELEVEN  {F("ELEVEN"), 6, { {10,7}, {10,8}, {10,9}, {10,10}, {10,11}, {10,12} } }
#define SEVEN   {F("SEVEN"), 5, { {11,2}, {11,3}, {11,4}, {11,5}, {11,6} }}
#define TWELVE  {F("TWELVE"), 6, { {11,7}, {11,8}, {11,9}, {11,10}, {11,11}, {11,12} } }
#define HTEN    {F("TEN"), 3, { {12,2}, {12,3}, {12,4} } }
#define OCLOCK  {F("OCLOCK"), 5, { {12,7}, {12,8}, {12,9}, {12,10}, {12,11}, {12,12} } }
#define MIN1    {F("."), 1, { {0,0} } }
#define MIN2    {F("."), 1, { {0,15} } }
#define MIN3    {F("."), 1, { {15,0} } }
#define MIN4    {F("."), 1, { {15,15} } }

#define NUMROWS 16
#define NUMCOLS 16
#define MAX_BRIGHTNESS 64 //do not increase unless you're absolutely sure you have the proper power supply to drive it

static unsigned long msTick =0;  // the number of Millisecond Ticks since we last 
                                 // incremented the second counter


byte BrightnessPin = 3;
byte brightnessButtonDown = 0;
byte brightness = MAX_BRIGHTNESS;

byte HourPin = 4;
byte hourButtonDown = 0;

byte MinutePin = 5;
byte minuteButtonDown = 0;

byte pushStart = 0;
int longPressDelay = 400; //time in millisecnods considered to be a long press of a button
byte inLongPress = 0; //true if long press is happening

time_t t;

void setup()
{
  // initialize the hardware	
  pinMode(MinutePin, INPUT); 
  pinMode(HourPin, INPUT);
  
  digitalWrite(MinutePin, HIGH);  //set internal pullup
  digitalWrite(HourPin, HIGH); //set internal pullup

  //initialize system time from rtc
  Serial.begin(9600);
  Serial.println(F("Getting time from RTC"));
  setTime(RTC.get());   // the function to get the time from the RTC
  Serial.println(F("Moving on"));
  msTick=millis();      // Initialise the msTick counter
  displaytime();        // display the current time

  strip.begin();
  strip.setBrightness(MAX_BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'
}

void loop(void)
{    
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
    checkBrightnessButton();
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

void DisplayWord(word_info cword) {
  // Now we write the actual values to the hardware
  Serial.print(cword.title);
  Serial.print(F(" "));
  for (int i=0; i<cword.len; i++) {
    strip.setPixelColor(getIndex(cword.pos[i].y, cword.pos[i].x), strip.Color(128, 128, 128));
  }
}

void displayHour(int offset) {
      switch (hourFormat12()+offset) {
        case 1: 
          DisplayWord(ONE);
          break;
        case 2: 
          DisplayWord(TWO);
          break;
        case 3: 
          DisplayWord(THREE);
          break;
        case 4: 
          DisplayWord(FOUR);
          break;
        case 5: 
          DisplayWord(HFIVE); 
          break;
        case 6: 
          DisplayWord(SIX);
          break;
        case 7: 
          DisplayWord(SEVEN);
          break;
        case 8: 
          DisplayWord(EIGHT); 
          break;
        case 9: 
          DisplayWord(NINE);
          break;
        case 10: 
          DisplayWord(HTEN); 
          break;
        case 11: 
          DisplayWord(ELEVEN); 
          break;
        case 12: 
          DisplayWord(TWELVE);
          break;
         case 13:
          DisplayWord(ONE);
          break;
    }
}

void displaytime(void){

  digitalClockDisplay();
  ledsoff();
  
  DisplayWord(ITIS);

  // now we display the appropriate minute counter
  if ((minute()>4) && (minute()<10)) { 
    DisplayWord(MFIVE);
  } 
  if ((minute()>9) && (minute()<15)) { 
    DisplayWord(MTEN);
  }
  if ((minute()>14) && (minute()<20)) {
    DisplayWord(QUARTER); 
  }
  if ((minute()>19) && (minute()<25)) { 
    DisplayWord(TWENTY);
  }
  if ((minute()>24) && (minute()<30)) { 
    DisplayWord(TWENTY);
    DisplayWord(MFIVE);
  }  
  if ((minute()>29) && (minute()<35)) {
    DisplayWord(HALF);
  }
  if ((minute()>34) && (minute()<40)) { 
    DisplayWord(TWENTY); 
    DisplayWord(MFIVE); 
  }  
  if ((minute()>39) && (minute()<45)) { 
    DisplayWord(TWENTY);
  }
  if ((minute()>44) && (minute()<50)) {
    DisplayWord(QUARTER);
  }
  if ((minute()>49) && (minute()<55)) { 
    DisplayWord(MTEN);
  } 
  if (minute()>54) { 
    DisplayWord(MFIVE);
  }

  if ((minute()<5))
  {
    displayHour(0);
    DisplayWord(OCLOCK);
  }
  else
    if ((minute() < 35) && (minute() >4))
    {
      DisplayWord(PAST);
      displayHour(0);
    }
    else
    {
      // if we are greater than 34 minutes past the hour then display
      // the next hour, as we will be displaying a 'to' sign
      DisplayWord(TO);
      displayHour(1);
    }
    
    //display individual minutes
    switch(minute() % 5) {
      case 0:
        break;
      case 1:
        DisplayWord(MIN1);
        break;
      case 2:
        DisplayWord(MIN1);
        DisplayWord(MIN2);
        break;
      case 3:
        DisplayWord(MIN1);
        DisplayWord(MIN2);
        DisplayWord(MIN3);
        break;
      case 4:
        DisplayWord(MIN1);
        DisplayWord(MIN2);
        DisplayWord(MIN3);
        DisplayWord(MIN4);
        break;
   }
   strip.show();
   Serial.println();
}


void decreaseBrightness(void) {
   //if brightness isn't on lowest setting, cut in half. 
   //otherwise return to full brightness
   if (brightness == 1) {
     brightness = MAX_BRIGHTNESS;
   } else {
     brightness = int(brightness/2);
   }
   strip.setBrightness(brightness);
   Serial.print(F("Brightness set to "));
   Serial.println(brightness);
 }
 
void checkHourButton() {
   //set hours based on hour button behavior
   
    if (digitalRead(HourPin) == 0 && hourButtonDown == 0) {
      //hour button pushed
      hourButtonDown = 1; //hour button is being pressed
      pushStart = millis(); //remember time that button was first pressed
    }
    
    if (digitalRead(HourPin) == 1 && hourButtonDown == 1) {
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
  
  if (digitalRead(MinutePin) == 0 && minuteButtonDown == 0) {
    //minute button pushed
    minuteButtonDown = 1; //minute button is being pressed
    pushStart = millis(); //mark time that button was pushed
  }
  
  if (digitalRead(MinutePin) == 1 && minuteButtonDown == 1) {
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

void checkBrightnessButton() {
  //set brightness based on brightness button behavior
  if (digitalRead(BrightnessPin) == 0) {
    brightnessButtonDown = 1;
  }
  if (digitalRead(BrightnessPin) == 1 && brightnessButtonDown == 1) {
    brightnessButtonDown = 0;
    Serial.println(F("Brightness button released"));
    decreaseBrightness();
    displaytime();
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(F(" "));
  Serial.print(day());
  Serial.print(F(" "));
  Serial.print(month());
  Serial.print(F(" "));
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(F(":"));
  if(digits < 10)
    Serial.print(F("0"));
  Serial.print(digits);
}

