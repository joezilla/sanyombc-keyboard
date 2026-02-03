# Keyboard controller for the Sanyo MBC550/555

## Background
The Sanyo MBC 550/555 was released in the early 80s, based on an 8088 CPU. Unfortunately, the computer doesn't use
standard connectors for keyboards yet, but the protocol is based on a serial protocol and easy to emulate.

This software and hardware allow the connection of a modern PS2 or USB keyboard to the Sanyo.

Tested with an Arduino Nano.

## Keyboard protocol
According to the technical manual, the keyboard operates at 1200 baud, with 8 data bits,
2 stop bits and even parity. Key repeat is handled within the keyboard.

## PS2 Connector
See this article for pinout: https://www.instructables.com/Connect-PS2-Keyboard-to-Arduino/

```
5V     :- Arduino 5V out
Ground :- Arduino GND
Clock  :- Arduino Pin 3  (has to be 3 as it's an interrupt pin)
Data   :- Arduino Pin 8  (can be changed if needed)
```

## Sanyo Keyboard Connector
To connect the circuit to the Arduino, honor the following pinout (looking at the female connector):

```
    ==\/==
  = o3  o1 =
  = o5  o4 =
   =  o2  =
     ====

  o1 = data (to Arduino TX)
  o2 = not used
  o3 = VCC (not used if Arduino powered externally)
  o4 = GND (to Arduino GND)
  o5 = reset (to Arduino pin 6)
 ```

## Flashing Firmware
Easily done via the USB connector. Do not flash firmware while connected to the Sanyo (conflict of the serial port).

## Debug mode
The firmware contains a debug mode (see code to enable and reflash). This will trigger output of the PS2 scan codes
to the serial console (1200 baud, 2 stopbits) and to the Sanyo if connected. Don't enable this if you want to use
the keyboard.

## Reset
The Sanyo uses a dedicated line that has to be pulled to GND to reset. This firmware triggers pin 6 to low
if **CTRL-ALT-DEL** are pressed simultaneously which is not the Sanyo's original behavior but familiar behavior
from other IBM clones.

## Raw ASCII mode
This mode allows submitting any ASCII character code (0x00-0xFF) to the computer. Hitting **CTRL-ALT-A** followed by two
hex characters releases these characters to the computer. Press **CTRL-ALT-A** again to cancel without sending.

## Protocol
The scancodes received correspond mostly to ASCII, with keys producing different scancodes in different
shift, control, and graph states. For example, key 1 produces 31h ('1') unshifted, but 21h ('!') shifted.

The funnest part is that the serial protocol uses the parity bit as part of the scan codes. The computer
expects parity errors to trigger CTRL behavior on most characters.

## Graph Mode (future release)
This firmware will support the GRAPH mode using the right ALT key on the keyboard. Unlike the original MBC, ALT doesn't
toggle, keeping right alt (AltGr) pushed with the appropriate character will send the graphical character:

*Graph*
![Graph keyboard layout](resources/graphmode.png?raw=true "Graph keyboard layout")

*Graph-Shift*
![Graph Shift keyboard layout](resources/graph-shift.png?raw=true "Graph-shift keyboard layout")

*Graph-Ctrl*
![Graph Ctrl keyboard layout](resources/graph-ctrl.png?raw=true "Graph-ctrl keyboard layout")

## Running Tests

The project includes a native test harness that compiles and runs on the host machine (no Arduino hardware required).
It mocks the Arduino Serial, GPIO, and PS2KeyAdvanced APIs to validate all scan code translation logic.

**Requirements:** A C++17 compiler (g++ or clang++).

```bash
cd test
make test
```

This compiles `test_keyboard.cpp` and runs 31 tests covering:
- Regular characters (A-Z, 0-9, shifted symbols)
- Function keys (F1-F10) and CTRL+function key combos
- Navigation keys, keypad, punctuation
- CTRL+letter combinations (parity error signaling)
- Special combos (CTRL-ALT-DEL reset, CTRL-ALT-A capture mode)
- Capture mode hex entry and error handling
- Serial parity flush/switch sequencing

To clean build artifacts: `make clean`

## Feature Status

| Feature | Status |
|---------|--------|
| Regular character translation (A-Z, 0-9, punctuation) | Working |
| Shift and CapsLock support | Working |
| Function keys F1-F10 | Working |
| CTRL key combinations (parity error signaling) | Working |
| CTRL+function keys | Working |
| Navigation keys (arrows, Home, End, PgUp, PgDn) | Working |
| Keypad digits and operators | Working |
| System reset (CTRL-ALT-DEL) | Working |
| Raw ASCII capture mode (CTRL-ALT-A) | Working |
| Graph mode (AltGr + key) | Partial (only GRAPH_A) |

## Limitations
+ Sanyo graphics characters not yet fully implemented (only GRAPH_A; remaining graph key mappings are planned)
