#include <Adafruit_NeoPixel.h>
#include <MeetAndroid.h>
#include <Timer.h>
#include <Wire.h>
#include "Adafruit_SI1145.h"

//---Notifications---------------
#define CALL 20
#define SMS 3 // Red
#define GMAIL 4 // Blue
#define SNAPCHAT 5 // Yellow
#define NOTIF6 6 // Cyan
#define NOTIF7 7 // Green
#define NOTIF8 8 // Magenta
#define DISPLAYON 0
int State = 0;  // Notification #
//--------------------------------

//---Arduino input/output pins----
int buttonPin = 6; 
int onboardLed = 13;
//--------------------------------

//---Clock Variables--------------
int Minute = 0;
int LastMinute = 0;
Timer t; //stopwatch and uv sensor timer
//Timer b; //brightness loop timer
int seconds = 0;
int timerEvent;
int uvTimer;
int secNumMin;
int secNumSec;
//--------------------------------

//---Button Reading Variables------
int buttonState;
int lastButtonState = HIGH;
long buttonPush = 0;
int Mode = 0;
int lastMode;
//---------------------------------
int StblzdBrtns;
int brightness[5];
int Brtns = 50; // NeoPixel Brightness (0-255)
int lastBrtns;
int BrtnsMode = 0; //Brightness Mode = Auto

Adafruit_SI1145 uv = Adafruit_SI1145();
MeetAndroid meetAndroid;
#define PIN 12 //Neopixel pin
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

void setup()  
{
  Serial.begin(9600); 

  // register callback functions, which will be called when an associated event occurs.
  meetAndroid.registerFunction(timeValues, 'B'); 
  meetAndroid.registerFunction(phoneState, 'A');
  pinMode(onboardLed, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(onboardLed, HIGH);
  State = 10;
  strip.begin();
  strip.show();
  colorWipeBack(strip.Color(255, 0, 0), 50);
  colorWipeBack(strip.Color(0, 255, 0), 50);
  colorWipeBack(strip.Color(0, 0, 255), 50);
  stopLight();
  t.every(2000, Brightness);
  uv.begin();
}

void loop()
{
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) { //If button was just pressed
    buttonPressed();
  }
  if (buttonState == HIGH && lastButtonState == LOW){ //If button was just released
    buttonReleased();
  }
  meetAndroid.receive(); // keep in loop() to receive events
  t.update(); //keep in loop() to update timer
}
//****************************
// Button Press Reading
//****************************

void buttonPressed(){
  buttonPush = millis(); // set button hold down time
  lastButtonState = buttonState; 
}

void buttonReleased(){
  long holdTime = millis() - buttonPush; //how long the button was held
  if (holdTime > 500 && holdTime < 2000){
    switch (Mode) //Long press action
    {
    case 0: 
      seconds = 0;
      colorWipeBack(strip.Color(255, 0, 0), 50);
      timerEvent = t.every(1000, timer); 
      Mode = 1; 
      break; //Start timer when switching to timer mode
    case 1: 
      t.stop(timerEvent);
      colorWipeBack(strip.Color(255, 0, 0), 50);
      uvTimer = t.every(1000, uvMeter); 
      Mode = 2; 
      break; //Stop timer when switching to mode 2 
    case 2: 
      t.stop(uvTimer);
      colorWipeBack(strip.Color(255, 0, 0), 50);
      stopLight(); 
      Mode = 3; 
      break;
    case 3:
      colorWipeBack(strip.Color(255, 0, 0), 50);
      LastMinute = Minute - 1;
      Mode = 0; 
      break; //Go back to clock mode
    case 10: 
      seconds = 0;
      colorWipeBack(strip.Color(255, 0, 0), 50);
      timerEvent = t.every(1000, timer); 
      Mode = 1;
      break;
    case 11:
      colorWipeBack(strip.Color(0, 0, 255), 50);   
      seconds = 0; 
      break; //Reset stopwatch when it is paused
    case 12: //Mode = 3; 
      break;
    case 13: 
      colorWipeBack(strip.Color(255, 0, 0), 50);
      LastMinute = Minute - 1;    
      Mode = 0;
      break; //Go back to clock mode
    case 20: 
      seconds = 0;
      colorWipeBack(strip.Color(255, 0, 0), 50);
      timerEvent = t.every(1000, timer); 
      Mode = 1; 
      break;
    }
  }
  if (holdTime > 2000){ //Very long click controls brightness profile (Day, Night, Auto)
    switch (BrtnsMode) {
    case 0:
      Brtns = 10;
      if (Mode == 0){
        LastMinute = Minute - 1;
      }
      BrtnsMode = 1;
      break;
    case 1:
      Brtns = 40;
      if (Mode == 0){
        LastMinute = Minute - 1;
      }
      BrtnsMode = 2;
      break;
    case 2:
      if (Mode == 0){
        LastMinute = Minute - 1;
      }
      BrtnsMode = 0;
      break;
    }
  }
  if (holdTime > 50 && holdTime < 500) {
    switch (Mode) //Short press action that varies per mode
    {
    case 0: 
      Mode = 10; 
      break; //Go to min mode
    case 1: 
      t.stop(timerEvent); 
      Mode = 11; 
      break; //Pause stopwatch when its going
    case 2: // Mode = 12; 
      break;
    case 3: 
      flashlight(); 
      Mode = 13; 
      break; //Turn off flashlight when it's on
    case 10: 
      Mode = 20; 
      secNumMin = secNumMin - 1; //Redraw clock after clearing 
      break; //Go to second mode
    case 11: 
      timerEvent = t.every(1000, timer); 
      Mode = 1; 
      break; //Start timer when it's stopped
    case 12: //Mode = 2; 
      break;
    case 13: 
      stopLight(); 
      Mode = 3; 
      break; //Turn on flashlight when it's off
    case 20: 
      Mode = 0; 
      LastMinute = Minute - 1; //Redraw clock after clearing 
      break; //Return to hour/min mode
    }
  }

  meetAndroid.send("Mode is");
  meetAndroid.send(Mode);
  lastButtonState = buttonState;
}

