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
/**
   Abstract class to represents a Button.
*/
class Button
{
  public:
    virtual ~Button() {};
    virtual void on() = 0;
    virtual void off() = 0;
};

/**
   Represents a button that does nothing.
*/
class NullButton: public Button
{
  public:
    virtual void on()
    {
    };
    virtual void off()
    {
    };
};

/**
   Represents a button that send a note on/off message.
*/
class NoteButton: public Button
{
  public:
    NoteButton(const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
      : channel(channel), pitch(pitch), velocity(velocity)
    {
    };
    virtual void on()
    {
      MIDI.sendNoteOn(pitch, velocity, channel);
    };
    virtual void off()
    {
      MIDI.sendNoteOff(pitch, velocity, channel);
    };
  private:
    uint8_t channel;
    uint8_t pitch;
    uint8_t velocity;
};

/**
   Represents a button that send a program change message.
*/
class ProgramButton: public Button
{
  public:
    ProgramButton(const uint8_t channel, const uint8_t program)
      : channel(channel), program(program)
    {
    };
    virtual void on()
    {
      MIDI.sendProgramChange(program, channel);
    };
    virtual void off()
    {
    };
  private:
    uint8_t channel;
    uint8_t program;
};

/**
   Represents a button that send a control change message
*/
class ControlButton: public Button
{
  public:
    ControlButton(const uint8_t channel, const uint8_t control, const uint8_t value)
      : channel(channel), control(control), value(value)
    {
    };
    virtual void on()
    {
      MIDI.sendControlChange(control, value, channel);
    };
    virtual void off()
    {
    };
  private:
    uint8_t channel;
    uint8_t control;
    uint8_t value;
};

/**
   Union to get the size of the largest Button subclass.
*/
union max_button_size {
  NoteButton s1;
  ProgramButton s2;
  ControlButton s3;
};

/**
   Class to statically allocate a button, while keeping all the
   polymorphic gains.
*/
class GenericButton
{
  public:
    Button *operator->() {
      return (Button *)buf;
    }
    Button const *operator->() const {
      return (Button const *)buf;
    }
    Button *get() { // For placement new or delete
      return (Button *)buf;
    }
    GenericButton& operator=(const GenericButton &x)
    {
      if (&x != this)
        memcpy(this, &x, sizeof(*this));
      return *this;
    }
  private:
    byte buf[sizeof(union max_button_size)];
};

/**
   Right keyboard.

   Represent a right button keyboard of 81 button, as 4 rows of 16 buttons
   and 1 row of 17 buttons.

   A keyboard is made of GenericButton. A GenericButton holds a Button.
   To create a button, we use the placement new operator :
     new(kbd.keyboard[i][j].get()) NoteButton(i, j, i+j);
   This doesn't allocate memory, but use the memory at kbd.keyboard[i][j].get().

*/
class RightKeyboard
{
  public:
    RightKeyboard()
    {
    };
    ~RightKeyboard()
    {
    }
    void clear()
    {
      for (auto& row : keyboard)
      {
        for (GenericButton& button : row)
        {
          button->~Button();
        }
      }
    }
    Button* getButton(int grp, int index)
    {
      return keyboard[grp][index].get();
    }
    GenericButton keyboard[12][8];
};
