/*
 * Amiga 500 Keyboard to USB HID Converter Arduino Leonardo
 * Copyright (c) 2024 by Luka "Void" MIT License
 * GitHub: https://github.com/arvvoid/
 * Contact: luka@lukavoid.xyz
 * Repository: https://github.com/arvvoid/Amiga500-USB-Keyboard-Leonardo
 * Original code and inspiration by olaf, Steve_Reaver (taken from https://forum.arduino.cc/index.php?topic=139358.15)
 *
 * This sketch converts an original Amiga 500 keyboard to a standard USB HID
 * keyboard using an Arduino Leonardo. It includes support for joystick inputs
 * and special function keys.
 * 
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Readme: https://github.com/arvvoid/Amiga500-USB-Keyboard-Leonardo/blob/main/README.md
 */
#include <Keyboard.h>
#include <HID.h>
#include <CircularBuffer.hpp>

// Preprocessor flag to enable or disable debug mode
// Debug mode provides verbose console output at every step.
#define DEBUG_MODE 0

// Preprocessor flag to enable or disable joystick support
// Joystick support is only required if you are going to attach DB9 connectors and use legacy Amiga joysticks with the controller.
#define ENABLE_JOYSTICKS 0

#define ENABLE_MULTIMEDIA_KEYS 1 // Enable multimedia keys (volume, play/pause, etc.)

#define MAX_MACRO_LENGTH 50  // Maximum number of key events in a macro (9bit x MAX_MACRO_LENGTH) x MACRO_SLOTS 
                              // of memory reserved. If PERSISTENT_MACRO is 1, macros get saved to EEPROM witch is 1kb
                              // so this should never go over 1000bytes to leave some room for checksum and other metadata

#define MACRO_SLOTS 5
// Macro Playback Timing and Behavior:
// 1. Macro playback occurs at fixed intervals, defined in milliseconds.
// 2. Active macro slots play sequentially, with a maximum of 2 macros running concurrently.
// 3. During each playback cycle:
//    - The maximum number of keys are played until the key report is full
//      (6 standard keys + modifiers).
//    - Live pressed keys take priority over macro-generated keys when filling the report.
//      If live keys occupy the report, macro playback may be delayed to accommodate them.
// 4. Key releases from macros are queued and processed after live keys and new key presses.
#define MACRO_DELAY 15
#define PERSISTENT_MACRO 1 // Save macros to EEPROM
#define MACRO_SAVE_VERSION 2 // Macro save version, increment this if you change the macro structure
#define MAX_CONCURENT_MACROS 2 // Maximum number of macros that can be played at the same time

#define DELAYED_QUEUE_DELAY 5 // Delayed key queue delay in milliseconds (primary for programmatic or macro key releases)

#define QUEUE_SIZE 50 + (MAX_CONCURENT_MACROS * MAX_MACRO_LENGTH ) // Size of the key event queues, its (QUEUE_SIZE*9bit) x 2 of memory reserved in currect code
#define RELEASE_QUEUE_SIZE 50 // Size of the delayed key event queue for programmatic and macro key releases
#define QUEUE_SIZE_MMKEY 5 // Size of the multimedia key event queue
#define CONSUME_KEY_TIMER 100 // Consume key timer in microseconds

#if PERSISTENT_MACRO
  #include <EEPROM.h>
  #define EEPROM_START_ADDRESS 0
#endif

// Define bit masks for keyboard and joystick inputs
#define BITMASK_A500CLK 0b00010000 // IO 8
#define BITMASK_A500SP 0b00100000  // IO 9
#define BITMASK_A500RES 0b01000000 // IO 10
#if ENABLE_JOYSTICKS
  #define BITMASK_JOY1 0b10011111    // IO 0..4,6
  #define BITMASK_JOY2 0b11110011    // IO A0..A5
#endif

enum AmigaKeys {   // ROM Default (USA0) and USA1 Console Key Mapping
  // 0x00 - 0x0F
  AMIGA_KEY_TILDE = 0x00,             // ` ~
  AMIGA_KEY_1 = 0x01,                 // 1 !
  AMIGA_KEY_2 = 0x02,                 // 2 @
  AMIGA_KEY_3 = 0x03,                 // 3 #
  AMIGA_KEY_4 = 0x04,                 // 4 $
  AMIGA_KEY_5 = 0x05,                 // 5 %
  AMIGA_KEY_6 = 0x06,                 // 6 ^
  AMIGA_KEY_7 = 0x07,                 // 7 &
  AMIGA_KEY_8 = 0x08,                 // 8 *
  AMIGA_KEY_9 = 0x09,                 // 9 (
  AMIGA_KEY_0 = 0x0A,                 // 0 )
  AMIGA_KEY_MINUS = 0x0B,             // - _
  AMIGA_KEY_EQUAL = 0x0C,             // = +
  AMIGA_KEY_PIPE = 0x0D,              // |
  AMIGA_KEY_UNDEFINED_0E = 0x0E,      // Undefined
  AMIGA_KEY_NUMPAD_0 = 0x0F,          // Numpad 0

  // 0x10 - 0x1F
  AMIGA_KEY_Q = 0x10,                 // Q
  AMIGA_KEY_W = 0x11,                 // W
  AMIGA_KEY_E = 0x12,                 // E
  AMIGA_KEY_R = 0x13,                 // R
  AMIGA_KEY_T = 0x14,                 // T
  AMIGA_KEY_Y = 0x15,                 // Y
  AMIGA_KEY_U = 0x16,                 // U
  AMIGA_KEY_I = 0x17,                 // I
  AMIGA_KEY_O = 0x18,                 // O
  AMIGA_KEY_P = 0x19,                 // P
  AMIGA_KEY_LEFT_BRACKET = 0x1A,      // [ {
  AMIGA_KEY_RIGHT_BRACKET = 0x1B,     // ] }
  AMIGA_KEY_UNDEFINED_1C = 0x1C,      // Undefined
  AMIGA_KEY_NUMPAD_1 = 0x1D,          // Numpad 1
  AMIGA_KEY_NUMPAD_2 = 0x1E,          // Numpad 2
  AMIGA_KEY_NUMPAD_3 = 0x1F,          // Numpad 3

  // 0x20 - 0x2F
  AMIGA_KEY_A = 0x20,                 // A
  AMIGA_KEY_S = 0x21,                 // S
  AMIGA_KEY_D = 0x22,                 // D
  AMIGA_KEY_F = 0x23,                 // F
  AMIGA_KEY_G = 0x24,                 // G
  AMIGA_KEY_H = 0x25,                 // H
  AMIGA_KEY_J = 0x26,                 // J
  AMIGA_KEY_K = 0x27,                 // K
  AMIGA_KEY_L = 0x28,                 // L
  AMIGA_KEY_SEMICOLON = 0x29,         // ; :
  AMIGA_KEY_APOSTROPHE = 0x2A,        // ' "
  AMIGA_KEY_NON_US_TILDE = 0x2B,      // Non-US #
  AMIGA_KEY_UNDEFINED_2C = 0x2C,      // Undefined
  AMIGA_KEY_NUMPAD_4 = 0x2D,          // Numpad 4
  AMIGA_KEY_NUMPAD_5 = 0x2E,          // Numpad 5
  AMIGA_KEY_NUMPAD_6 = 0x2F,          // Numpad 6

