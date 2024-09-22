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
 * @file sanyombc-keyboard.ino
 * @author Joezilla
 * @brief Keyboard adapter firmware for Sanyo MBC 550/555 computers
 *
 * This firmware translates modern PS/2 keyboard input to the format
 * expected by Sanyo MBC 550/555 computers. It handles key presses,
 * special functions, and modifier keys.
 *
 * Key Features:
 * - Translates PS/2 keyboard input to Sanyo MBC-compatible scan codes
 * - Supports special key combinations (e.g., CTRL-ALT-DEL for system reset)
 * - Implements a capture mode for entering arbitrary hex codes
 * - Configurable debug mode for development and troubleshooting
 *
 * Special Key Combinations:
 * - CTRL-ALT-DEL: Sends a reset signal to the MBC
 * - CTRL-ALT-A: Enables capture mode for entering arbitrary hex codes
 *
 * @note Currently supports US-American keyboard layout only.
 * @date September-2024
 * @
 */

// You can activate debug mode that outputs to the serial console
// For debugging and plugging the arduino directly into a
// computer's usb/serial. Don't enable in final firmare.
// #define DEBUG 1 // detail keystroke and program info
// #define OUTPUT_DEBUG 1 // just outputs the final hex characters readable

#include <PS2KeyAdvanced.h>
#include <PS2KeyMap.h>

// sanyo scan codes
#include "scancodes.h"

// standard stuff
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

// ps2 adapter pins
const int KB_DATAPIN = 8;  // ps2 data pin
const int KB_IRQPIN = 3;   // ps2 clxock pin. has to be on 2 or 3 (interrupt pin)

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

/**
 * @brief Initialize hardware and configure keyboard settings
 *
 * This function sets up the reset pin, initializes the PS/2 keyboard,
 * configures keyboard settings, and initializes serial communication
 * with the MBC.
 */
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

/**
 * @brief Convert a hexadecimal string to an integer
 *
 * @param hex Pointer to a null-terminated string containing a hex value
 * @return int The integer value of the hex string
 */
int hex_to_int(const char* hex) {
  int value = 0;
  sscanf(hex, "%x", &value);
  return value;
}

// buffer to store characters
static char hexBuffer[10] = "";  // Buffer to store hex characters
static int bufferIndex = 0;      // Buffer index
bool captureMode = false;

/**
 * @brief Main processing loop
 *
 * Continuously checks for available keyboard input and processes
 * scan codes when they are received.
 */
void loop() {
  if (keyboard.available()) {
    // read the next key
    currentScanCode = keyboard.read();
    if (currentScanCode > 0) {
      processScanCode();
    }
  }
}


/**
 * @brief Process a single PS/2 scan code
 *
 * This function is the core of the keyboard translation logic.
 * It interprets the scan code, handles modifier keys, and
 * translates the input to the appropriate MBC-compatible code.
 */
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
  Serial.print("  Code(");
  Serial.print(currentScanCode & 0xFF, HEX);
  Serial.print(")");
  Serial.print("  Shift(");
  Serial.print(upperCase, HEX);
  Serial.print(")");
  Serial.print("  Crtl(");
  Serial.print(isControlPressed, HEX);
  Serial.print(")");
  Serial.print("  Alt(");
  Serial.print(isAltPressed, HEX);
  Serial.print(")");
  Serial.print("\n");
  Serial.print("  AltGr(");
  Serial.print(isAltGrPressed, HEX);
  Serial.print(")");
  Serial.print("\n");
