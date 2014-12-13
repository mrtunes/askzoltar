// this is the arduino code for running the zoltar machine
// written by Tatiana Jennings and Elliott Fienberg, 2014

// Requires the Software Serial and Adafruit Thermal libraries to be installed.


// library references go here
#include "SoftwareSerial.h"
#include "Adafruit_Thermal.h"
#include <avr/pgmspace.h>

int printer_RX_Pin = 3;  // This is the green wire
int printer_TX_Pin = 4;  // This is the yellow wire
int incomingByte;
int state;
int buttonState = 0; // button to trigger the printer in an emergency

Adafruit_Thermal printer(printer_RX_Pin, printer_TX_Pin);

// global variable declarations go here

// these are commands being sent over to processing via USB and/or xbee
const int WAKE_UP = 1;
const int GO2SLEEP = 0;
const int BLANK = 2;
const int GO_BIG = 3;
const int RECEIPT = 4;
const int buttonPin = 7; 

// in this next session replace port numbers with correct ones

const int TOPLEDPIN1 = 6;        // pin that the 1st set of top lid LEDs is connected to
const int TOPLEDPIN2 = 13;        // pin that the ***2nd*** set of top lid LEDs is connected to
const int HANDLEDPIN = 9;        // pin that the HANDPRINT LED strip is connected to
const int PRINTLEDPIN = 4;       // pin that the Printer LED  is connected to
const int PROXSENSORPIN = A1;        // analog pin that the PROXIMITY sensor is connected to
const int PRESSURESENSORPIN = A0;    // pin that the PRESSURE sensor is connected to

// this section contains threshhold values for proximity and pressure sensors - find experimentally, replace values as needed

const int PROXTHRESHOLD = 100; 
const int PRESSTHRESHOLD = 1000; 


int timestamp = 0;
int timestamp2 = 0;
int timedelay = 20; // intial time out value for someone approaching the box
int timedelay2 = 0; // intial time out value for the second timer
int timer2 = 0; // this one is for the lightshow
boolean lightswitch = false;
int lightshowdelay = 500; // this is the cadense for top lid LEDS blinks, in milliseconds
boolean receiptprinted = false;
boolean triggered = false;
boolean pleaseplay = false;
boolean commandsent = false;


// one time setup routine
void setup() {
send2swami(BLANK);
timedelay = 5; // this is an initial value for proximity timeout
starttimer();


Serial.begin(9600);
// initialize all LED pins for output
pinMode(TOPLEDPIN1, OUTPUT);
pinMode(TOPLEDPIN2, OUTPUT);
pinMode(HANDLEDPIN, OUTPUT);
pinMode(PRINTLEDPIN, OUTPUT);
  pinMode(buttonPin, INPUT);   
}

// this simple function rapidly blinks both of two sets of top lid LEDs
void blinkLED() {
// simple blink of both sets of top lid LEDs, 100 milliseconds pause
digitalWrite(TOPLEDPIN1, HIGH);
digitalWrite(TOPLEDPIN2, HIGH);
delay(100);
digitalWrite(TOPLEDPIN1, LOW);
digitalWrite(TOPLEDPIN2, LOW);
}


// reads proximity sensor, returns TRUE if someone is near
boolean proximity() {
 float volts1 = analogRead(PROXSENSORPIN)*0.0048828125;   // value from sensor * (5/1024) - if running 3.3.volts then change 5 to 3.3
 float volts2 = analogRead(PROXSENSORPIN)*0.0048828125;   // value from sensor * (5/1024) - if running 3.3.volts then change 5 to 3.3
 float volts3 = analogRead(PROXSENSORPIN)*0.0048828125;   // value from sensor * (5/1024) - if running 3.3.volts then change 5 to 3.3
 float distance = 65*pow((volts1+volts2+volts3)/3, -1.10);          // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk

 if (distance < PROXTHRESHOLD) return true; else return false; 
}


boolean handpress() {
 // check if pressure sensor is triggered, return True
 // assuming here that more pressure reads as higher value, if opposite - reverse the conditional statement
 if (analogRead(PRESSURESENSORPIN) > PRESSTHRESHOLD) return true; else return false; 
}


void send2swami(int command) {
 // sends commands to play wake-up sequence, and the others
 // the only command that hasd to be send via xbee is GO_BIG
 // all other commands are send via serial usb port
 if (command == GO_BIG) {
   // use xbee
   Serial.println(command);
 } else {
   // use serial port
   Serial.println(command);
 }
}


// simple routine for turning printer LED ON and OFF, as needed
void handprintLED(boolean command) {
 // turn LED for the handprint ON or OFF depending on argument
 if (command) {
   // turn handprint LED on
   digitalWrite(HANDLEDPIN, HIGH);
 } else {
   // turn handprint LED off
   digitalWrite(HANDLEDPIN, LOW);

 }
}

void starttimer() {
 // this function, along with "timeout" boolean variable and "timedelay" int variable, manages various delays and timers
 // it's being used in many places, so be careful
 timestamp = millis()/1000 + timedelay;
}

void starttimer2() {
 // this function, along with "timeout" boolean variable and "timedelay" int variable, manages various delays and timers
 // it's being used in many places, so be careful
 timestamp2 = millis()/1000 + timedelay2;
}


boolean timeout() {
 // return True if X number of seconds passed since starttimer() had been invoked
 if (millis()/1000 > timestamp) return true; else return false;
}

boolean timeout2() {
 // return True if X number of seconds passed since starttimer() had been invoked
 if (millis()/1000 > timestamp2) return true; else return false;
}