  // 0x30 - 0x3F
  AMIGA_KEY_NON_US_BACKSLASH = 0x30,  // Non-US \ (Not on most US keyboards)
  AMIGA_KEY_Z = 0x31,                 // Z
  AMIGA_KEY_X = 0x32,                 // X
  AMIGA_KEY_C = 0x33,                 // C
  AMIGA_KEY_V = 0x34,                 // V
  AMIGA_KEY_B = 0x35,                 // B
  AMIGA_KEY_N = 0x36,                 // N
  AMIGA_KEY_M = 0x37,                 // M
  AMIGA_KEY_COMMA = 0x38,             // , <
  AMIGA_KEY_PERIOD = 0x39,            // . >
  AMIGA_KEY_SLASH = 0x3A,             // / ?
  AMIGA_KEY_UNDEFINED_3B = 0x3B,      // Undefined
  AMIGA_KEY_NUMPAD_PERIOD = 0x3C,     // Numpad .
  AMIGA_KEY_NUMPAD_7 = 0x3D,          // Numpad 7
  AMIGA_KEY_NUMPAD_8 = 0x3E,          // Numpad 8
  AMIGA_KEY_NUMPAD_9 = 0x3F,          // Numpad 9

  // 0x40 - 0x4F
  AMIGA_KEY_SPACE = 0x40,             // Spacebar
  AMIGA_KEY_BACKSPACE = 0x41,         // Backspace
  AMIGA_KEY_TAB = 0x42,               // Tab
  AMIGA_KEY_NUMPAD_ENTER = 0x43,      // Numpad Enter
  AMIGA_KEY_RETURN = 0x44,            // Return
  AMIGA_KEY_ESCAPE = 0x45,            // Escape
  AMIGA_KEY_DELETE = 0x46,            // Delete
  AMIGA_KEY_UNDEFINED_47 = 0x47,      // Undefined
  AMIGA_KEY_UNDEFINED_48 = 0x48,      // Undefined
  AMIGA_KEY_UNDEFINED_49 = 0x49,      // Undefined
  AMIGA_KEY_NUMPAD_MINUS = 0x4A,      // Numpad -
  AMIGA_KEY_UNDEFINED_4B = 0x4B,      // Undefined
  AMIGA_KEY_ARROW_UP = 0x4C,          // Up Arrow
  AMIGA_KEY_ARROW_DOWN = 0x4D,        // Down Arrow
  AMIGA_KEY_ARROW_RIGHT = 0x4E,       // Right Arrow
  AMIGA_KEY_ARROW_LEFT = 0x4F,        // Left Arrow

  // 0x50 - 0x5F
  AMIGA_KEY_F1 = 0x50,                // F1
  AMIGA_KEY_F2 = 0x51,                // F2
  AMIGA_KEY_F3 = 0x52,                // F3
  AMIGA_KEY_F4 = 0x53,                // F4
  AMIGA_KEY_F5 = 0x54,                // F5
  AMIGA_KEY_F6 = 0x55,                // F6
  AMIGA_KEY_F7 = 0x56,                // F7
  AMIGA_KEY_F8 = 0x57,                // F8
  AMIGA_KEY_F9 = 0x58,                // F9
  AMIGA_KEY_F10 = 0x59,               // F10
  AMIGA_KEY_NUMPAD_NUMLOCK_LPAREN = 0x5A, // Numpad ( Numlock
  AMIGA_KEY_NUMPAD_SCRLOCK_RPAREN = 0x5B, // Numpad ) Scroll Lock
  AMIGA_KEY_NUMPAD_SLASH = 0x5C,      // Numpad /
  AMIGA_KEY_NUMPAD_ASTERISK_PTRSCR = 0x5D,   // Numpad * PtrScr
  AMIGA_KEY_NUMPAD_PLUS = 0x5E,       // Numpad +
  AMIGA_KEY_HELP = 0x5F,              // Help

  // 0x60 - 0x67 (Modifiers and special keys)
  AMIGA_KEY_SHIFT_LEFT = 0x60,        // Left Shift
  AMIGA_KEY_SHIFT_RIGHT = 0x61,       // Right Shift
  AMIGA_KEY_CAPS_LOCK = 0x62,         // Caps Lock
  AMIGA_KEY_CONTROL_LEFT = 0x63,      // Left Control
  AMIGA_KEY_ALT_LEFT = 0x64,          // Left Alt
  AMIGA_KEY_ALT_RIGHT = 0x65,         // Right Alt
  AMIGA_KEY_AMIGA_LEFT = 0x66,        // Left Amiga (Windows key)
  AMIGA_KEY_AMIGA_RIGHT = 0x67,       // Right Amiga
  AMIGA_KEY_COUNT                     // Total number of Amiga keys
};

enum MultimediaKey {
  MMKEY_NEXT_TRACK   = 1 << 0, // 0x01
  MMKEY_PREV_TRACK   = 1 << 1, // 0x02
  MMKEY_STOP         = 1 << 2, // 0x04
  MMKEY_PLAY_PAUSE   = 1 << 3, // 0x08
  MMKEY_MUTE         = 1 << 4, // 0x10
  MMKEY_VOLUME_UP    = 1 << 5, // 0x20
  MMKEY_VOLUME_DOWN  = 1 << 6  // 0x40
};