#endif

  // the character
  int character = currentScanCode & 0xFF;

  // precedence - reboot
  if (isControlPressed && isAltPressed && character == PS2_KEY_DELETE) {
    reset();
    return;
  }

  // ascii mode - capture and release hex
  if (captureMode) {
    capture();
    return;
  }

  // altgr is the same as graph on the sanyo (except not sticky)
  if (isAltGrPressed) {
    handleGraphMode(character);
    return;
  }

  // different pipeline for ctrl characters because it's a lot of
  // special cases
  if (isControlPressed) {
    processWithControl(character);
    return;
  }

  // regular character
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
      Serial.write(CTRL_C);  // todo: not sure what break sends
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
      w(MBC_F1);
      break;
    case PS2_KEY_F2:
      w(MBC_F2);
      break;
    case PS2_KEY_F3:
      w(MBC_F3);
      break;
    case PS2_KEY_F4:
      w(MBC_F4);
      break;
    case PS2_KEY_F5:
      w(MBC_F5);
      break;
    case PS2_KEY_F6:
      w(MBC_F6);
      break;
    case PS2_KEY_F7:
      w(MBC_F7);
      break;
    case PS2_KEY_F8:
      w(MBC_F8);
      break;
    case PS2_KEY_F9:
      w(MBC_F9);
      break;
    case PS2_KEY_F10:
      w(MBC_F10);
      break;
      // ******* regular characters
    case PS2_KEY_A:
      w(upperCase ? 'A' : 'a');
      break;
    case PS2_KEY_B:
      w(upperCase ? 'B' : 'b');
      break;
    case PS2_KEY_C:
      w(upperCase ? 'C' : 'c');
      break;
    case PS2_KEY_D:
      w(upperCase ? 'D' : 'd');
      break;
    case PS2_KEY_E:
      w(upperCase ? 'E' : 'e');
      break;
    case PS2_KEY_F:
      w(upperCase ? 'F' : 'f');
      break;
    case PS2_KEY_G:
      w(upperCase ? 'G' : 'g');
      break;
    case PS2_KEY_H:
      w(upperCase ? 'H' : 'h');
      break;
    case PS2_KEY_I:
      w(upperCase ? 'I' : 'i');
      break;
    case PS2_KEY_J:
      w(upperCase ? 'J' : 'j');
      break;
    case PS2_KEY_K:
      w(upperCase ? 'K' : 'k');
      break;
    case PS2_KEY_L:
      w(upperCase ? 'L' : 'l');
      break;
    case PS2_KEY_M:
      w(upperCase ? 'M' : 'm');
      break;
    case PS2_KEY_N:
      w(upperCase ? 'N' : 'n');
      break;
    case PS2_KEY_O:
      w(upperCase ? 'O' : 'o');
      break;
    case PS2_KEY_P:
      w(upperCase ? 'P' : 'p');
      break;
    case PS2_KEY_Q:
      w(upperCase ? 'Q' : 'q');
      break;
    case PS2_KEY_R:
      w(upperCase ? 'R' : 'r');
      break;
    case PS2_KEY_S:
      w(upperCase ? 'S' : 's');
      break;
    case PS2_KEY_T:
      w(upperCase ? 'T' : 't');
      break;
    case PS2_KEY_U:
      w(upperCase ? 'U' : 'u');
      break;
    case PS2_KEY_V:
      w(upperCase ? 'V' : 'v');
      break;
    case PS2_KEY_W:
      w(upperCase ? 'W' : 'w');
      break;
    case PS2_KEY_X:
      w(upperCase ? 'X' : 'x');
      break;
    case PS2_KEY_Y:
      w(upperCase ? 'Y' : 'y');
      break;
    case PS2_KEY_Z:
      w(upperCase ? 'Z' : 'z');
      break;
    case PS2_KEY_0:
      w(upperCase ? ')' : '0');
      break;
    case PS2_KEY_1:
      w(upperCase ? '!' : '1');
      break;
    case PS2_KEY_2:
      w(upperCase ? '@' : '2');
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
      w(upperCase ? '^' : '6');
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
      w(upperCase ? '_' : '-');
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
      w(upperCase ? '{' : '[');
      break;
    case PS2_KEY_CLOSE_SQ:
      //^[, {, or [
      w(upperCase ? '}' : ']');
      break;
    case PS2_KEY_SPACE:
      w(' ');
      break;
    // backslash
    case PS2_KEY_BACK:
      w(upperCase ? '|' : '\\');
      break;
    default:
