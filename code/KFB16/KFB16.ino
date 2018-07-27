#include "MIDIUSB.h"
#include <Multiplexer4067.h> // Multiplexer CD4067 library >> https://github.com/sumotoy/Multiplexer4067

///////////////////////////////////////////
// Multiplexer
#define nMux 2 // number of multiplexers
#define s0 2
#define s1 3
#define s2 5
#define s3 7
#define x1 A6 // analog pin of the first mux
#define x2 A7 /// analog pin of the x mux...
// Initializes the multiplexer
Multiplexer4067 mux[nMux] = {
  Multiplexer4067(s0, s1, s2, s3, x1),
  Multiplexer4067(s0, s1, s2, s3, x2)
};
// Mux pins order:
// 7,6,5,4,3,2,1,0,8, 9,10,11,12,13,14,15 // Mux
// 5,1,6,2,3,7,4,8,9,13,10,14,15,11,16,12 // Pots

/////////////////////////////////////////////
// buttons
const int NButtons = 0;
int buttonCState[NButtons] = {};         // stores the button current value
int buttonPState[NButtons] = {};        // stores the button previous value
//byte pin13index = 3; // put the index of the pin 13 in the buttonPin[] if you are using it, if not, comment lines 68-70

/////////////////////////////////////////////
// debounce
unsigned long lastDebounceTime[NButtons] = {};  // the last time the output pin was toggled
unsigned long debounceDelay = 5;    //* the debounce time; increase if the output flickers

/////////////////////////////////////////////
// potentiometers
const int nPots = 16; // total numbers of pots (slide & rotary)
const int nPotsPerBoard[nMux] = {8, 8}; // number of pots in each board (in order)
const int potPin[nPots] = {7, 5, 2, 0, 9, 11, 12, 14, 7, 5, 2, 0, 9, 11, 12, 14}; //pin of each component of each board in order
int nPotsPerBoardSum = 0;

int potCState[nPots] = {0}; // Current state of the pot
int potPState[nPots] = {0}; // Previous state of the pot
int potVar = 0; // Difference between the current and previous state of the pot

int midiCState[nPots] = {0}; // Current state of the midi value
int midiPState[nPots] = {0}; // Previous state of the midi value

int TIMEOUT = 300; //* Amount of time the potentiometer will be read after it exceeds the varThreshold
int varThreshold = 10; //* Threshold for the potentiometer signal variation
boolean potMoving = true; // If the potentiometer is moving
unsigned long PTime[nPots] = {0}; // Previously stored time
unsigned long timer[nPots] = {0}; // Stores the time that has elapsed since the timer was reset

/////////////////////////////////////////////

byte midiCh = 1; //* MIDI channel to be used
byte note = 36; //* Lowest note to be used
byte cc = 1; //* Lowest MIDI CC to be used

void setup() {

//  Serial.begin(9600); // use if using with ATmega328 (uno, mega, nano...)
//  while (!Serial) {
//  }

  for (int i = 0; i < nMux; i++) {
    mux[i].begin();
  }

}

void loop() {

  //buttons();
  potentiometers();


}

/////////////////////////////////////////////
// POTENTIOMETERS
void potentiometers() {
  
  //reads all the pots of all the boards and stores in potCState
  nPotsPerBoardSum = 0;
  for (int j = 0; j < nMux; j++) {
    for (int i = 0; i < nPotsPerBoard[j]; i++) {
      potCState[i + nPotsPerBoardSum] = mux[j].readChannel(potPin[i + nPotsPerBoardSum]);
    }
    nPotsPerBoardSum += nPotsPerBoard[j];
  }
  //  //Debug only
  //  for (int i = 0; i < nPots; i++) {
  //    Serial.print(potCState[i]); Serial.print(" ");
  //  }
  //  Serial.println();

  for (int i = 0; i < nPots; i++) { // Loops through all the potentiometers

    midiCState[i] = map(potCState[i], 0, 1023, 127, 0); // Maps the reading of the potCState to a value usable in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis(); // Stores the previous time
    }

    timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        // use if using with ATmega328 (uno, mega, nano...)
        //do usbMIDI.sendControlChange if using with Teensy
        //MIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel

        //use if using with ATmega32U4 (micro, pro micro, leonardo...)
        controlChange(midiCh, cc + i, midiCState[i]); //  (channel, CC number,  CC value)
        MidiUSB.flush();

        //Serial.println(midiCState[i]);
        potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }

}

// use if using with ATmega32U4 (micro, pro micro, leonardo...)

/////////////////////////////////////////////
// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}