const uint8_t keyTable[AMIGA_KEY_COUNT] = { // US Keyboard Layout Amiga500 to HID

    // 0x00 - 0x0F
    0x35, // 0x00: AMIGA_KEY_TILDE (` ~)                 -> HID KEY_GRAVE_ACCENT_AND_TILDE
    0x1E, // 0x01: AMIGA_KEY_1 (1 ! )                    -> HID KEY_1
    0x1F, // 0x02: AMIGA_KEY_2 (2 @ )                    -> HID KEY_2
    0x20, // 0x03: AMIGA_KEY_3 (3 # )                    -> HID KEY_3
    0x21, // 0x04: AMIGA_KEY_4 (4 $ )                    -> HID KEY_4
    0x22, // 0x05: AMIGA_KEY_5 (5 % )                    -> HID KEY_5
    0x23, // 0x06: AMIGA_KEY_6 (6 ^ )                    -> HID KEY_6
    0x24, // 0x07: AMIGA_KEY_7 (7 & )                    -> HID KEY_7
    0x25, // 0x08: AMIGA_KEY_8 (8 * )                    -> HID KEY_8
    0x26, // 0x09: AMIGA_KEY_9 (9 ( )                    -> HID KEY_9
    0x27, // 0x0A: AMIGA_KEY_0 (0 ) )                    -> HID KEY_0
    0x2D, // 0x0B: AMIGA_KEY_MINUS (- _ )                -> HID KEY_MINUS_AND_UNDERSCORE
    0x2E, // 0x0C: AMIGA_KEY_EQUAL (= + )                -> HID KEY_EQUAL_AND_PLUS
    0x31, // 0x0D: AMIGA_KEY_PIPE (| )                   -> HID KEY_BACKSLASH_AND_PIPE
    0x00, // 0x0E: AMIGA_KEY_UNDEFINED_0E                -> Undefined
    0x62, // 0x0F: AMIGA_KEY_NUMPAD_0                    -> HID KEY_KEYPAD_0

    // 0x10 - 0x1F
    0x14, // 0x10: AMIGA_KEY_Q                           -> HID KEY_Q
    0x1A, // 0x11: AMIGA_KEY_W                           -> HID KEY_W
    0x08, // 0x12: AMIGA_KEY_E                           -> HID KEY_E
    0x15, // 0x13: AMIGA_KEY_R                           -> HID KEY_R
    0x17, // 0x14: AMIGA_KEY_T                           -> HID KEY_T
    0x1C, // 0x15: AMIGA_KEY_Y                           -> HID KEY_Y
    0x18, // 0x16: AMIGA_KEY_U                           -> HID KEY_U
    0x0C, // 0x17: AMIGA_KEY_I                           -> HID KEY_I
    0x12, // 0x18: AMIGA_KEY_O                           -> HID KEY_O
    0x13, // 0x19: AMIGA_KEY_P                           -> HID KEY_P
    0x2F, // 0x1A: AMIGA_KEY_LEFT_BRACKET ([ { )         -> HID KEY_LEFT_BRACKET_AND_LEFT_BRACE
    0x30, // 0x1B: AMIGA_KEY_RIGHT_BRACKET (] } )        -> HID KEY_RIGHT_BRACKET_AND_RIGHT_BRACE
    0x00, // 0x1C: AMIGA_KEY_UNDEFINED_1C                -> Undefined
    0x59, // 0x1D: AMIGA_KEY_NUMPAD_1                    -> HID KEY_KEYPAD_1_END
    0x5A, // 0x1E: AMIGA_KEY_NUMPAD_2                    -> HID KEY_KEYPAD_2_DOWN_ARROW
    0x5B, // 0x1F: AMIGA_KEY_NUMPAD_3                    -> HID KEY_KEYPAD_3_PAGE_DOWN

    // 0x20 - 0x2F
    0x04, // 0x20: AMIGA_KEY_A                           -> HID KEY_A
    0x16, // 0x21: AMIGA_KEY_S                           -> HID KEY_S
    0x07, // 0x22: AMIGA_KEY_D                           -> HID KEY_D
    0x09, // 0x23: AMIGA_KEY_F                           -> HID KEY_F
    0x0A, // 0x24: AMIGA_KEY_G                           -> HID KEY_G
    0x0B, // 0x25: AMIGA_KEY_H                           -> HID KEY_H
    0x0D, // 0x26: AMIGA_KEY_J                           -> HID KEY_J
    0x0E, // 0x27: AMIGA_KEY_K                           -> HID KEY_K
    0x0F, // 0x28: AMIGA_KEY_L                           -> HID KEY_L
    0x33, // 0x29: AMIGA_KEY_SEMICOLON (; : )            -> HID KEY_SEMICOLON_AND_COLON
    0x34, // 0x2A: AMIGA_KEY_APOSTROPHE (' " )           -> HID KEY_APOSTROPHE_AND_QUOTE
    0x32, // 0x2B: AMIGA_KEY_NON_US_TILDE (# ~ )         -> HID KEY_NON_US_HASH_AND_TILDE
    0x00, // 0x2C: AMIGA_KEY_UNDEFINED_2C                -> Undefined
    0x5C, // 0x2D: AMIGA_KEY_NUMPAD_4                    -> HID KEY_KEYPAD_4_LEFT_ARROW
    0x5D, // 0x2E: AMIGA_KEY_NUMPAD_5                    -> HID KEY_KEYPAD_5
    0x5E, // 0x2F: AMIGA_KEY_NUMPAD_6                    -> HID KEY_KEYPAD_6_RIGHT_ARROW

    // 0x30 - 0x3F
    0x64, // 0x30: AMIGA_KEY_NON_US_BACKSLASH (\ | )     -> HID KEY_NON_US_BACKSLASH_AND_PIPE
    0x1D, // 0x31: AMIGA_KEY_Z                           -> HID KEY_Z
    0x1B, // 0x32: AMIGA_KEY_X                           -> HID KEY_X
    0x06, // 0x33: AMIGA_KEY_C                           -> HID KEY_C
    0x19, // 0x34: AMIGA_KEY_V                           -> HID KEY_V
    0x05, // 0x35: AMIGA_KEY_B                           -> HID KEY_B
    0x11, // 0x36: AMIGA_KEY_N                           -> HID KEY_N
    0x10, // 0x37: AMIGA_KEY_M                           -> HID KEY_M
    0x36, // 0x38: AMIGA_KEY_COMMA (, < )                -> HID KEY_COMMA_AND_LESS_THAN
    0x37, // 0x39: AMIGA_KEY_PERIOD (. > )               -> HID KEY_PERIOD_AND_GREATER_THAN
    0x38, // 0x3A: AMIGA_KEY_SLASH (/ ? )                -> HID KEY_SLASH_AND_QUESTION_MARK
    0x00, // 0x3B: AMIGA_KEY_UNDEFINED_3B                -> Undefined
    0x63, // 0x3C: AMIGA_KEY_NUMPAD_PERIOD               -> HID KEY_KEYPAD_PERIOD
    0x5F, // 0x3D: AMIGA_KEY_NUMPAD_7                    -> HID KEY_KEYPAD_7_HOME
    0x60, // 0x3E: AMIGA_KEY_NUMPAD_8                    -> HID KEY_KEYPAD_8_UP_ARROW
    0x61, // 0x3F: AMIGA_KEY_NUMPAD_9                    -> HID KEY_KEYPAD_9_PAGE_UP

    // 0x40 - 0x4F
    0x2C, // 0x40: AMIGA_KEY_SPACE                       -> HID KEY_SPACE
    0x2A, // 0x41: AMIGA_KEY_BACKSPACE                   -> HID KEY_BACKSPACE
    0x2B, // 0x42: AMIGA_KEY_TAB                         -> HID KEY_TAB
    0x58, // 0x43: AMIGA_KEY_NUMPAD_ENTER                -> HID KEY_KEYPAD_ENTER
    0x28, // 0x44: AMIGA_KEY_RETURN                      -> HID KEY_ENTER (Return)
    0x29, // 0x45: AMIGA_KEY_ESCAPE                      -> HID KEY_ESCAPE
    0x4C, // 0x46: AMIGA_KEY_DELETE                      -> HID KEY_DELETE
    0x44, // 0x47: AMIGA_KEY_UNDEFINED_47                -> Undefined Used for HID F11
    0x45, // 0x48: AMIGA_KEY_UNDEFINED_48                -> Undefined Used for HID F12
    0x46, // 0x49: AMIGA_KEY_UNDEFINED_49                -> Undefined Used for HID PrtScr
    0x56, // 0x4A: AMIGA_KEY_NUMPAD_MINUS                -> HID KEY_KEYPAD_MINUS
    0x00, // 0x4B: AMIGA_KEY_UNDEFINED_4B                -> Undefined
    0x52, // 0x4C: AMIGA_KEY_ARROW_UP                    -> HID KEY_UP_ARROW
    0x51, // 0x4D: AMIGA_KEY_ARROW_DOWN                  -> HID KEY_DOWN_ARROW
    0x4F, // 0x4E: AMIGA_KEY_ARROW_RIGHT                 -> HID KEY_RIGHT_ARROW
    0x50, // 0x4F: AMIGA_KEY_ARROW_LEFT                  -> HID KEY_LEFT_ARROW

    // 0x50 - 0x5F
    0x3A, // 0x50: AMIGA_KEY_F1                          -> HID KEY_F1
    0x3B, // 0x51: AMIGA_KEY_F2                          -> HID KEY_F2
    0x3C, // 0x52: AMIGA_KEY_F3                          -> HID KEY_F3
    0x3D, // 0x53: AMIGA_KEY_F4                          -> HID KEY_F4
    0x3E, // 0x54: AMIGA_KEY_F5                          -> HID KEY_F5
    0x3F, // 0x55: AMIGA_KEY_F6                          -> HID KEY_F6
    0x40, // 0x56: AMIGA_KEY_F7                          -> HID KEY_F7
    0x41, // 0x57: AMIGA_KEY_F8                          -> HID KEY_F8
    0x42, // 0x58: AMIGA_KEY_F9                          -> HID KEY_F9
    0x43, // 0x59: AMIGA_KEY_F10                         -> HID KEY_F10
    0x53, // 0x5A: AMIGA_KEY_NUMPAD_NUMLOCK_LPAREN       -> HID NumLock
    0x47, // 0x5B: AMIGA_KEY_NUMPAD_SCRLOCK_RPAREN        > HID ScrollLock
    0x54, // 0x5C: AMIGA_KEY_NUMPAD_SLASH                -> HID KEY_KEYPAD_SLASH
    0x55, // 0x5D: AMIGA_KEY_NUMPAD_ASTERISK             -> HID KEY_KEYPAD_ASTERISK
    0x57, // 0x5E: AMIGA_KEY_NUMPAD_PLUS                 -> HID KEY_KEYPAD_PLUS
    0x00, // 0x5F: AMIGA_KEY_HELP                        -> Undefined

    // 0x60 - 0x67 (Modifiers and special keys)
    0xE1, // 0x60: AMIGA_KEY_SHIFT_LEFT                  -> HID KEY_MODIFIER_LEFT_SHIFT
    0xE5, // 0x61: AMIGA_KEY_SHIFT_RIGHT                 -> HID KEY_MODIFIER_RIGHT_SHIFT
    0x39, // 0x62: AMIGA_KEY_CAPS_LOCK                   -> Undefined
    0xE0, // 0x63: AMIGA_KEY_CONTROL_LEFT                -> HID KEY_MODIFIER_LEFT_CONTROL
    0xE2, // 0x64: AMIGA_KEY_ALT_LEFT                    -> HID KEY_MODIFIER_LEFT_ALT
    0xE6, // 0x65: AMIGA_KEY_ALT_RIGHT                   -> HID KEY_MODIFIER_RIGHT_ALT
    0xE3, // 0x66: AMIGA_KEY_AMIGA_LEFT                  -> HID KEY_MODIFIER_LEFT_GUI (Windows/Command)
    0xE7  // 0x67: AMIGA_KEY_AMIGA_RIGHT                 -> HID KEY_MODIFIER_RIGHT_GUI (Windows/Command)
};

