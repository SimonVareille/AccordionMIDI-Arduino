/*******************************************************************************
 Accordion MIDI - Arduino
 https://github.com/SimonVareille/AccordionMIDI-Arduino
 Copyright © 2021 Simon Vareille
 Copyright © 2016-2017 Brendan Vavra
 Based on projects by Brendan Vavra 2016-2017
 (https://github.com/bvavra/MIDI_Accordion), Dimon Yegorenkov 2011 and
 Jason Bugeja 2014
 *******************************************************************************
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#include <new>

//#define DEBUG//uncomment this line to print serial messages, comment to send MIDI data
//#define BLUETOOTH//uncomment this line to send MIDI data via bluetooth instead of USB
//#define BMP//uncomment this line to use the BMP180 to add dynamics via bellows
//#define JOYSTICK//uncomment this line to use a joystick as a pitch-bend controller

#include "midi.h"
#include "keyboard.hpp"

/*
 * We have 81 + 96 = 177 keys. We thus need a 12x16 grid.
 * We use 12 output pins and 16 (2x8) input pins.
 * We use pins D38-D49 as output. These pins are shared between left and right
 * keyboards. We thus output on one pin then read both right and left keyboard
 * at the same time.
 * We use PINA (pins D22-D29) for reading right keyboard.
 * We use PINC (pins D37-D30) for reading left keyboard.
 */

// array to store up/down status of left keys
// there is room for 12*8=96 buttons
int LeftKeysStatus[] = {
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000
};

// array to store up/down status of right keys
// there is room for 12*8=96 buttons
int right_keys_status[] = {
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000,
  B0000000
};

RightKeyboard right_keyboard;
Keyboard* edited_keyboard = nullptr;

void setup()
{
  //Handle incoming midi messages
  MIDI.setHandleSystemExclusive(systemExclusiveHandler);
  #ifdef DEBUG
    Serial.begin(9600);
    while (!Serial);
  #else
    MIDI.begin();
    //If we're sending MIDI over Serial1, open Serial for additional debugging
    #ifdef BLUETOOTH
      Serial.begin(9600);
      while (!Serial);
    #endif
  #endif

  //Disable soft thru so that incoming message are not sent back
  MIDI.turnThruOff();

  //Digital output pins start turned off
  for (int i=38; i<=49; i++) {
    pinMode(i,OUTPUT);
    digitalWrite(i, LOW);
  }

  //Turn on pullup resistor on input pins D22-D37
  for (int i=22; i<=37; i++) {
    pinMode(i, INPUT);
  }

  // Init right keyboard
  apply_default_right_keyboard();

  #ifdef BMP
    init_BMP();
  #endif

  #ifdef JOYSTICK
    init_joystick();
  #endif
}

//MIDI Control Change code for expression, which is a percentage of velocity
const int CC_Expression = 11;
int prev_expression = 127;
//The BMP_180 is very sensitive, so readings can vary wildly from sample to sample.
//We're getting around this by computing and sending the average of bmp_sample_rate samples.
//A smaller bmp_sample_rate is more granular, but allows more "noise" to come in.
//A larger bmp_sample_rate is less granular, but has a smoother contour.
//Setting bmp_sample_rate too large may also lose the amount of perceived expression
//and create a noticable delay between squeezing the bellows and hearing the volume change,
//resulting in choppy crescendos and decrescendos.
//Tweak this value as needed.
const int bmp_sample_rate = 5;
int expression_avg[bmp_sample_rate];
int e = 0;

int joystick_prev_val = 0;

bool receivingSysEx = false;

