/****************************************************************************/
/* Sanyo MBC 550/555 keyboard adapter firmare                               */
/*                                                                          */
/* Copyright (C) 2024 Jochen Toppe                                          */
/*                                                                          */
/* This program is free software; you can redistribute it and/or            */
/* modify it under the terms of the GNU Lesser General Public               */
/* License as published by the Free Software Foundation; either             */
/* version 3 of the License, or (at your option) any later version.         */
/*                                                                          */
/* This program is distributed in the hope that it will be useful,          */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU        */
/* Lesser General Public License for more details.                          */
/*                                                                          */
/* You should have received a copy of the GNU Lesser General Public License */
/* along with this program; if not, write to the Free Software Foundation,  */
/* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      */
/****************************************************************************/

/**
 * Built for US-American keyboard layout only at the moment.
 * CTL-ALT-DEL will send the reset signal to the MBC
 */

// You can activate debug mode that outputs to the serial console
// For debugging and plugging the arduino directly into a
// computer's usb/serial. Don't enable in final firmare.
 // #define DEBUG

#include <PS2KeyAdvanced.h>
#include <PS2KeyMap.h>
#include <PS2KeyAdvanced.h>

// ps2 adapter pins
const int kb_datapin = 8;  //  ps2 data pin
const int kb_irqpin = 3;   //  ps2 clock pin. has to be on 2 or 3 (interrupt pin)

// serial configuration as per MBC-555 specifications
const int MBC_BAUD = 1200;          // 1200 baud
const int MBC_SR_CFG = SERIAL_8E2;  // 8 data, 2 stop bits

// output pin for reset
const int mbc_reset_pin = 6;  // reset pin to MBC; pulled to low for reset

// key codes per MBC manual
const int MBC_END = 0x1;
const int MBC_PG_DOWN = 0x2;    // num pad
const int MBC_SCRL_LOCK = 0x3;  // LOCK key on bottom left
const int MBC_PG_UP = 0x4;      // num pad
const int MBC_BACKTAB = 0x5;    // reverse tab = shift-tab
const int MBC_INSERT = 0x7;     // delete/insert
const int MBC_PRN_SCRN = 0x7;
const int MBC_BACKSPACE = 0x8;  // next to ins/del on mbc keyboard
const int MBC_TAB = 0x9;        // regular tab
const int MBC_ENTER = 0xa;      // numeric keypad enter
const int MBC_HOME = 0xb;       // num pad
const int MBC_RETURN = 0xd;     // main return key (line feed)
const int MBC_F1 = 0x10;
const int MBC_F2 = 0x11;
const int MBC_F3 = 0x12;
const int MBC_F4 = 0x13;
const int MBC_F5 = 0x14;
const int MBC_F6 = 0x15;
const int MBC_F7 = 0x16;
const int MBC_F8 = 0x17;
const int MBC_F9 = 0x18;
const int MBC_F10 = 0x19;
const int MBC_ESC = 0x1b;
const int MBC_CRS_LEFT = 0x1c;
const int MBC_CRS_RIGHT = 0x1d;
const int MBC_CRS_UP = 0x1e;
const int MBC_CRS_DOWN = 0x1f;
const int MBC_DEL = 0x7;

#define CHECK_BIT(var, pos) ((var) & (1 << (pos))) > 0

PS2KeyAdvanced keyboard;

void setup() {
  // set reset pin to high
  pinMode(mbc_reset_pin, OUTPUT);
  digitalWrite(mbc_reset_pin, HIGH);

  // startup delay
  delay(500);

  // setup keyboard
  keyboard.begin(kb_datapin, kb_irqpin);

  // Disable Break codes (key release) from PS2KeyAdvanced
  keyboard.setNoBreak(1);
  // and set no repeat on CTRL, ALT, SHIFT, GUI while outputting
  keyboard.setNoRepeat(1);

  // output
  Serial.begin(MBC_BAUD, MBC_SR_CFG);

#ifdef DEBUG
  Serial.println("\nMBC keyboard translator **** DEBUG MODE ****\n");
#endif
}

uint16_t code;
uint8_t controlOn;
uint8_t altGrKey;
uint8_t altKey;
uint8_t altOn;
uint8_t capsOn;
uint8_t shiftOn;
uint8_t isUpper;

void loop() {
  if (keyboard.available()) {
    // read the next key
    code = keyboard.read();

    if (code > 0) {

#ifdef DEBUG
      Serial.print(" mapped ");
      Serial.print(code, HEX);
      Serial.print(" - Status Bits ");
      Serial.print(code >> 8, HEX);
      Serial.print("  Code (");
      Serial.print(code & 0xFF, HEX);
      Serial.print(")");

      controlOn = CHECK_BIT(code, 13);  // (code & 0x1 << 13) > 0;
      altGrKey = CHECK_BIT(code, 10);   //(code & 0x1 << 10) > 0;
      altKey = CHECK_BIT(code, 11);     //(code & 0x1 << 11) > 0;
      altOn = (altGrKey || altKey) > 0;
      capsOn = CHECK_BIT(code, 12);   // (code & 0x1 << 12) > 0;
      shiftOn = CHECK_BIT(code, 14);  //(code & 0x1 << 14) > 0;
      isUpper = (capsOn || shiftOn) > 0;

      Serial.print("  Shift (");
      Serial.print(isUpper, HEX);
      Serial.print(")");

      Serial.print("  Crtl (");
      Serial.print(controlOn, HEX);
      Serial.print(")");

      Serial.print("  Alt (");
      Serial.print(altOn, HEX);
      Serial.print(")");
      Serial.print("\n");
#endif

      translate(code);
    }
  }
}


