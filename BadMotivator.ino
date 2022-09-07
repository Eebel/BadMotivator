// Tim Hebel's Bad Motivator sequence
// V.03
// 7 Sep 2022

//****************LOG********************
//V0.02 added a time value for the Servo to move.  This allows you to move the time the servo triggers in the sequence
//V0.01 Initial release
// This code is not guaranteed.  Use at your own risk,
// *********  WARNING WARNING WARNING    *********
//    If you don't use OHM's Law to change the voltage to an appropriate value
//    you can start a FIRE. 
// *********  WARNING WARNING WARNING    *********

//  This sketch allow you to trigger from an external source such as an Arduino
//  or anything that can connect to the trigger pin and groun on the Arduino Nano
//  Alternativley, it provides for benchtop testing and tuning with manual buttons
//  mounted to the bad motivator.

#include <Arduino.h>
#include "NeoPatterns.h"
#include <ezButton.h>
#include "ServoEasing.hpp"

#define DEBUG  //Uncomment this line to see debug information output from the Serial Monitor on your computer


void PatternHandler(NeoPatterns *aLedsPtr);  //Function for controlling lighting behavior
ServoEasing Servo1;  //define a servo object using ServoEasing library

// Arduino Pin Assignments
const int  PIN_RESET       =    4; //resets Bad motivator logic and slowly closes the bad motivator
const int  PIN_SERVO       =    5; //controls the servo for opening and closing
const int  PIN_NEOPIXEL    =    6; //sends the signal to the 8-NeoPixel Ring
const int  PIN_TRIGGER     =    7; //looks for an external trigger signal on this pin
const int  PIN_PUMPRELAY   =    8; //Pin to turn the air pump relay on
const int  PIN_SMOKERELAY  =    9; //Pin to turn the vape coil relay on
const int  PIN_TESTPUMP    =   10; //powers the pump and runs the servo for rigging
const int  PIN_TESTSMOKE   =   11; //powers the pump and coil in a toggle fasion to tune the smoke
const int  PIN_TESTALL     =   12; //runs the entire sequence similating a signal on the PIN_TRIGGER

//Sequence times
//You can ajust these times to alter sequence timing here.
//These are elapsed times and are cumulative.  So, the OFF values of Pump and Smoke need to be greater than the ON values
//If you want the pump before smoke alter the TIME_PUMP_ON elapsed time to 0 and the TIME_SMOKE_ON to say 750
//If you want pump and smoke at the same time make both TIME_PUMP_ON and TIME_SMOKE_ON to 0
//1000ms = 1 second
const int TIME_SMOKE_ON   = 0;  //at 0 the smoke coils starts heating.  I found I needed to heat slightly before airflow
const int TIME_PUMP_ON    = 750;  // .75 seconds later, the air pump comes on
const int TIME_SERVO_MOVE = 1250; // .5 seconds after air pump starts
const int TIME_SMOKE_OFF  = 3750;  //3.75 seconds later, the smoke turns off
const int TIME_PUMP_OFF   = 6750;  //6.75 seconds later, the pump turn off to make sure the tubes are clear of vapor



//------Global Variables-----------
//unsigned long currentMillis = millis();
unsigned long pumpOnMillis = 0; // Time to sequence the Air Pump On
unsigned long pumpOffMillis = 0; //Tim to sequence the Air Pump Off
unsigned long smokeOnMillis = 0; //Time to sequence the Smoke ON
unsigned long smokeOffMillis = 0; //Time to sequence the Smoke Off
unsigned long servoOpenMillis = 0; //Time to sequence the servo to opent the bad motivator.

bool isTriggered = false; //was trigger sent? Then, Start smoke sequence
bool wasTriggered = false; //smoke sequence is active, don't trigger again until complete
bool updatePixels = false;  //flag for NeoPixelRing updating.

int servoClosed = 158;//degrees of servo when closed
int servoOpen   = 30;//degrees of servo weh open

ezButton buttonReset(PIN_RESET);        //Push this button to lower the bad motivator, turn off lights and reset logic
ezButton buttonTestPump(PIN_TESTPUMP);  // create ezButton object that attaches to PIN_TESTPUMP to test pump/servo range
ezButton buttonTestSmoke(PIN_TESTSMOKE);  //create ezButton object that attaches to PIN_TESTSMOKE to test smoke and air
ezButton buttonTestAll(PIN_TESTALL);  // create ezButton object that attaches to PIN_TESTALL to test the whole sequence

