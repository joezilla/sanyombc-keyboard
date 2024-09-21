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
 * CTL-ALT-DEL will send the reset signal to the MBC.
 */

// You can activate debug mode that outputs to the serial console
// For debugging and plugging the arduino directly into a
// computer's usb/serial. Don't enable in final firmare.
// #define DEBUG 1

#include <PS2KeyAdvanced.h>
#include <PS2KeyMap.h>

// sanyo scan codes
#include "scancodes.h"

// ps2 adapter pins
const int KB_DATAPIN = 8;  // ps2 data pin
const int KB_IRQPIN = 3;   // ps2 clock pin. has to be on 2 or 3 (interrupt pin)

// serial configuration as per MBC-555 specifications
const int MBC_BAUD = 1200;          // 1200 baud
const int MBC_SR_CFG = SERIAL_8E2;  // 8 data, 2 stop bits

// output pin for reset
const int MBC_RESET_PIN = 6;  // reset pin to MBC; pulled to low for reset

// macro
#define CHECK_BIT(var, pos) ((var) & (1 << (pos))) > 0

PS2KeyAdvanced keyboard;
// character from ps2
uint16_t currentScanCode;
// status codes for pressed keys
bool isControlPressed;
bool isAltGrPressed;
bool isAltPressed;
bool isAnyAltPressed;
bool isCapsLockOn;
bool isShiftPressed;
bool upperCase;

