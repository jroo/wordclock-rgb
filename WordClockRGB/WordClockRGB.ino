/***************************************************************************
 *                                                                         *
 *  W O R D C L O C K   - A clock that tells the time using words.         *
 *                                                                         *
 * Hardware: Arduino Duemilanove with a set of individual LEDs under a     *
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

// Display output pin assignments
#define MTEN 	Display1=Display1 | (1<<0)  
#define HALF	Display1=Display1 | (1<<1)
#define QUARTER	Display1=Display1 | (1<<2)
#define TWENTY	Display1=Display1 | (1<<3)
#define MFIVE	Display1=Display1 | (1<<4)
#define ITIS	Display1=Display1 | (1<<5)
#define PAST	Display1=Display1 | (1<<6)
#define UNUSED1	Display1=Display1 | (1<<7)

#define TO	Display2=Display2 | (1<<0)
#define ONE	Display2=Display2 | (1<<1)
#define TWO	Display2=Display2 | (1<<2)
#define THREE	Display2=Display2 | (1<<3)
#define FOUR	Display2=Display2 | (1<<4)
#define HFIVE	Display2=Display2 | (1<<5)
#define SIX	Display2=Display2 | (1<<6)
#define UNUSED2	Display2=Display2 | (1<<7)

#define SEVEN	Display3=Display3 | (1<<0)
#define EIGHT	Display3=Display3 | (1<<1)
#define NINE	Display3=Display3 | (1<<2)
#define HTEN	Display3=Display3 | (1<<3)
#define ELEVEN	Display3=Display3 | (1<<4)
#define TWELVE	Display3=Display3 | (1<<5)
#define OCLOCK  Display3=Display3 | (1<<6)
#define UNUSED3	Display3=Display3 | (1<<7)

#define MIN1	Display4=Display4 | (1<<0)
#define MIN2	Display4=Display4 | (1<<1)
#define MIN3	Display4=Display4 | (1<<2)
#define MIN4	Display4=Display4 | (1<<3)
#define UNUSED4	Display4=Display4 | (1<<4)
#define UNUSED5	Display4=Display4 | (1<<5)
#define UNUSED6 Display4=Display4 | (1<<6)
#define UNUSED7	Display4=Display4 | (1<<7)

static unsigned long msTick =0;  // the number of Millisecond Ticks since we last 
                                 // incremented the second counter
char Display1=0, Display2=0, Display3=0, Display4=0;

// hardware constants
int LEDClockPin=6;
int LEDDataPin=7;
int LEDStrobePin=8;

int HourButtonPin=3;
int hourButtonDown = 0;

int MinuteButtonPin=2;
int minuteButtonDown = 0;

int PWMPin = 9;

int BrightnessButtonPin = 4;
int brightnessButtonDown = 0;
int brightness = 255;

int pushStart = 0;
int longPressDelay = 400; //time in millisecnods considered to be a long press of a button
int inLongPress = 0; //true if long press is happening

time_t t;


void setup()
{
  // initialize the hardware	
  pinMode(LEDClockPin, OUTPUT); 
  pinMode(LEDDataPin, OUTPUT); 
  pinMode(LEDStrobePin, OUTPUT); 
  pinMode(PWMPin, OUTPUT); 
  
  pinMode(MinuteButtonPin, INPUT); 
  pinMode(HourButtonPin, INPUT);
  pinMode(BrightnessButtonPin, INPUT);
  
  digitalWrite(MinuteButtonPin, HIGH);  //set internal pullup
  digitalWrite(HourButtonPin, HIGH); //set internal pullup
  digitalWrite(BrightnessButtonPin, HIGH); //set internal pullup

  //initialize system time from rtc
  Serial.begin(9600);
  Serial.println("getting time from RTC");
  setTime(RTC.get());   // the function to get the time from the RTC
  Serial.println("moving on");
  msTick=millis();      // Initialise the msTick counter
  displaytime();        // display the current time
}

void loop(void)
{  
  //selftest(); //uncomment to run in test mode
  analogWrite(PWMPin, brightness);
  
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
 Display1=0;
 Display2=0;
 Display3=0;
 Display4=0;
}

void WriteLEDs(void) {
 // Now we write the actual values to the hardware
 shiftOut(LEDDataPin, LEDClockPin, MSBFIRST, Display4);
 shiftOut(LEDDataPin, LEDClockPin, MSBFIRST, Display3);
 shiftOut(LEDDataPin, LEDClockPin, MSBFIRST, Display2);
 shiftOut(LEDDataPin, LEDClockPin, MSBFIRST, Display1);
 digitalWrite(LEDStrobePin,HIGH);
 delay(2);
 digitalWrite(LEDStrobePin,LOW);
}

void selftest(void){
  //cycle through each word and display it
  Serial.print("TEST");
  analogWrite(PWMPin, 32);
  
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

void displayHour(int offset) {
      switch (hourFormat12()+offset) {
        case 1: 
          ONE; 
          Serial.println("One ");
          break;
        case 2: 
          TWO; 
          Serial.println("Two ");
          break;
        case 3: 
          THREE; 
          Serial.println("Three ");
          break;
        case 4: 
          FOUR; 
          Serial.println("Four ");
          break;
        case 5: 
          HFIVE; 
          Serial.println("Five ");
          break;
        case 6: 
          SIX; 
          Serial.println("Six ");
          break;
        case 7: 
          SEVEN; 
          Serial.println("Seven ");
          break;
        case 8: 
          EIGHT; 
          Serial.println("Eight ");
          break;
        case 9: 
          NINE; 
          Serial.println("Nine ");
          break;
        case 10: 
          HTEN; 
          Serial.println("Ten ");
          break;
        case 11: 
          ELEVEN; 
          Serial.println("Eleven ");
          break;
        case 12: 
          TWELVE; 
          Serial.println("Twelve ");
          break;
         case 13:
          ONE;
          Serial.println("One ");
          break;
    }
}

void displaytime(void){

  digitalClockDisplay();
  
  // start by clearing the display to a known state
  ledsoff();
  
  ITIS;
  Serial.print("It is ");

  // now we display the appropriate minute counter
  if ((minute()>4) && (minute()<10)) { 
    MFIVE; 
    Serial.print("Five ");
  } 
  if ((minute()>9) && (minute()<15)) { 
    MTEN; 
    Serial.print("Ten ");
  }
  if ((minute()>14) && (minute()<20)) {
    QUARTER; 
      Serial.print("A Quarter ");
  }
  if ((minute()>19) && (minute()<25)) { 
    TWENTY; 
    Serial.print("Twenty ");
  }
  if ((minute()>24) && (minute()<30)) { 
    TWENTY; 
    MFIVE; 
    Serial.print("Twenty Five ");
  }  
  if ((minute()>29) && (minute()<35)) {
    HALF;
    Serial.print("Half ");
  }
  if ((minute()>34) && (minute()<40)) { 
    TWENTY; 
    MFIVE; 
    Serial.print("Twenty Five ");
  }  
  if ((minute()>39) && (minute()<45)) { 
    TWENTY; 
    Serial.print("Twenty ");
  }
  if ((minute()>44) && (minute()<50)) {
    QUARTER; 
    Serial.print("A Quarter ");
  }
  if ((minute()>49) && (minute()<55)) { 
    MTEN; 
    Serial.print("Ten ");
  } 
  if (minute()>54) { 
    MFIVE; 
    Serial.print("Five ");
  }

  if ((minute()<5))
  {
    displayHour(0);
    OCLOCK;
    Serial.println("O'Clock");
  }
  else
    if ((minute() < 35) && (minute() >4))
    {
      PAST;
      Serial.print("Past ");
      displayHour(0);
    }
    else
    {
      // if we are greater than 34 minutes past the hour then display
      // the next hour, as we will be displaying a 'to' sign
      TO;
      Serial.print("To ");
      displayHour(1);
    }
    
    //display individual minutes
    switch(minute() % 5) {
      case 0:
        Serial.println("");
        break;
      case 1:
        MIN1;
        Serial.println(".");
        break;
      case 2:
        MIN1;
        MIN2;
        Serial.println("..");
        break;
      case 3:
        MIN1;
        MIN2;
        MIN3;
        Serial.println("...");
        break;
      case 4:
        MIN1;
        MIN2;
        MIN3;
        MIN4;
        Serial.println("....");
        break;
   }
   WriteLEDs();
}


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

