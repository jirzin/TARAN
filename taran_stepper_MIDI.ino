#include <MIDI.h>

//-----------------TIMER RELATED THINGS---------------------

// timer 1 is set up to tick every 40 microseconds
// every time tick_couter ++

volatile byte timer_state = 0;    // changing state of counter logics
//                      // 0 = step count
//                      // 1 = step wait
//                      // 2 = gate wait
volatile unsigned int timer_clk = 0;


//------------------MOTOR RELATED THINGS---------------------

// pins
#define DIR 8
#define STEP 7
#define M1 5
#define M2 6
#define GATE 2

// microstepping
// M1   M2  mode
// L    L   Full Step (2 Phase)
// H    L   Half Step
// L    H   Quarter Step
// H    H   Eigth Step

boolean m1 = false;
boolean m2 = true;

volatile unsigned int gateStop = 9000;        // gate stop 0 - 500 ms = 0 - 12500 (40uS)
volatile unsigned int spd = 60;             // spd 800 - 2000 ms = 20 - 50
volatile unsigned int dir = 1;
volatile boolean motor_run = true;

// mirrored variables that are used in timer when midi finished writing
volatile unsigned int gateStop_T;
volatile unsigned int spd_T;
volatile unsigned int dir_T;
volatile boolean midi_done = false;
//volatile boolean motor_run_T;

volatile boolean gateDisabled = false;
volatile unsigned int motor_step = 0;

int gateStop_MIDI = 9000;
int spd_MIDI = 60;             // spd 800 - 2000 ms = 20 - 50
int dir_MIDI = 1;
boolean motor_run_MIDI = true;

//------------------MIDI SETUP------------------------------

boolean midi_on = false;

//if (midi_on) {
MIDI_CREATE_DEFAULT_INSTANCE();
//}

#define MIDI_CH 1            // listening channel of device
#define MIDI_BROADCAST_CH 16  // all devices receive messages through midi ch 16

//  stepper motor controller midi implementation table
//  cc  values  variable  values
//  1   0-127   spd       1250 - 500  Hz
//  2   0-127   gateStop  1220 - 5    Hz
//  3   0-1     dir
//  4   0-1     motor_run

void handleCC(byte channel, byte ccNumber, byte ccValue) {
  //
  // map midi cc values to device defined variables
  //
  if (channel == MIDI_CH || channel == MIDI_BROADCAST_CH) {
    switch (ccNumber) {
      case 1:
        spd = map(ccValue, 0, 127, 50, 230);
        break;
      case 2:
        gateStop = map(ccValue, 0, 127, 50, 60000);
        break;
      case 3:
        dir = ccValue;
        break;
      case 4:
        motor_run = ccValue;
        break;
    }
  }
}



//-----------------GENERAL VARIABLES AND THINGS-------------





//-----------------STEUP------------------------------------
void setup() {

  // pins
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(13, OUTPUT);

  pinMode(GATE, INPUT);


  // microstepping pins setup
  digitalWrite(M1, m1);
  digitalWrite(M2, m2);


  // park motor to gate
  //  while (digitalRead(GATE) != HIGH) {
  //    makeStep();
  //  }


  // midi
  //  if (midi_on) {
  MIDI.setHandleControlChange(handleCC);
  MIDI.turnThruOff();
  //
  MIDI.begin(MIDI_CHANNEL_OMNI);
  //  } else {
  //Serial.begin(9600);
  //  }



  // TIMER 1 for interrupt frequency 100000 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 100000 Hz increments
  OCR1A = 159; // = 16000000 / (1 * 100000) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 1 prescaler
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts

  //delay(100);

  //attachInterrupt(digitalPinToInterrupt(GATE), gateExit, FALLING);

}

ISR(TIMER1_COMPA_vect) {
  //interrupt commands for TIMER 1 here

  //  spd=spd_MIDI;
  //  gateStop=gateStop_MIDI;
  //  dir=dir_MIDI;
  //  motor_run=motor_run_MIDI;
  if (!gateDisabled) {
    if (digitalRead(GATE) == HIGH) {
      timer_state = 2;
      motor_step = 0;
      gateDisabled = true;
    }
  }

  timer_clk++;


  //if (motor_run) {
  switch (timer_state) {
    case 0:
      digitalWrite(STEP, HIGH);
      if (timer_clk >= 4) {
        timer_state = 1;
        timer_clk = 0;
        digitalWrite(STEP, LOW);
//        motor_step++;
//        if (gateDisabled) {
//          if (motor_step > 100) {
//            gateDisabled = false;
//          }
//        }
        break;
      }
      break;
    case 1:
      //      if (timer_clk >= 80) {
      //        gateDisabled = false;
      //      }
      if (timer_clk >= spd) {
        motor_step++;
        if(motor_step>40){
            gateDisabled=false;
          }
        timer_state = 0;
        timer_clk = 0;
        break;
      }
      break;
    case 2:
      //gateWait = true;
      //gateDisabled = true;
      //digitalWrite(STEP, LOW);
      if (timer_clk >= gateStop) {
        timer_state = 0;
        timer_clk = 0;
        //gateDisabled = true;
        //gateWait = false;
        break;
      }
      break;
  }
  //}
}

//---------------LOOP---------------------------------------
void loop() {
  //  if (midi_on) {
  MIDI.read();
  //  } else {
  //
  //  }
  digitalWrite(DIR, dir);
  //  Serial.println(timer_clk);

}



//---------------GENERAL FUNCTIONS--------------------------
void makeStep() {
  //
  // this function is used just for start up to park motor to gate position
  // dont use this function enywhere else in code it implements delay and that is not COOL
  //
  digitalWrite(STEP, HIGH);
  delayMicroseconds(40);
  digitalWrite(STEP, LOW);
  delayMicroseconds(spd);
}

//
//void gateEntered() {
//
//  if (!gateDisabled) {
//    timer_clk = 0;
//    timer_state = 2;
//    gateDisabled = true;
//  }
//  //  for(int i = 4; i>0;i--){
//  //  makeStep();
//  //  }
//}

void gateExit() {
  // if (!gateWait) {
  gateDisabled = false;
  // }
}