bool statePumpButton =  false;   //Keep track of whether if the button was pressed for sequence logic
bool stateSmokeButton = false;   //Keep track of whether if the button was pressed for sequence logic
bool stateTestAllButton = false; //Keep track of whether if the button was pressed for sequence logic
bool buttonLock = false;  //lock buttons so we don't get tangled commands

NeoPatterns ring1 = NeoPatterns(8, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800, &PatternHandler);


void setup() {
  // put your setup code here, to run once:
  //----PinModes---------------------
pinMode(PIN_NEOPIXEL, OUTPUT);       //NeoPixel Controller, set as OUTPUT to control the pixels
pinMode(PIN_TRIGGER, INPUT_PULLUP);  //The external Trigger to launch the Bad motivator sequence
pinMode(PIN_PUMPRELAY, OUTPUT);      //Fire the AirPump
digitalWrite(PIN_PUMPRELAY, LOW);    //make sure the pump relay is off
pinMode(PIN_SMOKERELAY, OUTPUT);     //Fire the vape coil heater
digitalWrite(PIN_SMOKERELAY, LOW);   //Make sure the smoke relay is off

//Setup ServoEasing Object to control the servo
    if (Servo1.attach(PIN_SERVO, servoClosed) == INVALID_SERVO) {
        Serial.println(F("Error attaching servo"));
    }
//Start serial commincations
  Serial.begin(115200);

  ring1.begin();  //Start the NeoPixel ring


  ring1.PixelFlags |= PIXEL_FLAG_GEOMETRY_CIRCLE;  //So NEOPATTERNS library knows how to sequence the lights
  delay(300); // to avoid partial patterns at power up

  ring1.ColorWipe(COLOR32_PURPLE, 50); //If you see a single purple light flash you know the setup is done.
  delay(500);
  ring1.ColorWipe(0,0,0);  //turn the lights off

//Button setup...debounce is how many ms are needed to be sure the button is not accidently triggering twice or more.
buttonReset.setDebounceTime(50);
buttonTestPump.setDebounceTime(50);
buttonTestSmoke.setDebounceTime(50);
buttonTestAll.setDebounceTime(50);

  //This text is a joke.  Everything is working and we are ready to go!
  Serial.println("Motivation level is BAD! Failure imminent..");
  isTriggered = digitalRead(PIN_TRIGGER);
    Serial.println("PIN_TRIGGER State Read as:");
    Serial.println(isTriggered);
}

