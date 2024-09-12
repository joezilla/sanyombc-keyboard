== Keyboard controller for the Sanyo MBC550/555

== Background
The Sanyo MBC 550/555 was released in the early 80s, based on an 8088 CPU.

Unfortunately, the computer doesn't use standard connectors for keyboards yet, but the protocol is based on a serial protocol.

This software and hardware allow the connection of a modern ps2 or USB keyboard to the Sanyo.

== Keyboard protocol
According to the technical manual, the keyboard operates at 1200 baud, with 8 data bits, 
2 stop bits and even parity. Key repeat is handled within the keyboard.

== Pinout

        '
4 DAT o   o 1 VCC
3 GND o   o 2 reset
       + o

== Details

== Protocol
Reverse engineering of keyboard protocol based on https://github.com/toncho11/Sanyo_MBC_555_keyboard_converter

The scancodes received appear to correspond to ASCII, with keys producing 
different scancodes in different shift states. For example, key 1 produces 
31h ('1') unshifted, but 21h ('!') shifted. Scancodes are:

00: -
01: End
02: Page Down
03: Scroll Lock / Break
04: Page Up
05: Backtab?
06: -
07: Ins / Print Screen
08: Backspace
09: Tab
0A: Enter
0B: Home
0C: -
0D: Return
0E: -
0F: -
10-19: F1-F10
1A: -
1B: Esc
1C: Cursor left
1D: Cursor right
1E: Cursor up
1F: Cursor down
20-7E: ASCII characters SPACE to ~
7F: Del
80-FF: Graphical characters

The keyboard translation table (which converts Sanyo scancodes to PC-compatible scancode+character pairs) 
is at the start of the ROM (0FE000h). It contains 512 words; the first 256 are for when the 
CTRL key is up, and the second 256 for when it is down.