// Enumerate keyboard states
enum KeyboardState
{
  SYNCH_HI = 0,
  SYNCH_LO,
  HANDSHAKE,
  READ,
  WAIT_LO,
  WAIT_RES
};

struct KeyEvent
{
  uint8_t keyCode;
  bool isPressed;
  bool isMacro;
  uint8_t macroSlot;
  bool lastOfMacro;
  bool autoRelease;
  int32_t pressTime;
  int32_t releaseTime;
};

// Macro structure
struct Macro
{
  KeyEvent keyEvents[MAX_MACRO_LENGTH];
  uint8_t length;
};

struct MacroPlayStatus
{
  bool playing;
  bool queued;
  bool loop;
};

struct MultimediaKeyReport
{
  uint8_t report;
};

// Global variables
KeyReport keyReport;
KeyReport prevkeyReport;
MultimediaKeyReport multimediaKeyReport;
MultimediaKeyReport prevMultimediaKeyReport;

CircularBuffer<KeyEvent, QUEUE_SIZE> keyQueue;
CircularBuffer<KeyEvent, RELEASE_QUEUE_SIZE> delayedKeyQueue; //generally used for programmatic key releases

CircularBuffer<uint8_t, QUEUE_SIZE_MMKEY> multimediaQueue;
CircularBuffer<uint8_t, QUEUE_SIZE_MMKEY> multimediaReleaseQueue;

// Macro variables
Macro macros[MACRO_SLOTS]; // 5 macro slots
MacroPlayStatus macroPlayStatus[MACRO_SLOTS];
bool recording = false;
bool recordingSlot = false;
bool macro_looping = false;
uint8_t recordingMacroSlot = 0;
uint8_t recordingMacroIndex = 0;

bool functionMode = false; // Indicates if 'Help' key is active

#if ENABLE_JOYSTICKS
  // Joystick states
  uint8_t currentJoy1State = 0;
  uint8_t currentJoy2State = 0;
  uint8_t previousJoy1State = 0xFF; // Initialize to 0xFF so that initial state triggers update
  uint8_t previousJoy2State = 0xFF;

  const uint8_t joystick1Descriptor[] PROGMEM = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, 0x03,                    // REPORT_ID (3)
    0x09, 0x01,                    // USAGE (Pointer)
    0xA1, 0x00,                    // COLLECTION (Physical)
    0x09, 0x30,                    // USAGE (X)
    0x09, 0x31,                    // USAGE (Y)
    0x15, 0xFF,                    // LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x75, 0x02,                    // REPORT_SIZE (2)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xC0,                          // END_COLLECTION
    0x05, 0x09,                    // USAGE_PAGE (Button)
    0x19, 0x01,                    // USAGE_MINIMUM (Button 1)
    0x29, 0x02,                    // USAGE_MAXIMUM (Button 2)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xC0                           // END_COLLECTION
  };

  const uint8_t joystick2Descriptor[] PROGMEM = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, 0x04,                    // REPORT_ID (4)
    0x09, 0x01,                    // USAGE (Pointer)
    0xA1, 0x00,                    // COLLECTION (Physical)
    0x09, 0x30,                    // USAGE (X)
    0x09, 0x31,                    // USAGE (Y)
    0x15, 0xFF,                    // LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x75, 0x02,                    // REPORT_SIZE (2)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xC0,                          // END_COLLECTION
    0x05, 0x09,                    // USAGE_PAGE (Button)
    0x19, 0x01,                    // USAGE_MINIMUM (Button 1)
    0x29, 0x02,                    // USAGE_MAXIMUM (Button 2)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x02,                    // REPORT_COUNT (2)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0xC0                           // END_COLLECTION
  };

  // Wrap Descriptors in HIDSubDescriptor
  HIDSubDescriptor joystick1HID(joystick1Descriptor, sizeof(joystick1Descriptor));
  HIDSubDescriptor joystick2HID(joystick2Descriptor, sizeof(joystick2Descriptor));
#endif

#if ENABLE_MULTIMEDIA_KEYS
  const uint8_t multimediaDescriptor[] PROGMEM = {
    0x05, 0x0C,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x85, 0x05,                    // REPORT_ID (5)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x07,                    // REPORT_COUNT (7)
    0x09, 0xB5,                    // USAGE (Next Track)
    0x09, 0xB6,                    // USAGE (Previous Track)
    0x09, 0xB7,                    // USAGE (Stop)
    0x09, 0xCD,                    // USAGE (Play/Pause)
    0x09, 0xE2,                    // USAGE (Mute)
    0x09, 0xE9,                    // USAGE (Volume Up)
    0x09, 0xEA,                    // USAGE (Volume Down)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    0x95, 0x01,                    // REPORT_COUNT (1)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x81, 0x03,                    // INPUT (Const,Var,Abs) - Padding
    0xC0                           // END_COLLECTION
  };

  HIDSubDescriptor multimediaHID(multimediaDescriptor, sizeof(multimediaDescriptor));
#endif

// FUNCTIONS

// Keyboard and Joystick Handling
void handleKeyboard();
#if ENABLE_JOYSTICKS
void handleJoystick1(); // Only if ENABLE_JOYSTICKS is enabled
void handleJoystick2(); // Only if ENABLE_JOYSTICKS is enabled
#endif

// Report Handling
void resetReport();
bool isHIDreportfull();

// Macro Management
void playMacro();
void playMacroSlot(uint8_t slot);
void stopAllMacros();
void resetMacros();
void cleanMacros();
void startRecording();
void stopRecording();
void record_last_key(KeyEvent& keyEvent);
uint8_t macroSlotFromKeyCode(uint8_t keyCode);
bool isMacroPlaying();
uint8_t numPlayingMacros();

// EEPROM Management
#if PERSISTENT_MACRO
uint32_t adler32(Macro* macros, size_t len);
void saveMacrosToEEPROM();
bool loadMacrosFromEEPROM();
#endif

// Key Processing
void processKeyEvent(uint8_t keycode, bool isPressed);
void keyPress(uint8_t keyCode);
void keyRelease(uint8_t keyCode);
void handleFunctionModeKey(uint8_t keycode);
void addAmigaKeyToQueue(KeyEvent keyEvent);
template<typename... Keys> void addAmigaKeysToQueue(Keys... keys);
void addToMultimediaQueue(uint8_t keycode);

