/****************************************************************************/
/* Sanyo MBC 550/555 keyboard adapter firmware                              */
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
 * @author MrEppot
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
// computer's usb/serial. Don't enable in final firmware.
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
const int KB_IRQPIN = 3;   // ps2 clock pin. has to be on 2 or 3 (interrupt pin)

// serial configuration as per MBC-555 specifications
const int MBC_BAUD = 1200;          // 1200 baud
const int MBC_SR_CFG = SERIAL_8E2;  // 8 data, 2 stop bits

// output pin for reset
const int MBC_RESET_PIN = 6;  // reset pin to MBC; pulled to low for reset

// macro
#define CHECK_BIT(var, pos) ((var) & (1 << (pos))) > 0

// shifted digit symbols: index 0=')', 1='!', ... 9='('
static const char SHIFTED_DIGITS[] = ")!@#$%^&*(";

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

// forward declarations
void processScanCode();
void processWithControl(int aCharacter);
void handleGraphMode(int character);
void sendToMBC(int code);
void writeWithParityError(int c);
void reset();
void capture();
void disableCaptureMode();

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
static char hexBuffer[3] = "";   // 2 hex chars + null terminator
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
 * @brief Write a character code to the serial output
 *
 * This function sends the translated character code to the MBC.
 * In debug mode, it prints additional information about the output.
 *
 * @param code The character code to be sent
 */
void sendToMBC(int code) {
#ifdef OUTPUT_DEBUG
  Serial.print("Output: (");
  Serial.print(code, HEX);
  Serial.print(")\n");
#else
  Serial.write(code);
#endif
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

  // --- Letters A-Z ---
  if (character >= PS2_KEY_A && character <= PS2_KEY_Z) {
    char base = 'a' + (character - PS2_KEY_A);
    sendToMBC(upperCase ? toupper(base) : base);
    return;
  }

  // --- Digits 0-9 with shifted symbols ---
  if (character >= PS2_KEY_0 && character <= PS2_KEY_9) {
    int idx = character - PS2_KEY_0;
    sendToMBC(upperCase ? SHIFTED_DIGITS[idx] : ('0' + idx));
    return;
  }

  // --- Function keys F1-F10 ---
  if (character >= PS2_KEY_F1 && character <= PS2_KEY_F10) {
    sendToMBC(MBC_F1 + (character - PS2_KEY_F1));
    return;
  }

  // --- Keypad digits 0-9 ---
  if (character >= PS2_KEY_KP0 && character <= PS2_KEY_KP9) {
    sendToMBC('0' + (character - PS2_KEY_KP0));
    return;
  }

  // --- Remaining special keys ---
  switch (character) {
    // main enter
    case PS2_KEY_ENTER:
      sendToMBC(MBC_RETURN);
      break;
      // return (keypad)
    case PS2_KEY_KP_ENTER:
      sendToMBC(MBC_ENTER);
      break;
    case PS2_KEY_DELETE:
      sendToMBC(MBC_BACKSPACE);  // backspace
      break;
    case PS2_KEY_BS:
      sendToMBC(MBC_BACKSPACE);
      break;
    case PS2_KEY_INSERT:
      sendToMBC(MBC_INSERT);
      break;
    case PS2_KEY_TAB:
      sendToMBC(upperCase ? MBC_BACKTAB : MBC_TAB);
      break;
    case PS2_KEY_BREAK:
      Serial.write(CTRL_C);  // todo: not sure what break sends
      break;
    case PS2_KEY_ESC:
      sendToMBC(MBC_ESC);
      break;
    // keypad operators
    case PS2_KEY_KP_EQUAL:
      sendToMBC('=');
      break;
    case PS2_KEY_KP_MINUS:
      sendToMBC('-');
      break;
    case PS2_KEY_KP_PLUS:
      sendToMBC('+');
      break;
    case PS2_KEY_KP_DIV:
      sendToMBC('/');
      break;
    case PS2_KEY_KP_TIMES:
      sendToMBC('*');
      break;
    case PS2_KEY_KP_DOT:
      sendToMBC('.');
      break;
    // navigation keys
    case PS2_KEY_END:
      sendToMBC(MBC_END);
      break;
    case PS2_KEY_PGUP:
      sendToMBC(MBC_PG_UP);
      break;
    case PS2_KEY_PGDN:
      sendToMBC(MBC_PG_DOWN);
      break;
    case PS2_KEY_L_ARROW:
      sendToMBC(MBC_CRS_LEFT);
      break;
    case PS2_KEY_R_ARROW:
      sendToMBC(MBC_CRS_RIGHT);
      break;
    case PS2_KEY_DN_ARROW:
      sendToMBC(MBC_CRS_DOWN);
      break;
    case PS2_KEY_UP_ARROW:
      sendToMBC(MBC_CRS_UP);
      break;
    case PS2_KEY_HOME:
      sendToMBC(MBC_HOME);
      break;
    // punctuation
    case PS2_KEY_DOT:
      sendToMBC(upperCase ? '>' : '.');
      break;
    case PS2_KEY_DIV:
      sendToMBC(upperCase ? '?' : '/');
      break;
    case PS2_KEY_EQUAL:
      sendToMBC(upperCase ? '+' : '=');
      break;
    case PS2_KEY_MINUS:
      sendToMBC(upperCase ? '_' : '-');
      break;
    case PS2_KEY_COMMA:
      sendToMBC(upperCase ? '<' : ',');
      break;
    case PS2_KEY_APOS:
      sendToMBC(upperCase ? '\"' : '\'');
      break;
    case PS2_KEY_SEMI:
      sendToMBC(upperCase ? ':' : ';');
      break;
    case PS2_KEY_OPEN_SQ:
      sendToMBC(upperCase ? '{' : '[');
      break;
    case PS2_KEY_CLOSE_SQ:
      sendToMBC(upperCase ? '}' : ']');
      break;
    case PS2_KEY_SPACE:
      sendToMBC(' ');
      break;
    // backslash
    case PS2_KEY_BACK:
      sendToMBC(upperCase ? '|' : '\\');
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
      sendToMBC(GRAPH_A);
      break;
    default:
      break;
  }
}