#ifdef DEBUG
      Serial.print("NOOP");
#endif
      break;
  }
}

/**
 * @brief Graph mode (same as GRAPH on MBC keyboard)
 *
 * This function send graph characters to the computer per the
 * manual.
 *
 */
void handleGraphMode(int character) {
  // not yet implemented
  switch (character) {
    case PS2_KEY_A:
      w(GRAPH_A);
      break;
    default:
      break;
  }
}

/**
 * @brief Write a character code to the serial output
 *
 * This function sends the translated character code to the MBC.
 * In debug mode, it prints additional information about the output.
 *
 * @param code The character code to be sent
 */
void w(int code) {
#ifdef OUTPUT_DEBUG
  Serial.print("Output: (");
  Serial.print(code, HEX);
  Serial.print(")\n");
#else
  Serial.write(code);
#endif
}

/** experimental control */
void processWithControl(int aCharacter) {

  switch (aCharacter) {
    case PS2_KEY_A:
      // enable capture mode if CTRL-ALT-A is pressed
      if (isControlPressed && isAltPressed) {
#ifdef DEBUG
        Serial.println("CAP_ON");
#endif
        captureMode = true;
      } else {
        writeWithParityError(upperCase ? 'A' : 'a');
      }
      break;
    case PS2_KEY_B:
      writeWithParityError(upperCase ? 'B' : 'b');
      break;
      // ctrl-c is a special case
    case PS2_KEY_C:
      w(CTRL_C);
      break;
    case PS2_KEY_D:
      writeWithParityError(upperCase ? 'D' : 'd');
      break;
    case PS2_KEY_E:
      writeWithParityError(upperCase ? 'E' : 'e');
      break;
    case PS2_KEY_F:
      writeWithParityError(upperCase ? 'F' : 'f');
      break;
    case PS2_KEY_G:
      writeWithParityError(upperCase ? 'G' : 'g');
      break;
    case PS2_KEY_H:
      writeWithParityError(upperCase ? 'H' : 'h');
      break;
    case PS2_KEY_I:
      writeWithParityError(upperCase ? 'I' : 'j');
      break;
    case PS2_KEY_J:
      writeWithParityError(upperCase ? 'J' : 'j');
      break;
    case PS2_KEY_K:
      writeWithParityError(upperCase ? 'K' : 'k');
      break;
    case PS2_KEY_L:
      writeWithParityError(upperCase ? 'L' : 'l');
      break;
    case PS2_KEY_M:
      writeWithParityError(upperCase ? 'M' : 'm');
      break;
    case PS2_KEY_N:
      writeWithParityError(upperCase ? 'N' : 'n');
      break;
    case PS2_KEY_O:
      writeWithParityError(upperCase ? 'O' : 'o');
      break;
    case PS2_KEY_P:
      writeWithParityError(upperCase ? 'P' : 'p');
      break;
    case PS2_KEY_Q:
      writeWithParityError(upperCase ? 'Q' : 'q');
      break;
    case PS2_KEY_R:
      writeWithParityError(upperCase ? 'R' : 'r');
      break;
    case PS2_KEY_S:
      writeWithParityError(upperCase ? 'S' : 's');
      break;
    case PS2_KEY_T:
      writeWithParityError(upperCase ? 'T' : 't');
      break;
    case PS2_KEY_U:
      writeWithParityError(upperCase ? 'U' : 'y');
      break;
    case PS2_KEY_V:
      writeWithParityError(upperCase ? 'V' : 'v');
      break;
    case PS2_KEY_W:
      writeWithParityError(upperCase ? 'W' : 'w');
      break;
    case PS2_KEY_X:
      writeWithParityError(upperCase ? 'X' : 'x');
      break;
    case PS2_KEY_Y:
      writeWithParityError(upperCase ? 'Y' : 'y');
      break;
    case PS2_KEY_Z:
      writeWithParityError(upperCase ? 'Z' : 'z');
      break;
    // function keys -- all special cases again
    case PS2_KEY_F1:
      w(CTRL_F1);
      break;
    case PS2_KEY_F2:
      w(CTRL_F2);
      break;
    case PS2_KEY_F3:
      w(CTRL_F3);
      break;
    case PS2_KEY_F4:
      w(CTRL_F4);
      break;
    case PS2_KEY_F5:
      w(CTRL_F5);
      break;
    case PS2_KEY_F6:
      w(CTRL_F6);
      break;
    case PS2_KEY_F7:
      w(CTRL_F7);
      break;
    case PS2_KEY_F8:
      w(CTRL_F8);
      break;
    case PS2_KEY_F9:
      w(CTRL_F9);
      break;
    case PS2_KEY_F10:
      w(CTRL_F10);
      break;
    case PS2_KEY_OPEN_SQ:
      w(CTRL_OPEN_SQ);
      break;
    case PS2_KEY_CLOSE_SQ:
      //^[, {, or [
      w(CTRL_CLOSE_SQ);
      break;
    case PS2_KEY_END:
      w(CTRL_END);
      break;
    case PS2_KEY_PGDN:
      w(CTRL_PGDN);
      break;
    case PS2_KEY_TAB:
      w(CTRL_TAB);
      break;
    case PS2_KEY_ENTER:
      w(CTRL_ENTER);
      break;
      // return (keypad)
    case PS2_KEY_KP_ENTER:
      w(CTRL_ENTER);
      break;
    case PS2_KEY_HOME:
      w(CTRL_HOME);
      break;
    default:
      break;
  }
}