void loop()
{
  #ifndef DEBUG
  MIDI.read();
  #endif //DEBUG
  // When receiving a long SysEx, disabling the main loop so that we don't miss
  // parts of the SysEx
  if(!receivingSysEx) {
    #ifdef BMP
      //Read pressure from the BMP_180 and convert it to MIDI expression
      int expression = get_expression(prev_expression);
  
      //Ignore it if it didn't change
      if(expression != prev_expression) {
        expression_avg[e] = expression;
        //Only send MIDI CC every bmp_sample_rate times,
        //but send the average of the last bmp_sample_rate deltas
        if (e == bmp_sample_rate - 1){
          expression = 0;
          for (int i=0; i<bmp_sample_rate; i++){
            expression += expression_avg[i];
          }
          expression = expression/bmp_sample_rate;
  
          #ifdef DEBUG
            Serial.print("Expression Change: ");
            Serial.println(expression);
          #else
            MIDI.sendControlChange(CC_Expression,expression,1);
            //Don't let bass overpower melody
            MIDI.sendControlChange(CC_Expression,constrain(expression-6,0,127),2);
            //Don't let chords overpower melody
            MIDI.sendControlChange(CC_Expression,constrain(expression-12,0,127),3);
          #endif
          prev_expression = expression;
          e = 0;
        }
        else {
          e = e + 1;
        }
      }
    #endif
  
    #ifdef JOYSTICK
      int pitch_bend_val = scan_joystick();
      if(pitch_bend_val != joystick_prev_val) {
        #ifdef DEBUG
          Serial.print("Pitch Bend Change: ");
          Serial.println(pitch_bend_val);
        #else
          //Comment and uncomment to select which channels you want pitch bend to affect.
          MIDI.sendPitchBend(pitch_bend_val, 1);
          //MIDI.sendPitchBend(pitch_bend_val, 2);
          //MIDI.sendPitchBend(pitch_bend_val, 3);
          joystick_prev_val = pitch_bend_val;
        #endif
      }
    #endif
    
    for(int pin=38, group=0; pin<=49; pin++, group++){
      //TODO - I wonder if we can replace this with direct port write for
      // even better performance?
      digitalWrite(pin, HIGH);
      byte right_reg_value = ~PINA;
      byte left_reg_value = ~PINC;
      digitalWrite(pin, LOW);
      digitalWrite(pin+1 == 50 ? 38 : pin+1, HIGH);
      scan_pins(right_keyboard, group, right_reg_value,
                right_keys_status[group]);
    }
  }
}

void scan_pins(Keyboard &keyboard, int group, byte reg_value, int &key_status) {
  if (reg_value != key_status){
    //using bit-wise XOR to send modified bits only
    check_keys(keyboard, reg_value ^ key_status, key_status, group);
    key_status = reg_value;
  }
}

//Check to see which bits have changed and trigger corresponding button
void check_keys(Keyboard &keyboard, byte reg, byte PinStatus, int group){
  for(int i=0; i<8; i++) {
    if((reg >> i) & 1) {
      trigger_button(keyboard, group, i, (PinStatus >> i) & 1);
    }
  }
}

void trigger_button(Keyboard &keyboard, int group, int pos, bool on) {
  if (on)
    keyboard.getButton(group, pos)->on();
  else
    keyboard.getButton(group, pos)->off();
}

void apply_default_right_keyboard() {
  byte temp[100];
  for(size_t i=0; i<sizeof(right_keyboard_default); i+=100){
    size_t size = min(100, sizeof(right_keyboard_default)-i);
    memcpy_P(temp, right_keyboard_default+i, size);
    systemExclusiveHandler(temp, size);
  }
}

void send_default_right_keyboard() {
  byte temp[100];
  memcpy_P(temp, right_keyboard_default, 99);
  temp[2] = 0x01; // From storage
  MIDI.sendSysEx(99, temp, true);
  for(size_t i=101; i<sizeof(right_keyboard_default); i+=100){
    size_t size = min(98, sizeof(right_keyboard_default)-i);
    memcpy_P(temp, right_keyboard_default+i, size);
    MIDI.sendSysEx(size, temp, true);
  }
}

void sendKeyboards() {
  send_default_right_keyboard();
  right_keyboard.send();
}

void systemExclusiveHandler(byte* data, unsigned size) {
  /* If SysEx message is larger than the allocated buffer size,
     data is splitted like:
     first:  0xF0 .... 0xF0
     middle: 0xF7 .... 0xF0
     last:   0xF7 .... 0xF7
  */
  if(data[0] == 0xF0){ // Start of a new SysEx message
    if(edited_keyboard)
      edited_keyboard->clearEdition();
    edited_keyboard = nullptr;
    if(data[1] == 0x7D) {// The message is for us
      if(data[2] == 0x0F) {// We will receive a long SysEx
        receivingSysEx = true;
        MIDI.sendSysEx(sizeof(sysExDialog), sysExDialog, false);
        return;
      }
      else if(data[2] == 0x00) { // Remote asks for keyboards
        sendKeyboards();
      }
      else if(data[2] == 0x02) { // Remote sent a keyboard to apply
        if(data[3] == 0x01) { // RightKeyboard
          edited_keyboard = &right_keyboard;
          edited_keyboard->beginNameEdition();
          data += 4;
          size -= 4;
        }
      }
    }
  }
  else if(data[0] == 0xF7) { // Continuation of the previous SysEx
    data += 1;
    size -= 1;
  }

  if(data[size-1] == 0xF7) { // End of SysEx
    receivingSysEx = false;
  }

  if(edited_keyboard != nullptr) {
    edited_keyboard->editFromSysEx(data, size-1);
  }
}