void translate(int scanCode) {

  controlOn = CHECK_BIT(code, 13);  // (code & 0x1 << 13) > 0;
  altGrKey = CHECK_BIT(code, 10);   //(code & 0x1 << 10) > 0;
  altKey = CHECK_BIT(code, 11);     //(code & 0x1 << 11) > 0;
  altOn = (altGrKey || altKey) > 0;
  capsOn = CHECK_BIT(code, 12);   // (code & 0x1 << 12) > 0;
  shiftOn = CHECK_BIT(code, 14);  //(code & 0x1 << 14) > 0;
  isUpper = (capsOn || shiftOn) > 0;

  // the character
  int character = code & 0xFF;

  // control characters
  if ((code & 0xFF)) {
    //nop
  }

  switch (character) {
    // main enter
    case PS2_KEY_ENTER:
      w(MBC_ENTER);
      break;
      // return (keypad)
    case PS2_KEY_KP_ENTER:
      w(MBC_RETURN);
      break;
    case PS2_KEY_DELETE:
      if (controlOn && altKey)
        reset();
      else
        w(MBC_BACKSPACE);  // backspace
      break;
    case PS2_KEY_BS:
      w(MBC_BACKSPACE);
      break;
    case PS2_KEY_INSERT:
      w(MBC_INSERT);
      break;
    case PS2_KEY_TAB:
      w(isUpper ? MBC_BACKTAB : MBC_TAB);
      break;
    case PS2_KEY_BREAK:
      Serial.write("?");  // todo: not sure what break sends
      break;
    case PS2_KEY_ESC:
      w(MBC_ESC);
      break;
    case PS2_KEY_KP0:
      w('0');
      break;
    // *** Keypad Numbers with num lock on
    case PS2_KEY_KP1:
      w('1');
      break;
    case PS2_KEY_KP2:
      w('2');
      break;
    case PS2_KEY_KP3:
      w('3');
      break;
    case PS2_KEY_KP4:
      w('4');
      break;
    case PS2_KEY_KP5:
      w('5');
      break;
    case PS2_KEY_KP6:
      w('6');
      break;
    case PS2_KEY_KP7:
      w('7');
      break;
    case PS2_KEY_KP8:
      w('8');
      break;
    case PS2_KEY_KP9:
      w('9');
      break;
    case PS2_KEY_KP_EQUAL:
      w('=');
      break;
    // keypad w/o numlock
    case PS2_KEY_END:
      w(MBC_END);
      break;
    case PS2_KEY_PGUP:
      w(MBC_PG_UP);
      break;
    case PS2_KEY_PGDN:
      w(MBC_PG_DOWN);
      break;
    case PS2_KEY_L_ARROW:
      w(MBC_CRS_LEFT);
      break;
    case PS2_KEY_R_ARROW:
      w(MBC_CRS_RIGHT);
      break;
    case PS2_KEY_DN_ARROW:
      w(MBC_CRS_DOWN);
      break;
    case PS2_KEY_UP_ARROW:
      w(MBC_CRS_UP);
      break;
    case PS2_KEY_HOME:
      w(MBC_HOME);
      break;
    // function keys
    case PS2_KEY_F1:
      w(MBC_F1, controlOn);
      break;
    case PS2_KEY_F2:
      w(MBC_F2, controlOn);
      break;
    case PS2_KEY_F3:
      w(MBC_F3, controlOn);
      break;
    case PS2_KEY_F4:
      w(MBC_F4, controlOn);
      break;
    case PS2_KEY_F5:
      w(MBC_F5, controlOn);
      break;
    case PS2_KEY_F6:
      w(MBC_F6, controlOn);
      break;
    case PS2_KEY_F7:
      w(MBC_F7, controlOn);
      break;
    case PS2_KEY_F8:
      w(MBC_F8, controlOn);
      break;
    case PS2_KEY_F9:
      w(MBC_F9, controlOn);
      break;
      // ******* regular characters
    case PS2_KEY_A:
      w(isUpper ? 'A' : 'a', controlOn);
      break;
    case PS2_KEY_B:
      w(isUpper ? 'B' : 'b', controlOn);
      break;
    case PS2_KEY_C:
      w(isUpper ? 'C' : 'c', controlOn);
      break;
    case PS2_KEY_D:
      w(isUpper ? 'D' : 'd', controlOn);
      break;
    case PS2_KEY_E:
      w(isUpper ? 'E' : 'e', controlOn);
      break;
    case PS2_KEY_F:
      w(isUpper ? 'F' : 'f', controlOn);
      break;
    case PS2_KEY_G:
      w(isUpper ? 'G' : 'g', controlOn);
      break;
    case PS2_KEY_H:
      w(isUpper ? 'H' : 'h', controlOn);
      break;
    case PS2_KEY_I:
      w(isUpper ? 'I' : 'i', controlOn);
      break;
    case PS2_KEY_J:
      w(isUpper ? 'J' : 'j', controlOn);
      break;
    case PS2_KEY_K:
      w(isUpper ? 'K' : 'k', controlOn);
      break;
    case PS2_KEY_L:
      w(isUpper ? 'L' : 'l', controlOn);
      break;
    case PS2_KEY_M:
      w(isUpper ? 'M' : 'm', controlOn);
      break;
    case PS2_KEY_N:
      w(isUpper ? 'N' : 'n', controlOn);
      break;
    case PS2_KEY_O:
      w(isUpper ? 'O' : 'o', controlOn);
      break;
    case PS2_KEY_P:
      w(isUpper ? 'P' : 'p', controlOn);
      break;
    case PS2_KEY_Q:
      w(isUpper ? 'Q' : 'q', controlOn);
      break;
    case PS2_KEY_R:
      w(isUpper ? 'R' : 'r', controlOn);
      break;
    case PS2_KEY_S:
      w(isUpper ? 'S' : 's', controlOn);
      break;
    case PS2_KEY_T:
      w(isUpper ? 'T' : 't', controlOn);
      break;
    case PS2_KEY_U:
      w(isUpper ? 'U' : 'u', controlOn);
      break;
    case PS2_KEY_V:
      w(isUpper ? 'V' : 'v', controlOn);
      break;
    case PS2_KEY_W:
      w(isUpper ? 'W' : 'w', controlOn);
      break;
    case PS2_KEY_X:
      w(isUpper ? 'X' : 'x', controlOn);
      break;
    case PS2_KEY_Y:
      w(isUpper ? 'Y' : 'y', controlOn);
      break;
      break;
    case PS2_KEY_Z:
      w(isUpper ? 'Z' : 'z');
    // ******** numbers
    case PS2_KEY_0:
      w(isUpper ? ')' : '0');
      break;
    case PS2_KEY_1:
      w(isUpper ? '!' : '1');
      break;
    case PS2_KEY_2:
      w(isUpper ? '@' : '2');
      break;
    case PS2_KEY_3:
      w(isUpper ? '#' : '3');
      break;
    case PS2_KEY_4:
      w(isUpper ? '$' : '4');
      break;
    case PS2_KEY_5:
      w(isUpper ? '%' : '5');
      break;
    case PS2_KEY_6:
      w(isUpper ? '^' : '6');
      break;
    case PS2_KEY_7:
      w(isUpper ? '&' : '7');
      break;
    case PS2_KEY_8:
      w(isUpper ? '*' : '8');
      break;
    case PS2_KEY_9:
      w(isUpper ? '(' : '9');
      break;
      // ******** punctuation
    case PS2_KEY_DOT:
      w(isUpper ? '>' : '.');
      break;
    case PS2_KEY_DIV:
      w(isUpper ? '?' : '/');
      break;
    case PS2_KEY_EQUAL:
      w(isUpper ? '+' : '=');
      break;
    case PS2_KEY_MINUS:
      w(isUpper ? '_' : '-');
      break;
    case PS2_KEY_COMMA:
      w(isUpper ? '<' : ',');
      break;
    case PS2_KEY_APOS:
      w(isUpper ? '\'' : '\'');
      break;
    case PS2_KEY_SEMI:
      w(isUpper ? ':' : ';');
      break;
    case PS2_KEY_OPEN_SQ:
      w(isUpper ? '{' : '[');
      break;
    case PS2_KEY_CLOSE_SQ:
      w(isUpper ? '}' : ']');
      break;
    case PS2_KEY_BACK:
      w(isUpper ? '|' : '\\');
      break;
    case PS2_KEY_SPACE:
      w(' ');
      break;

    default:
#ifdef DEBUG
      Serial.print("NOOP");
#endif
      break;
  }

  //  }

  //todo: add keypad handling
}

/** Write to serial with debug magic*/
void w(char code) {
#ifdef DEBUG
  Serial.print("(");
  Serial.print(code, HEX);
  Serial.print(")");
#else
  Serial.write(code);
#endif
}

/**
 * shift to the upper range of the ASCII table
 */
void w(char code, int shift) {
  w(shift ? (code | 0x80) : code); // shift 8th bit on if contrl
}

/*** 
 * reset by pushing the line to 0
 */
void reset() {
#ifdef DEBUG
  Serial.print("RESET\n");
#else
  digitalWrite(mbc_reset_pin, LOW);
  delay(500);
  digitalWrite(mbc_reset_pin, HIGH);
#endif
}