/** experimental control */
void processWithControl(int aCharacter) {

  // --- Letters A-Z with CTRL ---
  if (aCharacter >= PS2_KEY_A && aCharacter <= PS2_KEY_Z) {
    // CTRL-ALT-A enables capture mode
    if (aCharacter == PS2_KEY_A && isAltPressed) {
#ifdef DEBUG
      Serial.println("CAP_ON");
#endif
      captureMode = true;
      return;
    }
    // CTRL-C is a special case: sent directly without parity error
    if (aCharacter == PS2_KEY_C) {
      sendToMBC(CTRL_C);
      return;
    }
    char base = 'a' + (aCharacter - PS2_KEY_A);
    writeWithParityError(upperCase ? toupper(base) : base);
    return;
  }

  // --- Function keys F1-F10 with CTRL ---
  if (aCharacter >= PS2_KEY_F1 && aCharacter <= PS2_KEY_F10) {
    sendToMBC(CTRL_F1 + (aCharacter - PS2_KEY_F1));
    return;
  }

  // --- Remaining CTRL special keys ---
  switch (aCharacter) {
    case PS2_KEY_OPEN_SQ:
      sendToMBC(CTRL_OPEN_SQ);
      break;
    case PS2_KEY_CLOSE_SQ:
      sendToMBC(CTRL_CLOSE_SQ);
      break;
    case PS2_KEY_END:
      sendToMBC(CTRL_END);
      break;
    case PS2_KEY_PGDN:
      sendToMBC(CTRL_PGDN);
      break;
    case PS2_KEY_TAB:
      sendToMBC(CTRL_TAB);
      break;
    case PS2_KEY_ENTER:
      sendToMBC(CTRL_ENTER);
      break;
      // return (keypad)
    case PS2_KEY_KP_ENTER:
      sendToMBC(CTRL_ENTER);
      break;
    case PS2_KEY_HOME:
      sendToMBC(CTRL_HOME);
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
  // ensure all pending data is transmitted before switching parity
  Serial.flush();
  Serial.end();
  Serial.begin(MBC_BAUD, SERIAL_8O2);

  sendToMBC(c);

  // ensure the odd-parity byte is fully transmitted before switching back
  Serial.flush();
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
    disableCaptureMode();
    return;
  }

  // Check if character is a valid hexadecimal character, if so, save it to the buffer
  if (isxdigit(character)) {
    if (bufferIndex < (int)(sizeof(hexBuffer) - 1)) {
      hexBuffer[bufferIndex++] = character;
      hexBuffer[bufferIndex] = '\0';  // Null terminate the buffer
    }
  } else {
#ifdef DEBUG
    Serial.println("non hex character");
#endif
    sendToMBC('?');
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
    sendToMBC(hex_value);                   // Output the integer value
    // Reset the buffer, disable capture mode
    disableCaptureMode();
  }
}
