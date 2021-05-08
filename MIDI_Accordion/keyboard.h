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
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <Arduino.h>
#include "midi.h"

/**
   Abstract class to represents a Button.
*/
class Button
{
  public:
    virtual ~Button() {};
    virtual void on() = 0;
    virtual void off() = 0;
    virtual uint8_t toBytes(byte *buf) = 0;
};

/**
   Represents a button that does nothing.
*/
class NullButton: public Button
{
  public:
    virtual void on()
    {};
    virtual void off()
    {};
    virtual uint8_t toBytes(byte *buf);
};

/**
   Represents a button that send a note on/off message.
*/
class NoteButton: public Button
{
  public:
    NoteButton(const uint8_t channel, const uint8_t pitch, const uint8_t velocity)
      : channel(channel), pitch(pitch), velocity(velocity)
    {};
    virtual void on();
    virtual void off();
    virtual uint8_t toBytes(byte *buf);
    static bool isValid(const uint8_t channel, const uint8_t pitch,
                        const uint8_t velocity);
    static void create(Button *place, const uint8_t channel,
                       const uint8_t pitch, const uint8_t velocity);
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
    {};
    virtual void on();
    virtual void off();
    virtual uint8_t toBytes(byte *buf);
    static bool isValid(const uint8_t channel, const uint8_t program);
    static void create(Button *place, const uint8_t channel,
                       const uint8_t program);
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
    {};
    virtual void on();
    virtual void off();
    virtual uint8_t toBytes(byte *buf);
    static bool isValid(const uint8_t channel, const uint8_t control,
                        const uint8_t value);
    static void create(Button *place, const uint8_t channel,
                       const uint8_t control, const uint8_t value);
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
    Button *operator->();
    Button const *operator->() const;
    // For placement new or delete
    Button *get();
    Button const *get() const;
    GenericButton& operator=(const GenericButton &x);
  private:
    byte buf[sizeof(union max_button_size)];
};

#define MAX_NAME_LENGTH 109

/**
 * Base class to handle keyboards.
 * Defines variable used by all keyboard types.
 */
class Keyboard
{
public:
  size_t nameFromSysEx(const byte* data, unsigned size);
  virtual size_t buttonsFromSysEx(const byte* data, unsigned size) = 0;
  virtual void send() = 0;
  virtual uint8_t type() = 0;
  void editFromSysEx(const byte* data, unsigned size);
  void clearEdition();
  void beginNameEdition();

  unsigned char name[MAX_NAME_LENGTH];