//****************************
// Notification Handling Service
//****************************
void phoneState(byte flag, byte numOfValues)
{
  if (Mode == 0){ //only execute when in clock/notification mode
    // phone state
    State = meetAndroid.getInt();

    switch (State)
    {
    case CALL:
      call();
      State = 20;
      break;  
    case SMS: 
      sms(); 
      break;
    case GMAIL: 
      gmail(); 
      break;
    case SNAPCHAT: 
      snapchat(); 
      break;
    case NOTIF6: 
      notif6(); 
      break;
    case NOTIF7: 
      notif7(); 
      break;
    case NOTIF8: 
      notif8(); 
      break;
    }
  }
}

void call()
{
  theaterChase(strip.Color(0,0,127), 50);
}

void sms()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(255, 0, 0), 50);
    stopLightIncr(50);
  }
}

void gmail()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(0, 0, 255), 50);
    stopLightIncr(50);
  }
}

void snapchat()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(255, 255, 0), 50);
    stopLightIncr(50);
  }
}
void notif6()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(0, 255, 255), 50);
    stopLightIncr(50);
  }
}

void notif7()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(0, 255, 0), 50);
    stopLightIncr(50);
  }
}

void notif8()
{
  for (int i=0;i<3;i++){
    colorWipe(strip.Color(255, 0, 255), 50);
    stopLightIncr(50);
  }
}



//****************************
// Clock Service
//****************************

void timeValues(byte flag, byte numOfValues)
{
  int data[numOfValues];
  meetAndroid.getIntValues(data);

  if (Mode == 0 && State != 20)
  { //only execute when in clock/notification mode
    int data[numOfValues];
    meetAndroid.getIntValues(data);
    Minute = data[1];

    if (State != 0 && (data[2]%2)==0){  //If there's a pending notification, blink for 1sec
      delay(500);                        // on even seconds
      stopLight();                         
      delay(500);
      LastMinute = Minute - 1; //Redraw clock after clearing
    }

    for (int i=0; i<2;i++) //Loops 2x for h,m
    {
      if (Minute != LastMinute) //Only redraws clock when it is a new minute
      {
        stopLight();
        if (data[1]/5+4 > 11) {
          strip.setPixelColor(data[1]/5-8, 255, 0, 0);
        }
        else {
          strip.setPixelColor(data[1]/5+4, 255, 0, 0);
        }
        if (data[0]+4 > 11) {
          strip.setPixelColor(data[0]-8, 0, 0, 255);
        }
        else {
          strip.setPixelColor(data[0]+4, 0, 0, 255);
        }

        if (data[1]/5 == data[0]) {
          if (data[0]+4 > 11) {
            strip.setPixelColor(data[0]-8, 255, 0, 255);
          }
          else {
            strip.setPixelColor(data[0]+4, 255, 0, 255);
          }
        }
        strip.setBrightness(Brtns);
        strip.show();
        LastMinute = Minute;
      }
    }
  }
  // ---------------------------
  // Minute Mode --------------
  //----------------------------
  if (Mode == 10)
  {
    if (data[1] - (data[1]/10)*10 != secNumMin)
    {
      int firstNumMin = data[1]/10; //first digit for seconds
      secNumMin = data[1] - firstNumMin*10; //second digit for seconds
      stopLight();
      strip.setPixelColor(firstNumMin+4, 0, 0, 255);
      if (secNumMin+4 > 11) {
        strip.setPixelColor(secNumMin-8, 0, 255, 0);
      }
      else {
        strip.setPixelColor(secNumMin+4, 0, 255, 0);
      }
      if (firstNumMin == secNumMin) {
        strip.setPixelColor(firstNumMin+4, 0, 255, 255);
      }
      strip.setBrightness(Brtns);
      strip.show();
    }  
  }
  // ---------------------------
  // Seconds Mode --------------
  //----------------------------
  if (Mode == 20)
  {
    int firstNumSec = data[2]/10; //first digit fot seconds
    secNumSec = data[2] - firstNumSec*10; //second digit for seconds
    stopLight();
    strip.setPixelColor(firstNumSec+4, 0, 0, 255);
    if (secNumSec+4 > 11) {
      strip.setPixelColor(secNumSec-8, 255, 0, 0);
    }
    else {
      strip.setPixelColor(secNumSec+4, 255, 0, 0);
    }
    if (firstNumSec == secNumSec) {
      strip.setPixelColor(firstNumSec+4, 255, 0, 255);
    }
    strip.setBrightness(Brtns);
    strip.show();
  }
}
//****************************
// NEOPIXEL ANIMATIONS
//****************************
void stopLight() {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
    strip.show();
  }
}
void stopLightIncr(uint8_t wait) {
  for(uint16_t i=4; i<16; i++) {
    if (i > 11) {
      strip.setPixelColor(i-12, 0,0,0);
    }
    else {
      strip.setPixelColor(i, 0,0,0);
    }
    strip.show();
    delay(wait);
  }
}
void colorWipeBack(uint32_t c, uint8_t wait) {
  for(uint16_t i=4; i<16; i++) {
    if (i > 11) {
      strip.setPixelColor(i-12, c);
    }
    else {
      strip.setPixelColor(i, c);
    }
    strip.setBrightness(Brtns);
    strip.show();
    delay(wait);
  }
  for(uint16_t i=15; i>4; i--) {
    if (i > 11) {
      strip.setPixelColor(i-12, 0, 0, 0);
    }
    else {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
    delay(wait);
  }
}
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=4; i<16; i++) {
    if (i > 11) {
      strip.setPixelColor(i-12, c);
    }
    else {
      strip.setPixelColor(i, c);
    }
    strip.setBrightness(Brtns);
    strip.show();
    delay(wait);
  }
}
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//****************************
// Other Functions
//****************************
void flashlight(){

  for (uint16_t i=0; i<12; i++) {
    strip.setPixelColor(i*2, 255,255,255);
  }
  strip.setBrightness(255);
  strip.show();
}

