// AC Control V1.1
//
// This Arduino sketch is for use with the heater
// control circuit board which includes a zero
// crossing detect function and an opto-isolated TRIAC.
//
// AC Phase control is accomplished using the internal
// hardware timer1 in the Arduino
//
// Timing Sequence
// * timer is set up but disabled
// * zero crossing detected on pin 2
// * timer starts counting from zero
// * comparator set to "delay to on" value
// * counter reaches comparator value
// * comparator ISR turns on TRIAC gate
// * counter set to overflow - pulse width
// * counter reaches overflow
// * overflow ISR turns off TRIAC gate
// * TRIAC stops conducting at next zero cross


// The hardware timer runs at 16MHz. Using a
// divide by 256 on the counter each count is
// 16 microseconds.  1/2 wave of a 60Hz AC signal
// is about 520 counts (8,333 microseconds).


#include <avr/io.h>
#include <avr/interrupt.h>
#include <MIDI.h>

#define DETECT 2  //zero cross detect
#define GATE 9    //TRIAC gate
#define PULSE 4   //trigger pulse width (counts)
int i = 400;
int i_targ = 400;
int trash = 0;



//----------------------MIDI RELATED STUFF-------------------------

MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_CH 1            // listening channel of device
#define MIDI_BROADCAST_CH 16  // all devices receive messages through midi ch 16

//  stepper motor controller midi implementation table
//  cc  values  variable  values
//  11   0-127  i         0 - 800  Hz

void handleCC(byte channel, byte ccNumber, byte ccValue) {
  //
  // map midi cc values to device defined variables
  //
  if (channel == MIDI_CH || channel == MIDI_BROADCAST_CH) {
    switch (ccNumber) {
      case 1:
        i = map(ccValue, 0, 127, 0, 420);
        break;
    }
  }
}



//---------------------------------STEUP----------------------------
void setup() {
  //Serial.begin(9600);

  // set up pins
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, HIGH); //enable pull-up resistor
  pinMode(GATE, OUTPUT);      //TRIAC gate control


  MIDI.setHandleControlChange(handleCC);
  MIDI.turnThruOff();
  //
  MIDI.begin(MIDI_CHANNEL_OMNI);
  // set up Timer1
  //(see ATMEGA 328 data sheet pg 134 for more details)
  cli(); // stop interrupts
  OCR1A = 100;      //initialize the comparator
  TIMSK1 = 0x03;    //enable comparator A and overflow interrupts
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled
  sei(); // allow interrupts

  // set up zero crossing interrupt
  attachInterrupt(0, zeroCrossingInterrupt, RISING);
  //IRQ0 is pin 2. Call zeroCrossingInterrupt
  //on rising signal

}

//Interrupt Service Routines

void zeroCrossingInterrupt() { //zero cross detect
  TCCR1B = 0x04; //start timer with divide by 256 input
  TCNT1 = 0;   //reset timer - count from zero
//  Serial.print(i);
//  Serial.println("  z");
//  Serial.setTimeout(10);
}

ISR(TIMER1_COMPA_vect) { //comparator match
  digitalWrite(GATE, HIGH); //set TRIAC gate to high
  TCNT1 = 65536 - PULSE;    //trigger pulse width
}

ISR(TIMER1_OVF_vect) { //timer1 overflow
  digitalWrite(GATE, LOW); //turn off TRIAC gate
  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

void loop() { // sample code to exercise the circuit
  OCR1A = i_targ;     //set the compare register brightness desired.
  MIDI.read();
}