// Key Queue Handling
void consumeKeys(); // Process key events from queue
void consumeDelayKeys(); // Process delayed key events from queue
void consumeMultimediaKeys(); // Process multimedia key events from queue
void consumeDelayedMultimediaKeys(); // Process released multimedia key events from queue

// Helper Functions
uint8_t mapAmigaToHIDKey(uint8_t amigaKey);
bool isValidAmigaKey(uint8_t keyCode);
bool isAmigaModifierKey(uint8_t keyCode);
void add_modifier(uint8_t* modifiers, uint8_t modifier);
void remove_modifier(uint8_t* modifiers, uint8_t modifier);
bool is_modifier_active(uint8_t modifiers, uint8_t modifier);
void add_mmkey(uint8_t* mmkeys, uint8_t mmkey);
void remove_mmkey(uint8_t* mmkeys, uint8_t mmkey);
bool is_mmkey_active(uint8_t mmkeys, uint8_t mmkey);

#if DEBUG_MODE
void printKeyReport();
#endif
//-----------

void setup()
{
  #if DEBUG_MODE
    Serial.begin(9600);
    while (!Serial) { ; } // Wait for Serial to be ready
    Serial.println("Debug mode enabled");
  #endif

  //Init key reports to 0
  memset(&keyReport, 0x00, sizeof(KeyReport));
  memset(&multimediaKeyReport, 0x00, sizeof(MultimediaKeyReport));
  // Init prev key reports to FF to force send on first loop
  memset(&prevkeyReport, 0xFF, sizeof(KeyReport));
  memset(&prevMultimediaKeyReport, 0xFF, sizeof(MultimediaKeyReport));

  foreach(uint8_t i, MACRO_SLOTS) {
    macroPlayStatus[i].playing = false;
    macroPlayStatus[i].queued = false;
    macroPlayStatus[i].loop = false;
  }

  loadMacrosFromEEPROM();

  #if ENABLE_JOYSTICKS
    HID().AppendDescriptor(&joystick1HID);
    HID().AppendDescriptor(&joystick2HID);
    // Initialize Joystick 1 (Port D)
    DDRD = (uint8_t)(~BITMASK_JOY1); // Set pins as INPUT
    PORTD = BITMASK_JOY1;            // Enable internal PULL-UP resistors

    // Initialize Joystick 2 (Port C)
    DDRF = (uint8_t)(~BITMASK_JOY2); // Set pins as INPUT
    PORTF = BITMASK_JOY2;            // Enable internal PULL-UP resistors
  #endif

  #if ENABLE_MULTIMEDIA_KEYS
    HID().AppendDescriptor(&multimediaHID);
  #endif

  // Initialize Keyboard (Port B)
  DDRB &= ~(BITMASK_A500CLK | BITMASK_A500SP | BITMASK_A500RES); // Set pins as INPUT
}

void loop()
{
#if ENABLE_JOYSTICKS
  handleJoystick1(); // Handle Joystick1 input
  handleJoystick2(); // Handle Joystick2 input
#endif
  handleKeyboard(); // Handle Amiga500 Keyboard input
  consumeKeys(); // Process key events from queue
  consumeDelayKeys(); // Process delayed key events from queue
  consumeMultimediaKeys(); // Process multimedia key events from queue
  consumeDelayedMultimediaKeys(); // Process released multimedia key events from queue
  playMacro(); // Play macros (add keys from recorded macro to key event queue)
  SendReports();
}

void SendReports(){
    static uint32_t reportRunTime = 0; // Tracks the last time the function was executed
    if (millis() - reportRunTime >=  1) {
      if(memcmp(&keyReport, &prevkeyReport, sizeof(KeyReport)) != 0){
        HID().SendReport(1, &keyReport, sizeof(KeyReport));
        memcpy(&prevkeyReport, &keyReport, sizeof(KeyReport));
      }
      if(memcmp(&multimediaKeyReport, &prevMultimediaKeyReport, sizeof(MultimediaKeyReport)) != 0){
        HID().SendReport(5, &multimediaKeyReport, sizeof(MultimediaKeyReport));
        memcpy(&prevMultimediaKeyReport, &multimediaKeyReport, sizeof(MultimediaKeyReport));
      }
    }
}

bool isHIDreportfull(){
   for(int i = 0; i < 6; i++){
    if(keyReport.keys[i] == 0){
      return false;
    }
   }
   return true;
}

void consumeKeys() {
    static uint32_t consumelastRunTime = 0; // Tracks the last time the function was executed

    // Check if 400 microseconds have passed since the last execution
    if (micros() - consumelastRunTime >= CONSUME_KEY_TIMER) {
        consumelastRunTime = micros(); // Update the last execution time

        while (!keyQueue.isEmpty()) {
            KeyEvent keyEvent = keyQueue[0]; // Peek at the oldest event

            if (!isHIDreportfull || !keyEvent.isPressed || isAmigaModifierKey(keyEvent.keyCode)) {
                // Remove the event from the queue
                keyQueue.shift();
                if(keyEvent.isPressed && keyEvent.releaseTime>0)addAmigaKeyToReleaseQueue(keyEvent);
                // Process the event (update HID report)
                processKeyEvent(keyEvent.keyCode, keyEvent.isPressed);
                if(keyEvent.isMacro && keyEvent.lastOfMacro){
                  macroPlayStatus[keyEvent.macroSlot].queued = false;
                }
            } else {
                // Exit the loop if the report is full
                break;
            }
        }
    }
}

void consumeDelayKeys() {
    static uint32_t consumeDelaylastRunTime = 0; // Tracks the last time the function was executed

    // Check if 400 microseconds have passed since the last execution
    if (micros() - consumeDelaylastRunTime >=  CONSUME_KEY_TIMER) {
        consumeDelaylastRunTime = micros(); // Update the last execution time

        int queueSize = delayedKeyQueue.size(); // Get the initial size of the queue
        for (int i = 0; i < queueSize; i++) {
            KeyEvent keyEvent = delayedKeyQueue.shift();
            uint32_t currentTime = millis();
            if (currentTime >= keyEvent.releaseTime) {
                processKeyEvent(keyEvent.keyCode, keyEvent.isPressed);
            } else {
                // Add the event back to the queue if the release time has not been reached
                delayedKeyQueue.push(keyEvent);
            }
        }
    }
}

void consumeMultimediaKeys(){
    static uint32_t consumeMultimedialastRunTime = 0; // Tracks the last time the function was executed

    // Check if 800 microseconds have passed since the last execution
    if (micros() - consumeMultimedialastRunTime >=  800) {
        consumeMultimedialastRunTime = micros(); // Update the last execution time

        while (!multimediaQueue.isEmpty()) {
            uint8_t mmkey = multimediaQueue.shift();
            add_mmkey(&multimediaKeyReport.report, mmkey);
            multimediaReleaseQueue.push(mmkey);
        }
    }
}

void consumeDelayedMultimediaKeys(){
    static uint32_t consumeDelayedMultimedialastRunTime = 0; // Tracks the last time the function was executed

    if (millis() - consumeDelayedMultimedialastRunTime >=  DELAYED_QUEUE_DELAY) {
        consumeDelayedMultimedialastRunTime = millis(); // Update the last execution time

        while (!mmultimediaReleaseQueue.isEmpty()) {
            uint8_t mmkey = multimediaReleaseQueue.shift();
            remove_mmkey(&multimediaKeyReport.report, mmkey);
        }
    }
}

//find key event in queue thats not macro and have no release time set and set it
bool updateKeyEventReleaseTime(uint8_t keycode){
  for (int i = keyQueue.size() - 1; i >= 0; i--) {
    if (keyQueue[i].keyCode == keycode && keyQueue[i].isMacro == false && keyQueue[i].releaseTime == 0) {
      keyQueue[i].releaseTime = millis();
      return true;
    }
  }
  return false;
}