void uvMeter(){
  float UVindex = uv.readUV();
  // the index is multiplied by 100 so /by 100
  UVindex /= 100.0;  
  int uvValWhole = UVindex;
  int uvValDec = (UVindex-uvValWhole)*10; 

  stopLight();
  if (uvValWhole+4 > 11) {
    strip.setPixelColor(uvValWhole-8, 255, 125, 0);
  }
  else {
    strip.setPixelColor(uvValWhole+4, 255, 125, 0);
  }
  if (uvValDec+4 > 11) {
    strip.setPixelColor(uvValDec-8, 0, 255, 125);
  }
  else {
    strip.setPixelColor(uvValDec+4, 0, 255, 125);
  }
  if (uvValDec == uvValWhole) {
    if (uvValWhole+4 > 11) {
      strip.setPixelColor(uvValWhole-8, 125, 255, 0);
    }
    else {
      strip.setPixelColor(uvValWhole+4, 125, 255, 0);
    }
  }
  strip.setBrightness(Brtns);
  strip.show();
}

void timer(){ //Whatever's in here will run every 1000ms
  if (seconds == 59) {
    seconds = 0;
  }
  else {
    seconds = seconds + 1;
  }
  int firstNum = seconds/10; //first number ie 24 -> 2 || Span: 0-5
  int secNum = seconds - firstNum*10; //second number ie 24 -> 4 || Span: 0-9
  stopLight();
  strip.setPixelColor(firstNum+4, 255, 255, 0);
  if (secNum+4 > 11) {
    strip.setPixelColor(secNum-8, 0, 255, 255);
  }
  else {
    strip.setPixelColor(secNum+4, 0, 255, 255);
  }
  if (firstNum == secNum) {
    strip.setPixelColor(firstNum+4, 0, 255, 0);
  }
  strip.setBrightness(Brtns);
  strip.show();
  meetAndroid.send("Timer is");
  meetAndroid.send(seconds);
}

void Brightness(){
  if (BrtnsMode == 0) { //Only execute when Brightness Mode is on Auto
    for (int i=0; i<4;i++){
      brightness[i] = uv.readIR();
    }
    StblzdBrtns = (brightness[0]+brightness[1]+brightness[2]+brightness[3])/4;
    meetAndroid.send(StblzdBrtns);
    if (StblzdBrtns < 255){
      Brtns = 10;
    }
    if (StblzdBrtns < 265 && StblzdBrtns > 254){
      Brtns = 25;
    }
    if (StblzdBrtns < 300 && StblzdBrtns > 264){
      Brtns = 40;
    }
    if (StblzdBrtns < 500 && StblzdBrtns > 299){
      Brtns = 115;
    }
    if (StblzdBrtns < 5000 && StblzdBrtns > 499){
      Brtns = 200;
    }
    if (Brtns != lastBrtns && Mode == 0){
      LastMinute = Minute - 1;
    }
    lastBrtns = Brtns;
  }
}







