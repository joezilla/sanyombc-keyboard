# Keyboard controller for the Sanyo MBC550/555

## Background
The Sanyo MBC 550/555 was released in the early 80s, based on an 8088 CPU. Unfortunately, the computer doesn't use 
standard connectors for keyboards yet, but the protocol is based on a serial protocol and easy to emulate.

This software and hardware allow the connection of a modern PS2 or USB keyboard to the Sanyo.

Tested with an Arduiono Nano.

## Keyboard protocol
According to the technical manual, the keyboard operates at 1200 baud, with 8 data bits, 
2 stop bits and even parity. Key repeat is handled within the keyboard.

## PS2 Connector
See this article for pinout: https://www.instructables.com/Connect-PS2-Keyboard-to-Arduino/

5V     :- Arduino 5V out
Ground :- Arduino GND
Clock  :- Arduino Pin 3  (has to be 3 as it's an interrupt pin)
Data   :- Arduino Pin 8  (can be changed if needed)

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
The Sanyo uses a dedicated line that has to be pulled to GND to reset. This firmare triggers pin 6 to low 
if CTRL-ALT-DEL are pressed simultaneously which is not the Sanyo's original behavior but familiar from DOS days.

## Protocol
Reverse engineering of keyboard protocol based on https://github.com/toncho11/Sanyo_MBC_555_keyboard_converter. 

The scancodes received appear to correspond to ASCII, with keys producing different scancodes in different 
shift states. For example, key 1 produces 31h ('1') unshifted, but 21h ('!') shifted. 

## Limitations
Control key behavior not yet working.