void handleKeyboard()
{
  // Keyboard state machine variables
  static KeyboardState keyboardState = SYNCH_HI;
  static uint8_t bitIndex = 0;
  static uint8_t currentKeyCode = 0;
  static uint32_t handshakeTimer = 0;
  uint8_t pinB = PINB;

  if (((pinB & BITMASK_A500RES) == 0) && keyboardState != WAIT_RES)
  {
    // Reset detected
    functionMode = false;
    stopAllMacros();
    resetReport();    
    addAmigaKeysToQueue(AMIGA_KEY_CONTROL_LEFT, AMIGA_KEY_ALT_LEFT, AMIGA_KEY_DELETE);
    keyboardState = WAIT_RES;
    interrupts();
    return;
  }
  else if (keyboardState == WAIT_RES)
  {
    // Waiting for reset end
    if ((pinB & BITMASK_A500RES) != 0)
    {
      keyboardState = SYNCH_HI;
    }
  }
  else if (keyboardState == SYNCH_HI)
  {
    // Sync Pulse High
    if ((pinB & BITMASK_A500CLK) == 0)
    {
      keyboardState = SYNCH_LO;
    }
  }
  else if (keyboardState == SYNCH_LO)
  {
    // Sync Pulse Low
    if ((pinB & BITMASK_A500CLK) != 0)
    {
      keyboardState = HANDSHAKE;
    }
  }
  else if (keyboardState == HANDSHAKE)
  {
    // Handshake
    if (handshakeTimer == 0)
    {
      DDRB |= BITMASK_A500SP;   // Set SP pin as OUTPUT
      PORTB &= ~BITMASK_A500SP; // Set SP pin LOW
      handshakeTimer = micros(); // Use micros() for precise timing
    }
    else if (micros() - handshakeTimer > 85) // 85 microseconds
    {
      handshakeTimer = 0;
      DDRB &= ~BITMASK_A500SP; // Set SP pin as INPUT
      keyboardState = WAIT_LO;
      currentKeyCode = 0;
      bitIndex = 7;
    }
  }
  else if (keyboardState == READ)
  {
    // Read key message (8 bits)
    if ((pinB & BITMASK_A500CLK) != 0)
    {
      if (bitIndex--)
      {
        currentKeyCode |= ((pinB & BITMASK_A500SP) == 0) << bitIndex; // Accumulate bits
        keyboardState = WAIT_LO;
      }
      else
      {
        // Read last bit (key down/up)
        const bool isKeyDown = ((pinB & BITMASK_A500SP) != 0); // true if key down
        interrupts();
        keyboardState = HANDSHAKE;
        if(isKeyDown){
          KeyEvent newKeyEvent;
          newKeyEvent.keyCode = currentKeyCode;
          newKeyEvent.isPressed = isKeyDown;
          newKeyEvent.isMacro = false;
          newKeyEvent.macroSlot = 0;
          newKeyEvent.lastOfMacro = false;
          newKeyEvent.autoRelease = false;
          newKeyEvent.pressTime = millis();
          newKeyEvent.releaseTime = 0;
          addAmigaKeyToQueue(newKeyEvent);
          record_last_key(newKeyEvent);
        }else{
          if(!updateKeyEventReleaseTime(currentKeyCode)){
              KeyEvent newKeyEvent;
              newKeyEvent.keyCode = currentKeyCode;
              newKeyEvent.isPressed = isKeyDown;
              newKeyEvent.isMacro = false;
              newKeyEvent.macroSlot = 0;
              newKeyEvent.lastOfMacro = false;
              newKeyEvent.autoRelease = false;
              newKeyEvent.pressTime = 0;
              newKeyEvent.releaseTime = millis();
              addAmigaKeyToQueue(newKeyEvent);
          }
          record_release(currentKeyCode);
        }
      }
    }
  }
  else if (keyboardState == WAIT_LO)
  {
    // Waiting for the next bit
    if ((pinB & BITMASK_A500CLK) == 0)
    {
      noInterrupts();
      keyboardState = READ;
    }
  }
}

#if ENABLE_JOYSTICKS
void handleJoystick1()
{
  uint8_t currentJoyState = ~PIND & BITMASK_JOY1;
  if (currentJoyState != previousJoy1State)
  {
    HID().SendReport(3, &currentJoyState, 1);
    previousJoy1State = currentJoyState;
  }
}

void handleJoystick2()
{
  uint8_t currentJoyState = ~PINF & BITMASK_JOY2;
  if (currentJoyState != previousJoy2State)
  {
    HID().SendReport(4, &currentJoyState, 1);
    previousJoy2State = currentJoyState;
  }
}
#endif

#if PERSISTENT_MACRO
    // Function to calculate macros checksum
    uint32_t adler32(Macro* macros, size_t len) {
        #if DEBUG_MODE
          Serial.println("Calculation checksum");
          Serial.print("Size of Macros in bytes: ");
          Serial.println(len);
        #endif
        uint8_t* data = (uint8_t*)macros;
        uint32_t A = 1; // Initial value for A
        uint32_t B = 0; // Initial value for B
        const uint32_t MOD_ADLER = 65521;

        for (size_t i = 0; i < len; i++) {
            A = (A + data[i]) % MOD_ADLER;
            B = (B + A) % MOD_ADLER;
        }

        return (B << 16) | A;
    }

    // Function to save macros to EEPROM
    void saveMacrosToEEPROM()
    {
      #if DEBUG_MODE
        Serial.println("Saving macros to EEPROM");
      #endif
      int address = EEPROM_START_ADDRESS;
      uint8_t save_version = MACRO_SAVE_VERSION;
      EEPROM.put(address, save_version);
      address += sizeof(save_version);
      for (int i = 0; i < MACRO_SLOTS; i++)
      {
        for (int j = 0; j < MAX_MACRO_LENGTH; j++)
        {
          EEPROM.put(address, macros[i].keyEvents[j]);
          address += sizeof(KeyEvent);
        }
        EEPROM.put(address, macros[i].length);
        address += sizeof(macros[i].length);
      }
      // Calculate and save checksum
      uint32_t checksum = adler32(macros, sizeof(Macro)*MACRO_SLOTS);
      EEPROM.put(address, checksum);
      #if DEBUG_MODE
        Serial.println("Saving macros to EEPROM Done!");
      #endif
    }

    // Function to load macros from EEPROM
    bool loadMacrosFromEEPROM()
    {
      #if DEBUG_MODE
        Serial.println("Loading macros from EEPROM");
      #endif
      int address = EEPROM_START_ADDRESS;
      uint8_t save_version;
      EEPROM.get(address, save_version);
      address += sizeof(save_version);
      if(save_version != MACRO_SAVE_VERSION)
      {
        #if DEBUG_MODE
          Serial.println("Save version mismatch, clearing macros");
        #endif
        cleanMacros();
        return false;
      }
      for (int i = 0; i < MACRO_SLOTS; i++)
      {
        for (int j = 0; j < MAX_MACRO_LENGTH; j++)
        {
          EEPROM.get(address, macros[i].keyEvents[j]);
          address += sizeof(KeyEvent);
        }
        EEPROM.get(address, macros[i].length);
        address += sizeof(macros[i].length);
      }
      uint32_t storedChecksum;
      EEPROM.get(address, storedChecksum);
      uint32_t calculatedChecksum = adler32(macros, sizeof(macros));
      // if checksum bad make sure to blank the memory of the macros array to not have garbage there
      if (storedChecksum != calculatedChecksum)
      {
        #if DEBUG_MODE
          Serial.println("Checksum mismatch, clearing macros");
        #endif
        cleanMacros();
        return false;
      }
      #if DEBUG_MODE
        Serial.println("Loading macros from EEPROM Done!");
      #endif
      return true;
    }
