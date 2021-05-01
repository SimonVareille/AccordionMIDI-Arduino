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
