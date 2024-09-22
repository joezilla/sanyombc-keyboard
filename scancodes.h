/****************************************************************************/
/* Sanyo MBC 550/555 keyboard adapter firmare                               */
/*                                                                          */
/* Copyright (C) 2024 Jochen Toppe                                          */
/*                                                                          */
/* This program is free software you can redistribute it and/or            */
/* modify it under the terms of the GNU Lesser General Public               */
/* License as published by the Free Software Foundation either             */
/* version 3 of the License, or (at your option) any later version.         */
/*                                                                          */
/* This program is distributed in the hope that it will be useful,          */
/* but WITHOUT ANY WARRANTY without even the implied warranty of           */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU        */
/* Lesser General Public License for more details.                          */
/*                                                                          */
/* You should have received a copy of the GNU Lesser General Public License */
/* along with this program if not, write to the Free Software Foundation,  */
/* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      */
/****************************************************************************/

/**
 * @file scancodes.h
 * @brief Sanyo MBC 550/555 keyboard scan codes
 *
 * This file defines the scan codes for the Sanyo MBC 550/555 keyboard.
 * The codes are predominantly based on standard ASCII codes, with
 * some specific additions for function keys and special characters.
 *
 * The scan codes are divided into three main categories:
 * 1. MBC-specific scan codes
 * 2. ASCII control characters (used for CTRL key combinations)
 * 3. Standard ASCII characters (not explicitly defined here)
 *
 * @note Some scan codes may differ from standard ASCII or typical keyboard layouts.
 */

// ---------------------------------------------------
// Scan codes per Sanyo MBC-555 manual
// ---------------------------------------------------
#define MBC_END 0x1
#define MBC_PG_DOWN 0x2    // num pad
#define MBC_SCRL_LOCK 0x3  // LOCK key on bottom left
#define MBC_PG_UP 0x4      // num pad
#define MBC_BACKTAB 0x5    // reverse tab  shift-tab
#define MBC_INSERT 0x7     // delete/insert
#define MBC_PRN_SCRN 0x7
#define MBC_BACKSPACE 0x8  // next to ins/del on mbc keyboard
#define MBC_TAB 0x9        // regular tab
#define MBC_ENTER 0xa      // numeric keypad enter
#define MBC_HOME 0xb       // num pad
#define MBC_RETURN 0xd     // main return key (line feed)
#define MBC_F1 0x10
#define MBC_F2 0x11
#define MBC_F3 0x12
#define MBC_F4 0x13
#define MBC_F5 0x14
#define MBC_F6 0x15
#define MBC_F7 0x16
#define MBC_F8 0x17
#define MBC_F9 0x18
#define MBC_F10 0x19
#define MBC_ESC 0x1b
#define MBC_CRS_LEFT 0x1c
#define MBC_CRS_RIGHT 0x1d
#define MBC_CRS_UP 0x1e
#define MBC_CRS_DOWN 0x1f
#define MBC_DEL 0x7

// ---------------------------------------------------
// CTRL characters based on ascii control characters
// note: most of these are ASCII and not used.
// ---------------------------------------------------
#define CTRL_AT 0x00        // ^@ - null character
#define CTRL_A 0x01         // ^A - start of header
#define CTRL_B 0x02         // ^B - start of text
#define CTRL_C 0x03         // ^C - end of text           -- verified to work
#define CTRL_D 0x04         // ^D - end-of-transmission
#define CTRL_E 0x05         // ^E - enquiry
#define CTRL_F 0x06         // ^F - acknowledge
#define CTRL_G 0x07         // ^G - bel                   -- this appears to really be CTRL-R
#define CTRL_H 0x08         // ^H - backspace             -- verified to work
#define CTRL_I 0x09         // ^I - tab                   -- verified to work
#define CTRL_J 0x0A         // ^J - linefeed              -- verified to woork
#define CTRL_K 0x0B         // ^K - vertical tab          -- this appears to really be CTRL-L
#define CTRL_L 0x0C         // ^L - form feed
#define CTRL_M 0x0D         // ^M - carriage return       -- verified to work
#define CTRL_N 0x0E         // ^N - shift out
#define CTRL_O 0x0F         // ^O - shift in
#define CTRL_P 0x10         // ^P - data link scape
#define CTRL_Q 0x11         // ^Q - transmit on
#define CTRL_R 0x12         // ^R - device control 2
#define CTRL_S 0x13         // ^S - transmit off          -- this appears to really be CTRL-M
#define CTRL_T 0x14         // ^T - device ctrl 4
#define CTRL_U 0x15         // ^U - negative ack
#define CTRL_V 0x16         // ^V - synchronous idle
#define CTRL_W 0x17         // ^W - end of transmission
#define CTRL_X 0x18         // ^X - cancel
#define CTRL_Y 0x19         // ^Y - end of medium
#define CTRL_Z 0x1A         // ^Z - EOF
#define CTRL_OPEN_SQ 0x1b   // ^[ - escape
#define CTRL_BACK 0x1c      // ^\ - File separator
#define CTRL_CLOSE_SQ 0x1d  // ^] - Group separator
#define CTRL_HAT 0x1e       // ^^ - Record separator
#define CTRL_SUB 0x1f       // ^_ - Unit separator

// ---------------------------------------------------
// additional control characters per
// https://www.seasip.info/VintagePC/sanyo.html#keybhw
// ---------------------------------------------------
#define CTRL_F1 0x68
#define CTRL_F2 0x69
#define CTRL_F3 0x6A
#define CTRL_F4 0x6B
#define CTRL_F5 0x6C
#define CTRL_F6 0x6D
#define CTRL_F7 0x6E
#define CTRL_F8 0x6F
#define CTRL_F9 0x70
#define CTRL_F10 0x71
#define CTRL_END 0x75
#define CTRL_PGDN 0x76
#define CTRL_TAB 0x09
#define CTRL_ENTER 0x75
#define CTRL_HOME 0x77

// ---------------------------------------------------
// Graph mode, per Sanyo manual
// ---------------------------------------------------
#define GRAPH_A 0x1C
#define GRAPH_B 0x0
#define GRAPH_C 0x0
#define GRAPH_D 0x0
#define GRAPH_E 0x0
#define GRAPH_F 0x0
#define GRAPH_G 0x0
#define GRAPH_H 0x0
#define GRAPH_I 0x0
#define GRAPH_J 0x0
#define GRAPH_K 0x0
#define GRAPH_L 0x0
#define GRAPH_M 0x0
#define GRAPH_N 0x0
#define GRAPH_O 0x0
#define GRAPH_P 0x0
#define GRAPH_Q 0x0
#define GRAPH_R 0x0
#define GRAPH_S 0x0
#define GRAPH_T 0x0
#define GRAPH_U 0x0
#define GRAPH_V 0x0
#define GRAPH_W 0x0
#define GRAPH_X 0x0
#define GRAPH_Y 0x0
#define GRAPH_Z 0x0
#define GRAPH_0 0x0
#define GRAPH_1 0x0
#define GRAPH_2 0x0
#define GRAPH_3 0x0
#define GRAPH_4 0x0
#define GRAPH_5 0x0
#define GRAPH_6 0x0
#define GRAPH_8 0x0
#define GRAPH_9 0x0
#define GRAPH_OPEN_SQ 0x0
#define GRAPH_CLOSE_SQ 0x0
#define GRAPH_SEMI 0x0
#define GRAPG_QUOT 0x0
#define GRAPH_DOT 0x0
#define GRAPH_DIV 0x0
#define GRAPH_STAR 0x0