#else
    void saveMacrosToEEPROM()
    {
      // Do nothing
    }
    bool loadMacrosFromEEPROM()
    {
      cleanMacros();
      return true;
    }
#endif

void resetReport()
{
  keyReport.modifiers = 0;
  for (uint8_t i = 0; i < 6; i++)
  {
    keyReport.keys[i] = 0;
  }
  keyQueue.clear();
  delayedKeyQueue.clear();

  multimediaKeyReport.report = 0;
  multimediaQueue.clear();
  multimediaReleaseQueue.clear();
}

bool isAmigaModifierKey(uint8_t keyCode)
{
  switch (keyCode)
  {
    case AMIGA_KEY_SHIFT_LEFT:
    case AMIGA_KEY_SHIFT_RIGHT:
    case AMIGA_KEY_CONTROL_LEFT:
    case AMIGA_KEY_ALT_LEFT:
    case AMIGA_KEY_ALT_RIGHT:
    case AMIGA_KEY_AMIGA_LEFT:
    case AMIGA_KEY_AMIGA_RIGHT:
      return true;
    default:
      return false;
  }
}

// Function to map Amiga keycodes to HID keycodes using keyTable
uint8_t mapAmigaToHIDKey(uint8_t amigaKey) {
    if (amigaKey < sizeof(keyTable)) {
        return keyTable[amigaKey];
    } else {
        return 0; // Undefined key
    }
}

// Function to check if an Amiga key code is valid
bool isValidAmigaKey(uint8_t keyCode) {
    return (keyCode < AMIGA_KEY_COUNT);
}

void add_modifier(uint8_t *modifiers, uint8_t modifier) {
    *modifiers |= modifier; // Set the bit corresponding to the modifier
}

void remove_modifier(uint8_t *modifiers, uint8_t modifier) {
    *modifiers &= ~modifier; // Clear the bit corresponding to the modifier
}

bool is_modifier_active(uint8_t modifiers, uint8_t modifier) {
    return (modifiers & modifier) != 0; // Check if the bit is set
}

void add_mmkey(uint8_t *mmkeys, uint8_t mmkey) {
    *mmkeys |= mmkey; // Set the bit corresponding to the multimedia key
}

void remove_mmkey(uint8_t *mmkeys, uint8_t mmkey) {
    *mmkeys &= ~mmkey; // Clear the bit corresponding to the multimedia key
}

bool is_mmkey_active(uint8_t mmkeys, uint8_t mmkey) {
    return (mmkeys & mmkey) != 0; // Check if the bit is set
}

// Variadic template function to add Amiga keys to the queues
template<typename... Keys>
void addAmigaKeysToQueue(Keys... keys) {
    uint8_t amigaKeys[] = { static_cast<uint8_t>(keys)... };

    for (uint8_t amigaKey : amigaKeys) {
        if (isValidAmigaKey(amigaKey)) {
            // Press key event
            KeyEvent keyEvent;
            keyEvent.keyCode = amigaKey; // Use the Amiga key code directly
            keyEvent.isPressed = true;
            keyQueue.push(keyEvent);
        } else {
            // Optionally handle invalid keys
            #if DEBUG_MODE
                Serial.print("Invalid Amiga key code: 0x");
                Serial.println(amigaKey, HEX);
            #endif
        }
    }
}

void addAmigaKeyToQueue(KeyEvent keyEvent) {
    if (isValidAmigaKey(keyEvent.keyCode)) {
            keyQueue.push(keyEvent);
    }
    else {
        #if DEBUG_MODE
            Serial.print("Invalid Amiga key code: 0x");
            Serial.println(keyEvent.keyCode, HEX);
        #endif
    }
}

void addAmigaKeyToReleaseQueue(KeyEvent keyEvent) {
            keyEvent.isPressed = false;
            //we need to calculate when the key need to be relesead from now
            keyEvent.releaseTime = millis() + (keyEvent.releaseTime - keyEvent.pressTime);
            delayedKeyQueue.push(keyEvent);
}

void processKeyEvent(uint8_t keycode, bool isPressed) {
  #if DEBUG_MODE
    Serial.print("Processing key code: ");
    Serial.println(keycode, HEX);
  #endif

  if (keycode == AMIGA_KEY_HELP) {
    // 'Help' key toggles function mode
    functionMode = isPressed; // Set function mode state
    return;
  }

  if(isPressed && functionMode){
    handleFunctionModeKey(keycode); //Layer 2 alternative keys or functions
    return;
  }

  if (recording && !recordingSlot) { // Recording mode, slot selection state, discard all keys until slot selected
      if (isPressed && keycode >= AMIGA_KEY_F6 && keycode <= AMIGA_KEY_F10) {
        noInterrupts();
        recordingMacroSlot = macroSlotFromKeyCode(keycode);
        memset(macros[recordingMacroSlot].keyEvents, 0, sizeof(macros[recordingMacroSlot].keyEvents)); // Clear macro slot
        resetReport();
        macros[recordingMacroSlot].length = 0;
        recordingMacroIndex = 0;
        recordingSlot = true;
        interrupts();
        #if DEBUG_MODE
        Serial.print("Recording slot selected: ");
        Serial.println(recordingMacroSlot, HEX);
        #endif
      }
      return;
  }

  if (isPressed) {
      keyPress(keycode); // Map and handle the key press
  } else {
      keyRelease(keycode); // Map and handle the key release
  }
}

void addToMultimediaQueue(uint8_t keycode) {
  multimediaQueue.push(keycode);
}

void handleFunctionModeKey(uint8_t keycode)
{
#if DEBUG_MODE
  Serial.print("Handling function mode key: ");
  Serial.println(keycode, HEX);
#endif
  switch (keycode)
  {
  case AMIGA_KEY_F1:
    addAmigaKeysToQueue(AMIGA_KEY_UNDEFINED_47);
    break; // F11
  case AMIGA_KEY_F2:
    addAmigaKeysToQueue(AMIGA_KEY_UNDEFINED_48);
    break; // F12
  case 0x5D:
    addAmigaKeysToQueue(AMIGA_KEY_UNDEFINED_49);
    break; // PrtSc
  case 0x52:
    startRecording();
    break; // Help + F3: Start recording macro
  case 0x53:
    stopRecording();
    break; // Help + F4: Stop recording and save
  case 0x54:
    macro_looping = !macro_looping;
    break; // Help + F5: Toggle loop play macro mode
  case 0x41:
    stopAllMacros();
    break; // Help + Backspace: Stop any playing macro
  case 0x46:
    resetMacros();
    break; // Help + Del: Stop any playing macro and reset all macros including eeprom
  case 0x55:
  case 0x56:
  case 0x57:
  case 0x58:
  case 0x59:
    playMacroSlot(macroSlotFromKeyCode(keycode));
    break; // Help + F6 to F10: Play macro in corresponding slot
#if ENABLE_MULTIMEDIA_KEYS
  case 0x4C: // HELP + Arrow Up: Volume Up
    addToMultimediaQueue(MMKEY_VOLUME_UP); // Bit 5: Volume Up
    break;
  case 0x4D: // HELP + Arrow Down: Volume Down
    addToMultimediaQueue(MMKEY_VOLUME_DOWN); // Bit 6: Volume Down
    break;
  case 0x4E: // HELP + Arrow Right: Next Track
    addToMultimediaQueue(MMKEY_NEXT_TRACK); // Bit 0: Next Track
    break;
  case 0x4F: // HELP + Arrow Left: Previous Track
    addToMultimediaQueue(MMKEY_PREV_TRACK); // Bit 1: Previous Track
    break;
  case 0x44: // HELP + Enter: Play/Pause
    addToMultimediaQueue(MMKEY_PLAY_PAUSE); // Bit 3: Play/Pause
    break;
  case 0x40: // HELP + Space: Stop
    addToMultimediaQueue(MMKEY_STOP); // Bit 2: Stop
    break;
  case 0x64: // HELP + Right ALT: Mute
    addToMultimediaQueue(MMKEY_MUTE); // Bit 4: Mute
    break;
#endif
  default:
    break;
  }
}