/**
 * For certain control characters, a parity error has to be triggered.
 * Yes, the parity bit is part of the scan codes.
 */
void writeWithParityError(int c) {
  // trigger a parity error
  Serial.end();
  Serial.begin(MBC_BAUD, SERIAL_8O2);

  w(c);

  // continue
  Serial.end();
  Serial.begin(MBC_BAUD, MBC_SR_CFG);
}

/**
 * @brief Perform a system reset
 *
 * This function triggers a reset of the MBC by pulling the reset
 * line low for a short duration. In debug mode, it only prints
 * a message without actually triggering the reset.
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


/**
 * @brief Disable capture mode and reset related variables
 *
 * This function is called when exiting capture mode or when
 * an invalid input is received during capture mode.
 */
void disableCaptureMode() {
  bufferIndex = 0;
  hexBuffer[0] = '\0';
  captureMode = false;
#ifdef DEBUG
  Serial.println("CAP_OFF");
#endif
}

/**
 * @brief Handle character input during capture mode
 *
 * This function processes input when in capture mode, allowing
 * the user to enter arbitrary hex codes. It handles the buffering
 * of input and conversion of hex strings to character codes.
 */
void capture() {

  int character = currentScanCode & 0xFF;

  // toggle off if CTRL-ALT-A pressed again
  if (character == PS2_KEY_A && isControlPressed && isAltPressed) {
    captureMode = false;
#ifdef DEBUG
    Serial.println("CAP_OFF");
#endif
    return;
  }

  // Check if character is a valid hexadecimal character, if so, save it to the buffer
  if (isxdigit(character)) {
    if (bufferIndex < sizeof(hexBuffer) - 1) {
      hexBuffer[bufferIndex++] = character;
      hexBuffer[bufferIndex] = '\0';  // Null terminate the buffer
    }
  } else {
#ifdef DEBUG
    Serial.println("non hex character");
#endif
    Serial.print('?');
    // Reset the buffer, disable capture mode
    disableCaptureMode();
    return;
  }

  // release buffer if it has 2 characters
  if (bufferIndex > 1) {
    // release buffer
#ifdef DEBUG
    Serial.print("flushing capture buffer (");
    Serial.print(hexBuffer);
    Serial.println(")");
#endif
    int hex_value = hex_to_int(hexBuffer);  // Convert hex string to int
    w(hex_value);                           // Output the integer value
    // Reset the buffer, disable capture mode
    disableCaptureMode();
  }
}