void loop() {
  //Check for button presses 
  buttonReset.loop();
  buttonTestPump.loop();
  buttonTestSmoke.loop();
  buttonTestAll.loop();


  if(buttonReset.isReleased()){
    //RESET all the logic and lower the motivator slowly
    Servo1.easeTo(servoClosed,45); //close at 45/sec
    updatePixels = false; //stop updates to pixels
    ring1.ColorWipe(0,0,0);  //turn NeoPixel ring off;    
    #ifdef DEBUG
      Serial.println("Button Reset Pushed");
    #endif
  }
  //Run pump only-toggle
  //Also run servo limits
  if(buttonTestPump.isReleased()){
     //Toggle the Button state      
    statePumpButton = (!statePumpButton); //sketch starts with statePumpButton = false
    digitalWrite(PIN_PUMPRELAY, statePumpButton); //toggle pump relay
    switch (statePumpButton){
      case true:
         //digitalWrite(PIN_PUMPRELAY, statePumpButton);
         //Open Servo
         Servo1.setEasingType(EASE_BOUNCE_OUT);      
         Servo1.easeTo(servoOpen,360);//was 360
         Servo1.setEasingType(EASE_LINEAR);

        #ifdef DEBUG
          Serial.println("buttonTestPump Pushed to Open Servo");
          Serial.print(" statePumpButton: ");
          Serial.println (statePumpButton);
          Serial.print(" servoOpen Value: ");
          Serial.println (servoOpen);          
        #endif   
        break;
      case false:
          //Close Servo 
          Servo1.easeTo(servoClosed,45);

          #ifdef DEBUG
            Serial.println("buttonTestPump Pushed to close Servo");
            Serial.print("statePumpButton: ");
            Serial.println(statePumpButton);
            Serial.print(" servoClosed Value: ");
            Serial.println (servoClosed);             
          #endif       
        break;
    }
  }
  //run Smoke..but turnn pump on
  if(buttonTestSmoke.isReleased()){
    //Use this to test how long it takes to get good smoke.  Don't go too long
    //we have plastic parts!
    stateSmokeButton = !stateSmokeButton;
    //digitalWrite(PIN_PUMPRELAY, stateSmokeButton);//turn pump ON
    digitalWrite(PIN_SMOKERELAY, stateSmokeButton);  //turn on smoke
    #ifdef DEBUG
      Serial.println("buttonTestSmoke Pushed");
      Serial.print("stateSmokeButton: ");
      Serial.println(stateSmokeButton);
    #endif
  }
  //Run Entire sequence
  if(buttonTestAll.isReleased()){
    //stateTestAllButton = !stateTestAllButton;
    #ifdef DEBUG
      Serial.println("buttonTestAll Pushed");
      Serial.print("stateTestAllButton: ");
      Serial.println(stateTestAllButton);
    #endif
    isTriggered = true;
  }
 
 if(digitalRead(PIN_TRIGGER) == HIGH){
   //Trigger signal from user detected
  isTriggered = true;//digitalRead(PIN_TRIGGER);
      #ifdef DEBUG
        Serial.println("PIN_TRIGGER State Read as:");
        //Serial.print("stateTestAllButton: ");
        Serial.println(isTriggered);
      #endif

 }


  
  if(isTriggered && !wasTriggered){
    //start smoke sequence only if isTriggered is true and wasTriggered is false
    isTriggered = false;
    //set wasTriggered to true to keep it from firing twice without finishing.
    wasTriggered = true; //Set the sequence active flag to true
   //Store the times of the events for easy reference
    smokeOnMillis = millis() + TIME_SMOKE_ON; //Sequence Elapsed time = 0ms
    pumpOnMillis = millis()+ TIME_PUMP_ON; //Sequence Elapsed time  = 750ms
    servoOpenMillis = millis()+ TIME_SERVO_MOVE; //Sequence Elapsed time = 1250 ms
    smokeOffMillis = millis() + TIME_SMOKE_OFF;//Sequence Elapsed time  = 3750ms
    pumpOffMillis = millis() + TIME_PUMP_OFF;//Sequence Elapsed time  = 6750ms
    //Turn airpump ON if user wants airflow first
    // if (TIME_PUMP_BEFORE_SMOKE > 0){
    //   digitalWrite(PIN_PUMPRELAY, HIGH);
    // }
    // else{//turn on smoke and no air
    //   digitalWrite(PIN_SMOKERELAY, HIGH);
    // }
    //Open the Motivator  
    //Servo1.easeTo(servoOpen,360);
    
    #ifdef DEBUG
    Serial.println("isTriggered was true;");
    #endif
  }
  if (wasTriggered){
    updatePixels = true;
    if(millis()>pumpOffMillis){ //should run this last
      digitalWrite(PIN_PUMPRELAY, LOW);  //Pump OFF and reset flags for a subsequent trigger
      isTriggered = false; //set this false again in case we got a second command while running.
      wasTriggered = false;
      #ifdef DEBUG
        //Serial.println("Smoke Sequence complete;");
      #endif
    }
    else if(millis()> smokeOffMillis){//should run this third
      digitalWrite(PIN_SMOKERELAY,LOW); //Smoke OFF
      #ifdef DEBUG
        //Serial.println("Smoke ON;");
      #endif
    }
    else if(millis()> servoOpenMillis){//This can run at anytime base on yout value.  Here it is slightly after smoke starts.
      Servo1.easeTo(servoOpen, 360);
    }
    else if(millis()> pumpOnMillis){//should run this second
      digitalWrite(PIN_PUMPRELAY, HIGH); //Smoke ON
      #ifdef DEBUG
        Serial.println("Sequence started, Pump ON;");
      #endif     
    }
    else if(millis()> smokeOnMillis){//should run this first
      digitalWrite(PIN_SMOKERELAY, HIGH); //Smoke ON
      #ifdef DEBUG
        Serial.println("Sequence started, Smoke ON;");
      #endif     
    }
  }    
if(updatePixels){
  ring1.update();  
}  
    // Servo1.update();
    // if(!Servo1.isMoving()){
    //   Servo1.detach();
    // }
}

void PatternHandler(NeoPatterns *aLedsPtr) {
      static int8_t sState = 0;

        aLedsPtr->Fire(10, 100); // OK Fire(30, 260)is also OK

}