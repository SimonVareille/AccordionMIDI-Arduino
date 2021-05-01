/*******************************************************************************
  Accordion MIDI - Arduino
  https://github.com/SimonVareille/AccordionMIDI-Arduino
  Copyright � 2021 Simon Vareille

  Based on projects byBrendan Vavra 2016-2017, Dimon Yegorenkov 2011 and Jason Bugeja 2014
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
#include "keyboard.h"

void NoteButton::on()
{
  MIDI.sendNoteOn(pitch, velocity, channel);
}
void NoteButton::off()
{
  MIDI.sendNoteOff(pitch, velocity, channel);
}
static bool NoteButton::isValid(const uint8_t channel, const uint8_t pitch,
                                const uint8_t velocity)
{
  return channel < 16 && pitch < 128 && velocity < 128;
}
static void NoteButton::create(Button *place, const uint8_t channel,
                               const uint8_t pitch, const uint8_t velocity) {
  if(!isValid(channel, pitch, velocity)) {
    // A value is bad, skip
    new(place) NullButton();
  }
  else {
    new(place) NoteButton(channel, pitch, velocity);
  }
}

void ProgramButton::on()
{
  MIDI.sendProgramChange(program, channel);
}
void ProgramButton::off()
{
}
static bool ProgramButton::isValid(const uint8_t channel, const uint8_t program)
{
  return channel < 16 && program < 128;
};
static void ProgramButton::create(Button *place, const uint8_t channel,
                                  const uint8_t program) {
  if(!isValid(channel, program)) {
    // A value is bad, skip
    new(place) NullButton();
  }
  else {
    new(place) ProgramButton(channel, program);
  }
};

void ControlButton::on()
{
  MIDI.sendControlChange(control, value, channel);
}
void ControlButton::off()
{
}
static bool ControlButton::isValid(const uint8_t channel, const uint8_t control,
                                   const uint8_t value)
{
  return channel < 16 && control < 128 && value < 128;
}
static void ControlButton::create(Button *place, const uint8_t channel,
                                  const uint8_t control, const uint8_t value) {
  if(!isValid(channel, control, value)) {
    // A value is bad, skip
    new(place) NullButton();
  }
  else {
    new(place) ControlButton(channel, control, value);
  }
}

Button *GenericButton::operator->() {
  return (Button *)buf;
}
Button const *GenericButton::operator->() const {
  return (Button const *)buf;
}
Button *GenericButton::get() {
  return (Button *)buf;
}
Button const *GenericButton::get() const {
  return (Button const *)buf;
}
GenericButton& GenericButton::operator=(const GenericButton &x)
{
  if (&x != this)
    memcpy(this, &x, sizeof(*this));
  return *this;
}

void Keyboard::clearEdition() {
  write_pos = 0;
  read_name = false;
  read_buttons = false;
  pad = 0;
  read_junk = false;
}
void Keyboard::beginNameEdition() {
  write_pos = 0;
  pad = 0;
  read_name = true;
}
size_t Keyboard::write_pos = 0;
bool Keyboard::read_name = false;
bool Keyboard::read_buttons = false;
uint8_t Keyboard::pad = 0;
unsigned char Keyboard::temp_bytes[4] = {0,0,0,0};
bool Keyboard::read_junk = false;
void Keyboard::editFromSysEx(const byte* data, unsigned size) {
  if(read_name) {
    size_t pos = nameFromSysEx(data, size);
    data += pos;
    size -= pos;
    if(!read_name) { // Name is done
      read_buttons = true;
      write_pos = 0;
      pad = 0;
    }
  }
  if(read_buttons) {
    buttonsFromSysEx(data, size);
  }
}
size_t Keyboard::nameFromSysEx(const byte* data, unsigned size) {
  // Get the end of the name
  size_t i;
  for(i=0; data[i] != 0x00 && i < size; i++);
  if(read_junk) { // The last chunck was bad
    if(i==size) { // Current chunck is bad
      return size;
    }
    else {
      read_junk = false;
      return i+1;
    }
  }
  if(i==size) { // The name is not over but the chunck is
    if(pad) { // The last chunck ended abruptly
      memcpy(temp_bytes+pad, data, 4-pad);
      write_pos += decode_base64(temp_bytes, 4,
                                 name+write_pos);
      name[write_pos] = '\0';
      data += 4-pad;
      i -= 4-pad;
    }
    read_name = true;
    pad = i%4; // Get the number of bytes that goes with the next chunck
    size_t decode_len = min(i - pad,
                            (MAX_NAME_LENGTH - 1 - write_pos)/3*4);
    size_t len = decode_base64((unsigned char*)data, decode_len,
                               name+write_pos);
    write_pos += len;
    name[write_pos] = '\0';
    if(len < decode_len/4*3 // The base64 string is corrupted
       || decode_len < i - pad) { // The base64 string is too long
      read_junk = true;
      return encode_base64_length(len);
    }
    memcpy(temp_bytes, data+i-pad, pad);
    return size;
  }
  else {
    if(pad) { // The last chunck ended abruptly
      memcpy(temp_bytes+pad, data, 4-pad);
      write_pos += decode_base64(temp_bytes, 4,
                                 name+write_pos);
      name[write_pos] = '\0';
      data += 4-pad;
      i -= 4-pad;
    }
    size_t decode_len = min(i,
                            (MAX_NAME_LENGTH - 1 - write_pos)/3*4);
    write_pos += decode_base64(data, decode_len,
                               name+write_pos);
    name[write_pos] = '\0';
    write_pos = 0;
    read_name = false;
    return i+1;
  }
}

void RightKeyboard::clear()
{
  for (auto& row : keyboard)
  {
    for (GenericButton& button : row)
    {
      button->~Button();
    }
  }
}
Button* RightKeyboard::getButton(int grp, int index)
{
  return keyboard[grp][index].get();
}
size_t RightKeyboard::buttonsFromSysEx(const byte* data, unsigned size) {
  size_t i=0;
  if(pad) {
    switch(temp_bytes[0])
    {
    case 0x01:
      memcpy(temp_bytes+pad, data, 4-pad);
      NoteButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         temp_bytes[1], temp_bytes[2], temp_bytes[3]);
      i += 4-pad;
      break;
    case 0x02:
      memcpy(temp_bytes+pad, data, 3-pad);
      ProgramButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         temp_bytes[1], temp_bytes[2]);
      i += 3-pad;
      break;
    case 0x03:
      memcpy(temp_bytes+pad, data, 4-pad);
      ControlButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         temp_bytes[1], temp_bytes[2], temp_bytes[3]);
      i += 4-pad;
      break;
    }
    pad = 0;
    write_pos++;
  }
  for(;i<size && write_pos<81; write_pos++) {
    switch(data[i])
    {
    case 0x01: // NoteButton
      if(size - i < 4) { // This chunk end abruptly
        pad = size - i;
        memcpy(temp_bytes, data+i, pad);
        return size;
      }
      NoteButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         data[i+1], data[i+2], data[i+3]);
      i += 4;
      break;
    case 0x02: //ProgramButton
      if(size - i < 3) { // This chunk end abruptly
        pad = size - i;
        memcpy(temp_bytes, data+i, pad);
        return size;
      }
      ProgramButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         data[i+1], data[i+2]);
      i += 3;
      break;
    case 0x03: //ControlButton
      if(size - i < 4) { // This chunk end abruptly
        pad = size - i;
        memcpy(temp_bytes, data+i, pad);
        return size;
      }
      ControlButton::create(keyboard[write_pos/8][write_pos%8].get(),
                         data[i+1], data[i+2], data[i+3]);
      i += 4;
      break;
    default:
      new(keyboard[write_pos/8][write_pos%8].get()) NullButton();
    }
  }
  if(write_pos == 81) { // End of keyboard
    read_buttons = false;
  }
  return i;
}