void setup() {
  // set reset pin to high
  pinMode(MBC_RESET_PIN, OUTPUT);
  digitalWrite(MBC_RESET_PIN, HIGH);

  // startup delay
  delay(500);

  // setup keyboard
  keyboard.begin(KB_DATAPIN, KB_IRQPIN);

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

void loop() {
  if (keyboard.available()) {
    // read the next key
    currentScanCode = keyboard.read();
    if (currentScanCode > 0) {
      processScanCode();
    }
  }
}

void processScanCode() {

  isControlPressed = CHECK_BIT(currentScanCode, 13);
  isAltGrPressed = CHECK_BIT(currentScanCode, 10);
  isAltPressed = CHECK_BIT(currentScanCode, 11);
  isAnyAltPressed = isAltGrPressed || isAltPressed;
  isCapsLockOn = CHECK_BIT(currentScanCode, 12);
  isShiftPressed = CHECK_BIT(currentScanCode, 14);
  upperCase = isCapsLockOn || isShiftPressed;

#ifdef DEBUG
  Serial.print(" mapped ");
  Serial.print(currentScanCode, HEX);
  Serial.print(" - Status Bits ");
  Serial.print(currentScanCode >> 8, HEX);
  Serial.print("  Code (");
  Serial.print(currentScanCode & 0xFF, HEX);
  Serial.print(")");
  Serial.print("  Shift (");
  Serial.print(upperCase, HEX);
  Serial.print(")");
  Serial.print("  Crtl (");
  Serial.print(isControlPressed, HEX);
  Serial.print(")");
  Serial.print("  Alt (");
  Serial.print(isAnyAltPressed, HEX);
  Serial.print(")");
  Serial.print("\n");
#endif

  // the character
  int character = currentScanCode & 0xFF;

  // control characters
  if ((currentScanCode & 0xFF)) {
    //nop
  }

  switch (character) {
    // main enter
    case PS2_KEY_ENTER:
      w(MBC_RETURN);
      break;
      // return (keypad)
    case PS2_KEY_KP_ENTER:
      w(MBC_ENTER);
      break;
    case PS2_KEY_DELETE:
      if (isControlPressed && isAltPressed)
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
      w(upperCase ? MBC_BACKTAB : MBC_TAB);
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
    case PS2_KEY_KP_MINUS:
      w('-');
      break;
    case PS2_KEY_KP_PLUS:
      w('+');
      break;
    case PS2_KEY_KP_DIV:
      w('/');
      break;
    case PS2_KEY_KP_TIMES:
      w('*');
      break;
    case PS2_KEY_KP_DOT:
      w('.');
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
      w(isControlPressed ? CTRL_F1 : MBC_F1);
      break;
    case PS2_KEY_F2:
      w(isControlPressed ? CTRL_F2 : MBC_F2);
      break;
    case PS2_KEY_F3:
      w(isControlPressed ? CTRL_F3 : MBC_F3);
      break;
    case PS2_KEY_F4:
      w(isControlPressed ? CTRL_F4 : MBC_F4);
      break;
    case PS2_KEY_F5:
      w(isControlPressed ? CTRL_F5 : MBC_F5);
      break;
    case PS2_KEY_F6:
      w(isControlPressed ? CTRL_F6 : MBC_F6);
      break;
    case PS2_KEY_F7:
      w(isControlPressed ? CTRL_F7 : MBC_F7);
      break;
    case PS2_KEY_F8:
      w(isControlPressed ? CTRL_F8 : MBC_F8);
      break;
    case PS2_KEY_F9:
      w(isControlPressed ? CTRL_F9 : MBC_F9);
      break;
    case PS2_KEY_F10:
      w(isControlPressed ? CTRL_F10 : MBC_F10);
      break;
      // ******* regular characters
    case PS2_KEY_A:
      w(isControlPressed ? CTRL_A : (upperCase ? 'A' : 'a'));
      break;
    case PS2_KEY_B:
      w(isControlPressed ? CTRL_B : (upperCase ? 'B' : 'b'));
      break;
    case PS2_KEY_C:
      w(isControlPressed ? CTRL_C : (upperCase ? 'C' : 'c'));
      break;
    case PS2_KEY_D:
      w(isControlPressed ? CTRL_D : (upperCase ? 'D' : 'd'));
      break;
    case PS2_KEY_E:
      w(isControlPressed ? CTRL_E : (upperCase ? 'E' : 'e'));
      break;
    case PS2_KEY_F:
      w(isControlPressed ? CTRL_F : (upperCase ? 'F' : 'f'));
      break;
    case PS2_KEY_G:
      w(isControlPressed ? CTRL_G : (upperCase ? 'G' : 'g'));
      break;
    case PS2_KEY_H:
      w(isControlPressed ? CTRL_H : (upperCase ? 'H' : 'h'));
      break;
    case PS2_KEY_I:
      w(isControlPressed ? CTRL_I : (upperCase ? 'I' : 'i'));
      break;
    case PS2_KEY_J:
      w(isControlPressed ? CTRL_J : (upperCase ? 'J' : 'j'));
      break;
    case PS2_KEY_K:
      w(isControlPressed ? CTRL_K : (upperCase ? 'K' : 'k'));
      break;
    case PS2_KEY_L:
      w(isControlPressed ? CTRL_L : (upperCase ? 'L' : 'l'));
      break;
    case PS2_KEY_M:
      w(isControlPressed ? CTRL_M : (upperCase ? 'M' : 'm'));
      break;
    case PS2_KEY_N:
      w(isControlPressed ? CTRL_N : (upperCase ? 'N' : 'n'));
      break;
    case PS2_KEY_O:
      w(isControlPressed ? CTRL_O : (upperCase ? 'O' : 'o'));
      break;
    case PS2_KEY_P:
      w(isControlPressed ? CTRL_P : (upperCase ? 'P' : 'p'));
      break;
    case PS2_KEY_Q:
      w(isControlPressed ? CTRL_Q : (upperCase ? 'Q' : 'q'));
      break;
    case PS2_KEY_R:
      w(isControlPressed ? CTRL_R : (upperCase ? 'R' : 'r'));
      break;
    case PS2_KEY_S:
      w(isControlPressed ? CTRL_S : (upperCase ? 'S' : 's'));
      break;
    case PS2_KEY_T:
      w(isControlPressed ? CTRL_T : (upperCase ? 'T' : 't'));
      break;
    case PS2_KEY_U:
      w(isControlPressed ? CTRL_U : (upperCase ? 'U' : 'u'));
      break;
    case PS2_KEY_V:
      w(isControlPressed ? CTRL_V : (upperCase ? 'V' : 'v'));
      break;
    case PS2_KEY_W:
      w(isControlPressed ? CTRL_W : (upperCase ? 'W' : 'w'));
      break;
    case PS2_KEY_X:
      w(isControlPressed ? CTRL_X : (upperCase ? 'X' : 'x'));
      break;
    case PS2_KEY_Y:
      w(isControlPressed ? CTRL_Y : (upperCase ? 'Y' : 'y'));
      break;
    case PS2_KEY_Z:
      w(isControlPressed ? CTRL_Z : (upperCase ? 'Z' : 'z'));
      break;
    case PS2_KEY_0:
      w(upperCase ? ')' : '0');
      break;
    case PS2_KEY_1:
      w(upperCase ? '!' : '1');
      break;
    case PS2_KEY_2:
      if (isControlPressed && upperCase) {
        // ^@
        w(CTRL_AT);
      } else {
        w(upperCase ? '@' : '2');
      }
      break;
    case PS2_KEY_3:
      w(upperCase ? '#' : '3');
      break;
    case PS2_KEY_4:
      w(upperCase ? '$' : '4');
      break;
    case PS2_KEY_5:
      w(upperCase ? '%' : '5');
      break;
    case PS2_KEY_6:
      // ^^, ^, or 6
      w(isControlPressed ? CTRL_HAT : (upperCase ? '^' : '6'));
      break;
    case PS2_KEY_7:
      w(upperCase ? '&' : '7');
      break;
    case PS2_KEY_8:
      w(upperCase ? '*' : '8');
      break;
    case PS2_KEY_9:
      w(upperCase ? '(' : '9');
      break;
      // ******** punctuation
    case PS2_KEY_DOT:
      w(upperCase ? '>' : '.');
      break;
    case PS2_KEY_DIV:
      w(upperCase ? '?' : '/');
      break;
    case PS2_KEY_EQUAL:
      w(upperCase ? '+' : '=');
      break;
    case PS2_KEY_MINUS:
      // ^_, _, or -
      w(isControlPressed ? CTRL_SUB : (upperCase ? '_' : '-'));
      break;
    case PS2_KEY_COMMA:
      w(upperCase ? '<' : ',');
      break;
    case PS2_KEY_APOS:
      w(upperCase ? '\"' : '\'');
      break;
    case PS2_KEY_SEMI:
      w(upperCase ? ':' : ';');
      break;
    case PS2_KEY_OPEN_SQ:
      //^[, {, or [
      w(isControlPressed ? CTRL_OPEN_SQ : (upperCase ? '{' : '['));
      break;
    case PS2_KEY_CLOSE_SQ:
      //^[, {, or [
      w(isControlPressed ? CTRL_CLOSE_SQ : (upperCase ? '}' : ']'));
      break;
    case PS2_KEY_SPACE:
      w(' ');
      break;
    case PS2_KEY_BACK:
       w(isControlPressed ? CTRL_BACK : (upperCase ? '|' : '\\'));
      break;
    default:
#ifdef DEBUG
      Serial.print("NOOP");
#endif
      break;
  }
}

/** Write to serial with debug magic*/
void w(int code) {
#ifdef DEBUG
  Serial.print("Output: (");
  Serial.print(code, HEX);
  Serial.print(")\n");
#else
  Serial.write(code);
#endif
}

/*** 
 * reset by pushing the line to 0
 */
void reset() {
#ifdef DEBUG
  Serial.print("RESET\n");
#else
  digitalWrite(MBC_RESET_PIN, LOW);
  delay(500);
  digitalWrite(MBC_RESET_PIN, HIGH);
#endif
}
