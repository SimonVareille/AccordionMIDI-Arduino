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

void ProgramButton::on()
{
  MIDI.sendProgramChange(program, channel);
}
void ProgramButton::off()
{
}

void ControlButton::on()
{
  MIDI.sendControlChange(control, value, channel);
}
void ControlButton::off()
{
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