// this routine actually talks to LEDs (two sets) depending on a flag
// might roll it in later
void flickLEDs() {
   if (lightswitch) {
     digitalWrite(TOPLEDPIN1, HIGH);
     digitalWrite(TOPLEDPIN2, LOW);
   } else {
     digitalWrite(TOPLEDPIN2, HIGH);
     digitalWrite(TOPLEDPIN1, LOW);
   }
}


// these following three subroutines flick the LEDs (2 sets) on and off, in alternating fashion
// first one starts the show
void lightshowstart() {
 // starts the lightshow (two alternate blinking sets of LEDs in top lid)
 lightswitch = false;
 timer2 = millis()+lightshowdelay;
 flickLEDs(); // this is riding on a global boolean flag var lightswitch, so don't touch it
 pleaseplay = true;
}

// this one flips between two sets of LEDs, based on timer
void lightshowGO() {
 // this routine blinks LEDs on top of the box, alternatively
 if (millis()>timer2) {
   lightswitch=!lightswitch;
   timer2=millis()+lightshowdelay;
   if (pleaseplay) flickLEDs();
 }
}

// and this one shuts everything down
void lightshowstop() {
 // stops the lightshow
 timer2 = 0;
 // and turn LEDs off too. OFF!
 digitalWrite(TOPLEDPIN2, LOW);
 digitalWrite(TOPLEDPIN1, LOW);
 pleaseplay = false;
}


void printreceipt() {
 int rnd_index = int(random(6))+1; // random number 1 to 6

 // turn LED for printer and print the receipt, then turn led off
 digitalWrite(PRINTLEDPIN, HIGH);
 // the code that actually prints receipts goes here
 // like this (or something like this):
 // print(myStrings[rnd_index];
 digitalWrite(PRINTLEDPIN, LOW);
}

//this function controls what gets sent to the printer 

void printerStart(){
  
   printer.begin(50);
  printer.setTimes(35000, 2100);
   
   printer.doubleHeightOff();
   printer.inverseOff();
 printer.feed(1);
 
 
 printer.println("Thank you for asking Zoltar"); 
 printer.feed(1);
  printer.println("On December 8, 2014"); 
  
  printer.feed(1);
   
    printer.println("After pondering your very important question and searching for an answer in the deep divine then carefully considering the circumstances  of your past, present and future Zoltar reluctantly offers his wisdom:"); 
printer.feed(1);
printer.println("My sources say no."); 
 
printer.feed(1);
 


  printer.feed(3);

  printer.sleep();      // Tell printer to sleep
  printer.wake();       // MUST call wake() before printing again, even if reset
  printer.setDefault(); // Restore printer to defaults

}


//main loop
void loop() {
  
  
    
  
  /*if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    // if it's a capital H (ASCII 72), turn on the LED:
    if (incomingByte == 'H') {
      printerStart();
    } 
    // if it's an L (ASCII 76) turn off the LED:
    if (incomingByte == 'L') {
     // digitalWrite(ledPin, LOW);
    }
  }*/
  
  
 // idle state
 // every 20 seconds blink one of two box LED sets
 timedelay = 2; // delays between blinks in seconds. Change as needed
 if (timeout()) {
   blinkLED();
   starttimer();
   Serial.println("idle"); // remove
   

   
    

 }
 // this is the end of idle cycle. All it does is simple blink of LEDs every 10 sec

 if(proximity()) {    // this is where action starts, when proximity sensor detects someone
   Serial.println("proximity"); // remove

   send2swami(GO2SLEEP); // we start playing Swami sleeping
   handprintLED(true); // turn on Handprrint LED
   timedelay = 3000; // setting timeout to 30 sec - this is a total time for swami videos
   starttimer(); // start the countdown. If noone pushes hand sensor in next 25 sec we reset everything
   Serial.println("waiting for handpress"); // remove
   triggered=false;
   commandsent= false;
   lightshowstart(); // blinkey lights start


   while (!timeout() && proximity()) {
     if (!triggered) triggered = handpress();

     if (triggered) {  // they pushed the handsensor! set the timer and WAKE UP SWAMI!!!!
//          Serial.println("Handpress!!!!"); // remove

         lightshowGO(); // while swami video is running, with nothing to do we just play blinky lights

         timedelay2 = 93000; // IMPORTANT!!! set the correct delay here - length of "Swami wakes up" video - was 43 before
         if (!commandsent) {
           send2swami(WAKE_UP);
           starttimer2();

         }
         commandsent = true;
//          Serial.println("swami wakes up playing"); // remove


         if (timeout2()) { // meaning, first video stopped playing
           send2swami(GO_BIG); // this command goes via xbee to another processing unit.
           Serial.println("swami BIG is playing"); // remove
           timedelay2 = 93000; // IMPORTANT!!! set the correct delay here - length of "Swami wakes up" video
           starttimer2();
         }  

     }
   }

   // reset back to initial state
   lightshowstop(); // blinkey lights off! otherwise they might not see printer LED
   if (proximity() && !receiptprinted && triggered) {
     printreceipt();
     Serial.println("RECEIPT printed"); // remove

     send2swami(RECEIPT);
     receiptprinted = true; 
   }
   send2swami(BLANK);
   lightshowstop();
   receiptprinted = false;
   // and flick the LED couple of times
   handprintLED(true);
   delay(100);
   handprintLED(false);
   delay(100);
   handprintLED(true);
   delay(100);
   handprintLED(false);
 }
 
   // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) {     
    // turn LED on:    
    printerStart(); 
  } 
  else {
    // turn LED off:
    //digitalWrite(ledPin, LOW); 
  }
 
  /* this section would be responsible for receiving the xbee message from the indoor computer and tell the printer to start
  
  if (Serial.available() > 0){
   if (Serial.peek() == 'L'){
   Serial.read();
   state = Serial.parseInt();
   printerStart();
   }
   while (Serial.available() > 0) {
   Serial.read();
   }
   }*/
}