  static size_t write_pos;
  static bool read_name;
  static bool read_buttons;
  static uint8_t pad;
  static unsigned char temp_bytes[5];
  static bool read_junk;
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
class RightKeyboard: public Keyboard
{
  public:
    RightKeyboard()
    {};
    ~RightKeyboard()
    {};
    void clear();
    Button* getButton(int grp, int index);
    virtual size_t buttonsFromSysEx(const byte* data, unsigned size);
    virtual uint8_t type();
    virtual void send();
    GenericButton keyboard[12][8];
};

const byte right_keyboard_default[] PROGMEM =
  {0xf0, 0x7d, 0x02, 0x01, 0x55, 0x6d, 0x6c, 0x6e, 0x61, 0x48,
   0x51, 0x67, 0x61, 0x32, 0x56, 0x35, 0x59, 0x6d, 0x39, 0x68,
   0x63, 0x6d, 0x51, 0x3d, 0x00, 0x01, 0x01, 0x34, 0x7f, 0x01,
   0x01, 0x34, 0x7f, 0x01, 0x01, 0x35, 0x7f, 0x01, 0x01, 0x36,
   0x7f, 0x01, 0x01, 0x37, 0x7f, 0x01, 0x01, 0x36, 0x7f, 0x01,
   0x01, 0x37, 0x7f, 0x01, 0x01, 0x38, 0x7f, 0x01, 0x01, 0x39,
   0x7f, 0x01, 0x01, 0x3a, 0x7f, 0x01, 0x01, 0x39, 0x7f, 0x01,
   0x01, 0x3a, 0x7f, 0x01, 0x01, 0x3b, 0x7f, 0x01, 0x01, 0x3c,
   0x7f, 0x01, 0x01, 0x3d, 0x7f, 0x01, 0x01, 0x3c, 0x7f, 0x01,
   0x01, 0x3d, 0x7f, 0x01, 0x01, 0x3e, 0x7f, 0x01, 0x01, 0xf0,
   0xf7, 0x3f, 0x7f, 0x01, 0x01, 0x40, 0x7f, 0x01, 0x01, 0x3f,
   0x7f, 0x01, 0x01, 0x40, 0x7f, 0x01, 0x01, 0x41, 0x7f, 0x01,
   0x01, 0x42, 0x7f, 0x01, 0x01, 0x43, 0x7f, 0x01, 0x01, 0x42,
   0x7f, 0x01, 0x01, 0x43, 0x7f, 0x01, 0x01, 0x44, 0x7f, 0x01,
   0x01, 0x45, 0x7f, 0x01, 0x01, 0x46, 0x7f, 0x01, 0x01, 0x45,
   0x7f, 0x01, 0x01, 0x46, 0x7f, 0x01, 0x01, 0x47, 0x7f, 0x01,
   0x01, 0x48, 0x7f, 0x01, 0x01, 0x49, 0x7f, 0x01, 0x01, 0x48,
   0x7f, 0x01, 0x01, 0x49, 0x7f, 0x01, 0x01, 0x4a, 0x7f, 0x01,
   0x01, 0x4b, 0x7f, 0x01, 0x01, 0x4c, 0x7f, 0x01, 0x01, 0x4b,
   0x7f, 0x01, 0x01, 0x4c, 0x7f, 0x01, 0x01, 0x4d, 0x7f, 0xf0,
   0xf7, 0x01, 0x01, 0x4e, 0x7f, 0x01, 0x01, 0x4f, 0x7f, 0x01,
   0x01, 0x4e, 0x7f, 0x01, 0x01, 0x4f, 0x7f, 0x01, 0x01, 0x50,
   0x7f, 0x01, 0x01, 0x51, 0x7f, 0x01, 0x01, 0x52, 0x7f, 0x01,
   0x01, 0x51, 0x7f, 0x01, 0x01, 0x52, 0x7f, 0x01, 0x01, 0x53,
   0x7f, 0x01, 0x01, 0x54, 0x7f, 0x01, 0x01, 0x55, 0x7f, 0x01,
   0x01, 0x54, 0x7f, 0x01, 0x01, 0x55, 0x7f, 0x01, 0x01, 0x56,
   0x7f, 0x01, 0x01, 0x57, 0x7f, 0x01, 0x01, 0x58, 0x7f, 0x01,
   0x01, 0x57, 0x7f, 0x01, 0x01, 0x58, 0x7f, 0x01, 0x01, 0x59,
   0x7f, 0x01, 0x01, 0x5a, 0x7f, 0x01, 0x01, 0x5b, 0x7f, 0x01,
   0x01, 0x5a, 0x7f, 0x01, 0x01, 0x5b, 0x7f, 0x01, 0x01, 0xf0,
   0xf7, 0x5c, 0x7f, 0x01, 0x01, 0x5d, 0x7f, 0x01, 0x01, 0x5e,
   0x7f, 0x01, 0x01, 0x5d, 0x7f, 0x01, 0x01, 0x5e, 0x7f, 0x01,
   0x01, 0x5f, 0x7f, 0x01, 0x01, 0x60, 0x7f, 0x01, 0x01, 0x61,
   0x7f, 0x01, 0x01, 0x60, 0x7f, 0x01, 0x01, 0x61, 0x7f, 0x01,
   0x01, 0x62, 0x7f, 0x01, 0x01, 0x63, 0x7f, 0x01, 0x01, 0x63,
   0x7f, 0x01, 0x01, 0x64, 0x7f, 0xf7};
#endif //__KEYBOARD_H__