void keyPress(uint8_t keyCode)
{
#if DEBUG_MODE
  Serial.print("Key press: ");
  Serial.println(keyCode, HEX);
#endif
  uint8_t hidCode = mapAmigaToHIDKey(keyCode);
  if (isAmigaModifierKey(keyCode))
  {
    keyReport.modifiers |= hidCode; // Modifier key
  }
  else
  {
    for (uint8_t i = 0; i < 6; i++)
    {
      if (keyReport.keys[i] == 0)
      {
        keyReport.keys[i] = hidCode;
        break;
      }
    }
  }
  #if DEBUG_MODE
    printKeyReport();
  #endif
}

void keyRelease(uint8_t keyCode)
{
#if DEBUG_MODE
  Serial.print("Key release: ");
  Serial.println(keyCode, HEX);
#endif
  uint8_t hidCode = mapAmigaToHIDKey(keyCode);
  if (isAmigaModifierKey(keyCode))
  {
    keyReport.modifiers &= ~hidCode; // Modifier key
  }
  else
  {
    for (uint8_t i = 0; i < 6; i++)
    {
      if (keyReport.keys[i] == hidCode)
      {
        keyReport.keys[i] = 0;
      }
    }
  }
  #if DEBUG_MODE
    printKeyReport();
  #endif
}

void startRecording()
{
  if(!recording){
      #if DEBUG_MODE
        Serial.println("Start recording macro");
      #endif
      noInterrupts();
      stopAllMacros();
      resetReport();
      recordingMacroIndex = 0;
      recordingMacroSlot = 0;
      recordingSlot = false;
      recording = true;
      interrupts();
  }
}

void stopRecording()
{
  if(recording){
    #if DEBUG_MODE
      Serial.println("Stop recording macro");
    #endif
    noInterrupts();
    recording = false;
    recordingSlot = false;
    resetReport();
    // Save macros to EEPROM
    saveMacrosToEEPROM();
    interrupts();
  }
}

void cleanMacros()
{
    for (int i = 0; i < MACRO_SLOTS; i++)
    {
      memset(macros[i].keyEvents, 0, sizeof(macros[i].keyEvents));
      macros[i].length = 0;
    }
}

void resetMacros()
{
#if DEBUG_MODE
  Serial.println("Reset macros");
#endif
  noInterrupts();
  stopAllMacros();
  resetReport();
  cleanMacros();
  saveMacrosToEEPROM();
  interrupts();
}

void playMacroSlot(uint8_t slot)
{
  if(!macroPlayStatus[slot].playing && numPlayingMacros()<=MAX_CONCURENT_MACROS){
    #if DEBUG_MODE
     Serial.print("Play macro slot: ");
     Serial.println(slot);
    #endif
    if(macro_looping){
      macroPlayStatus[slot].loop = true;
    }
    else{
      macroPlayStatus[slot].loop = false;
    }
    macroPlayStatus[slot].playing = true;
  }
  else{
    //toggle playing
    #if DEBUG_MODE
     Serial.print("Stop Play macro slot: ");
     Serial.println(slot);
    #endif
    macroPlayStatus[slot].playing = false;
    macroPlayStatus[slot].loop = false;
    resetReport();
    macroPlayStatus[slot].queued = false;
  }
}

// Check if any macro is playing
bool isMacroPlaying()
{
   for(int i = 0; i < MACRO_SLOTS; i++){
     if(macroPlayStatus[i].playing){
       return true;
     }
   }
   return false;
}

// Check number of playing macros
uint8_t numPlayingMacros()
{
  uint8_t num = 0;
  for(int i = 0; i < MACRO_SLOTS; i++){
    if(macroPlayStatus[i].playing){
      num++;
    }
  }
  return num;
}

// Stop all playing macros
void stopAllMacros()
{
  noInterrupts();
  for(int i = 0; i < MACRO_SLOTS; i++){
    if(macroPlayStatus[i].playing){
      macroPlayStatus[i].playing = false;
      macroPlayStatus[i].loop = false;
      macroPlayStatus[i].queued = false;
    }
  }
  resetReport();
  interrupts();
}

void playMacro()
{
    static uint32_t lastMacroTime = 0;
    if (millis() - lastMacroTime >= MACRO_DELAY)
    {
        for (uint8_t macro_slot = 0; macro_slot < MACRO_SLOTS; macro_slot++)
        {
            // Check if the macro is currently playing
            if (macroPlayStatus[macro_slot].playing && !macroPlayStatus[macro_slot].queued)
            {
              //We have a key queue, so we simply add the whole macro the the key queue as is
              for(uint8_t keyEventIndex = 0; keyEventIndex < macros[macro_slot].length; keyEventIndex++){
                addAmigaKeyToQueue(macros[macro_slot].keyEvents[keyEventIndex]);
              }
              //if looping is false we set playing to false
              macroPlayStatus[macro_slot].queued = true;
              macroPlayStatus[macro_slot].playing = macroPlayStatus[macro_slot].loop;
            }
        }
        // Update the last macro time for the next iteration
        lastMacroTime = millis();
    }
}


void record_last_key(KeyEvent& keyEvent){
  if (recording && recordingSlot && recordingMacroIndex < MAX_MACRO_LENGTH)
  {
    #if DEBUG_MODE
      Serial.print("Recording key at index: ");
      Serial.println(macroIndex);
    #endif
    KeyEvent& macroKeyEvent=macros[recordingMacroSlot].keyEvents[recordingMacroIndex];
    macroKeyEvent.keyCode = keycode;
    macroKeyEvent.isPressed = true;
    macroKeyEvent.isMacro = true;
    macroKeyEvent.macroSlot = recordingMacroSlot;
    macroKeyEvent.lastOfMacro = false;
    macroKeyEvent.pressTime = millis();
    macroKeyEvent.releaseTime = 0;
    recordingMacroIndex++;
    macros[recordingMacroSlot].length = recordingMacroIndex;
    // Check if the last index was recorded
    if (recordingMacroIndex >= MAX_MACRO_LENGTH)
    {
      macroKeyEvent.lastOfMacro = true;
      stopRecording();
    }
  }
}

void record_release(uint8_t keycode){
  if (recording && recordingSlot && recordingMacroIndex < MAX_MACRO_LENGTH)
  {
    for (int i = recordingMacroIndex - 1; i >= 0; i--) {
      KeyEvent& macroKeyEvent=macros[recordingMacroSlot].keyEvents[i];
      if (macroKeyEvent.keyCode == keycode && macroKeyEvent.releaseTime == 0) {
        macroKeyEvent.releaseTime = millis();
        return true;
      }
    }
    return false;
  }
}

uint8_t macroSlotFromKeyCode(uint8_t keyCode)
{
  uint8_t slot = keyCode - AMIGA_KEY_F6;
  if (slot >= MACRO_SLOTS)
  {
    slot = MACRO_SLOTS - 1; // Ensure it does not exceed the maximum slots
  }
  return slot;
}

#if DEBUG_MODE
void printKeyReport()
{
  Serial.print("Modifiers: ");
  Serial.println(keyReport.modifiers, BIN);
  Serial.print("Keys: ");
  for (uint8_t i = 0; i < 6; i++)
  {
    Serial.print(keyReport.keys[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
#endif