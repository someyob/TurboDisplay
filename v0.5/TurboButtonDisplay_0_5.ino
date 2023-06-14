/*
Turbo Button LED display  v0.5
written for Arduino Pro Mini
KJ 14 June 2023

CP2102 to Pro Mini connection:
5v -> Vcc
GND -> GND
Rx -> Tx
Tx -> Rx
DTR -> GRN
Ref: https://forum.arduino.cc/t/programming-pro-mini-using-cp2102-module/242828

7seg library By Gabriel Staples 
Website: http://www.ElectricRCAircraftGuy.com
My contact info is available by clicking the "Contact Me" tab at the top of my website.
Written: 1 Oct. 2015
Last Updated: 1 Oct. 2015
*/

// comment out to suppress serial port debugging
#define DEBUG 1

#include <SevSeg.h>

SevSeg sevseg; 

const int buttonPin = 12;   
const int ledPin = LED_BUILTIN;      

// int ledState = HIGH;         
int buttonState;             
int lastButtonState = HIGH;  
bool state_changed, turbo_button;

/*
  Turbo switch,  Asuka 486

  Fast speed (66MHz), turbo switch IN, LED ON
  Slow speed (25MHz), turbo switch OUT, LED OFF

  Turbo LED black wire on D12, pass - thru to front panel
  D12 has pull-down resistor to GND

  reads LOW when Turbo ON  (LED is ON)
        HIGH when Turbo OFF (LED is off)
*/

unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 50;    

#define STARTUP_STATE 0
#define START_FAST_STATE 1
#define FAST_STATE 2
#define START_SLOW_STATE 3
#define SLOW_STATE 4

uint8_t state;

// Bit-segment mapping:  0bHGFEDCBA
//      Visual mapping:
//                        AAAA          0000
//                       F    B        5    1
//                       F    B        5    1
//                        GGGG          6666
//                       E    C        4    2
//                       E    C        4    2        (Segment H is often called
//                        DDDD  H       3333  7      DP, for Decimal Point)
//                     Digit 0      Digit 1
uint8_t num;
int segcount = 0;
int digit = 2;  // 1=msb, 2=lsb

#define DISPLAY_WAIT_INTERVAL 3000
#define SLOW_ANIMATE_INTERVAL 150
#define FAST_ANIMATE_INTERVAL 50
unsigned long last_disp_millis, last_anim_millis;

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  byte numDigits = 2;   
  byte digitPins[] = {11, 2}; //Digits: 1,2,3,4 <--put one resistor (ex: 220 Ohms, or 330 Ohms, etc, on each digit pin)
  byte segmentPins[] = {3, 4, 5, 6, 7, 8, 9, 10}; //Segments: A,B,C,D,E,F,G,Period
//                      a  b  c  d  e  f  g   h
  sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(10); //Dev Note: 100 brightness simply corresponds to a delay of 2000us after lighting each segment. A brightness of 0 
                            //is a delay of 1us; it doesn't really affect brightness as much as it affects update rate (frequency).
                            //Therefore, for a 4-digit 7-segment + pd, COMMON_ANODE display, the max update rate for a "brightness" of 100 is 1/(2000us*8) = 62.5Hz.
                            //I am choosing a "brightness" of 10 because it increases the max update rate to approx. 1/(200us*8) = 625Hz.
                            //This is preferable, as it decreases aliasing when recording the display with a video camera....I think.

  
  lastButtonState = digitalRead(buttonPin);
  turbo_button = (lastButtonState == LOW);  // Read the button once on startup
  digitalWrite(ledPin, turbo_button);
  state_changed = false;
  state = STARTUP_STATE;    
  last_disp_millis=millis();
}

void loop() {

  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) lastDebounceTime = millis();  // reset the debouncing timer
  // debounce routine courtesy Limor Fried
  // ...use of interrupts for debouncing was not found to be reliable for some reason.
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      state_changed = true;
      }
    }
  
  lastButtonState = reading;  // save the reading. Next time through the loop, it'll be the lastButtonState:

  if (state_changed) {
    turbo_button = (lastButtonState == LOW);
    digitalWrite(ledPin, turbo_button);  // button LOW is Turbo ON
    #ifdef DEBUG
    if (turbo_button)
      Serial.println("Turbo ON");
    else
      Serial.println("Turbo OFF");
    #endif
  }

  switch(state) {
    ///////////////////////////////////////////
    case STARTUP_STATE:  // display '88' (all segments on, except DP)
      sevseg.setNumber(88, 0);
      if ((millis() - last_disp_millis) > DISPLAY_WAIT_INTERVAL) {
        if (turbo_button)  
          state = START_FAST_STATE; 
        else
          state = START_SLOW_STATE;
        segcount = 0;  // set up animation variables
        digit = 1; 
        last_disp_millis = millis();  // reset display delay var
        last_anim_millis = millis();
      }
      break;
    case START_FAST_STATE:  // fast animation
      if ((millis() - last_anim_millis) > FAST_ANIMATE_INTERVAL) {
        if ((digit == 1) && (num == 8)) {
          sevseg.setSegmentsDigit(digit, 0);  
          digit = 0;
        }
        else if ((digit == 0) && (num == 1)) {
          sevseg.setSegmentsDigit(digit, 0); 
          digit = 1;
        }
        else {
          num = 1 << segcount;
          if (segcount == 5) 
            segcount = 0;
          else
            segcount++;
        }
        sevseg.setSegmentsDigit(digit, num);  
        last_anim_millis = millis();
      }
      if ((millis() - last_disp_millis) > DISPLAY_WAIT_INTERVAL) {
        state = FAST_STATE;
        last_disp_millis = millis();
        #ifdef DEBUG
        Serial.println("FAST");
        #endif
      }
      if (state_changed) {  // during animation
        state = START_SLOW_STATE;
        last_disp_millis = millis();  // reset display delay var
        last_anim_millis = millis();
      }
      break;
    case FAST_STATE:  // display '66' 
      sevseg.setNumber(66, 0);
      if (state_changed) {
        state = START_SLOW_STATE;
        last_disp_millis = millis();  // reset display delay var
        last_anim_millis = millis();
      }
      break;
    case START_SLOW_STATE:  // slow animation
      if ((millis() - last_anim_millis) > SLOW_ANIMATE_INTERVAL) {
        if ((digit == 1) && (num == 8)) {
          sevseg.setSegmentsDigit(digit, 0);  
          digit = 0;
        }
        else if ((digit == 0) && (num == 1)) {
          sevseg.setSegmentsDigit(digit, 0); 
          digit = 1;
        }
        else {
          num = 1 << segcount;
          if (segcount == 5) 
            segcount = 0;
          else
            segcount++;
        }
        sevseg.setSegmentsDigit(digit, num);  
        last_anim_millis = millis();
      }
      if ((millis() - last_disp_millis) > DISPLAY_WAIT_INTERVAL) {
        state = SLOW_STATE;
        last_disp_millis = millis();
        #ifdef DEBUG
        Serial.println("SLOW");
        #endif
      }
      if (state_changed) {  // during animation
        state = START_FAST_STATE;
        last_disp_millis = millis();  // reset display delay var
        last_anim_millis = millis();
      }
      break;
    case SLOW_STATE:  // display '25' 
      sevseg.setNumber(25, 0);
      if (state_changed) {
        state = START_FAST_STATE;
        last_disp_millis = millis();  // reset display delay var
        last_anim_millis = millis();
      }
      break;
  } // switch
  if (state_changed)
    state_changed = false;
  sevseg.refreshDisplay();  // Must run repeatedly; don't use blocking code (ex: delay()) in the loop() function or this won't work right
}
    
