/*
 * Amiga 500 Keyboard to USB HID Converter Arduino Leonardo
 * This rewrite/update and version (c) 2024 by Luka "Void" MIT License
 * GitHub: https://github.com/arvvoid/
 * Contact: luka@lukavoid.xyz
 * Original code and inspiration by olaf, Steve_Reaver (taken from https://forum.arduino.cc/index.php?topic=139358.15)
 *
 * This sketch converts an original Amiga 500 keyboard to a standard USB HID
 * keyboard using an Arduino Leonardo. It includes support for joystick inputs
 * and special function keys.
 */

#include <Keyboard.h>
#include <HID.h>

// Preprocessor flag to enable or disable debug mode
// Debug mode provides verbose console output at every step.
#define DEBUG_MODE 0

// Preprocessor flag to enable or disable joystick support
// Joystick support is only required if you are going to attach DB9 connectors and use legacy Amiga joysticks with the controller.
#define ENABLE_JOYSTICKS 0

#define ENABLE_MULTIMEDIA_KEYS 1

#define MAX_MACRO_LENGTH 24 // Maximum number of key reports in a macro
#define MACRO_SLOTS 5
#define MACRO_DELAY 30     // ms between reports in macro playback
#define CONCURENT_MACROS 1 // Allow multiple macros to be played at the same time
#define PERSISTENT_MACRO 1 // Save macros to EEPROM

#define PROGRAMMATIC_KEYS_RELEASE 2 // delay between press and release on programmatic keys (send keystrokes, macros...)

#if PERSISTENT_MACRO
#include <EEPROM.h>
#define EEPROM_START_ADDRESS 0
#endif

// Define bit masks for keyboard and joystick inputs
#define BITMASK_A500CLK 0b00010000 // IO 8
#define BITMASK_A500SP 0b00100000  // IO 9
#define BITMASK_A500RES 0b01000000 // IO 10
#if ENABLE_JOYSTICKS
#define BITMASK_JOY1 0b10011111 // IO 0..4,6
#define BITMASK_JOY2 0b11110011 // IO A0..A5
#endif

enum AmigaKeys
{ // ROM Default (USA0) and USA1 Console Key Mapping
  // 0x00 - 0x0F
  AMIGA_KEY_TILDE = 0x00,        // ` ~
  AMIGA_KEY_1 = 0x01,            // 1 !
  AMIGA_KEY_2 = 0x02,            // 2 @
  AMIGA_KEY_3 = 0x03,            // 3 #
  AMIGA_KEY_4 = 0x04,            // 4 $
  AMIGA_KEY_5 = 0x05,            // 5 %
  AMIGA_KEY_6 = 0x06,            // 6 ^
  AMIGA_KEY_7 = 0x07,            // 7 &
  AMIGA_KEY_8 = 0x08,            // 8 *
  AMIGA_KEY_9 = 0x09,            // 9 (
  AMIGA_KEY_0 = 0x0A,            // 0 )
  AMIGA_KEY_MINUS = 0x0B,        // - _
  AMIGA_KEY_EQUAL = 0x0C,        // = +
  AMIGA_KEY_PIPE = 0x0D,         // |
  AMIGA_KEY_UNDEFINED_0E = 0x0E, // Undefined
  AMIGA_KEY_NUMPAD_0 = 0x0F,     // Numpad 0

  // 0x10 - 0x1F
  AMIGA_KEY_Q = 0x10,             // Q
  AMIGA_KEY_W = 0x11,             // W
  AMIGA_KEY_E = 0x12,             // E
  AMIGA_KEY_R = 0x13,             // R
  AMIGA_KEY_T = 0x14,             // T
  AMIGA_KEY_Y = 0x15,             // Y
  AMIGA_KEY_U = 0x16,             // U
  AMIGA_KEY_I = 0x17,             // I
  AMIGA_KEY_O = 0x18,             // O
  AMIGA_KEY_P = 0x19,             // P
  AMIGA_KEY_LEFT_BRACKET = 0x1A,  // [ {
  AMIGA_KEY_RIGHT_BRACKET = 0x1B, // ] }
  AMIGA_KEY_UNDEFINED_1C = 0x1C,  // Undefined
  AMIGA_KEY_NUMPAD_1 = 0x1D,      // Numpad 1
  AMIGA_KEY_NUMPAD_2 = 0x1E,      // Numpad 2
  AMIGA_KEY_NUMPAD_3 = 0x1F,      // Numpad 3

  // 0x20 - 0x2F
  AMIGA_KEY_A = 0x20,            // A
  AMIGA_KEY_S = 0x21,            // S
  AMIGA_KEY_D = 0x22,            // D
  AMIGA_KEY_F = 0x23,            // F
  AMIGA_KEY_G = 0x24,            // G
  AMIGA_KEY_H = 0x25,            // H
  AMIGA_KEY_J = 0x26,            // J
  AMIGA_KEY_K = 0x27,            // K
  AMIGA_KEY_L = 0x28,            // L
  AMIGA_KEY_SEMICOLON = 0x29,    // ; :
  AMIGA_KEY_APOSTROPHE = 0x2A,   // ' "
  AMIGA_KEY_NON_US_TILDE = 0x2B, // Non-US #
  AMIGA_KEY_UNDEFINED_2C = 0x2C, // Undefined
  AMIGA_KEY_NUMPAD_4 = 0x2D,     // Numpad 4
  AMIGA_KEY_NUMPAD_5 = 0x2E,     // Numpad 5
  AMIGA_KEY_NUMPAD_6 = 0x2F,     // Numpad 6

  // 0x30 - 0x3F
  AMIGA_KEY_NON_US_BACKSLASH = 0x30, // Non-US \ (Not on most US keyboards)
  AMIGA_KEY_Z = 0x31,                // Z
  AMIGA_KEY_X = 0x32,                // X
  AMIGA_KEY_C = 0x33,                // C
  AMIGA_KEY_V = 0x34,                // V
  AMIGA_KEY_B = 0x35,                // B
  AMIGA_KEY_N = 0x36,                // N
  AMIGA_KEY_M = 0x37,                // M
  AMIGA_KEY_COMMA = 0x38,            // , <
  AMIGA_KEY_PERIOD = 0x39,           // . >
  AMIGA_KEY_SLASH = 0x3A,            // / ?
  AMIGA_KEY_UNDEFINED_3B = 0x3B,     // Undefined
  AMIGA_KEY_NUMPAD_PERIOD = 0x3C,    // Numpad .
  AMIGA_KEY_NUMPAD_7 = 0x3D,         // Numpad 7
  AMIGA_KEY_NUMPAD_8 = 0x3E,         // Numpad 8
  AMIGA_KEY_NUMPAD_9 = 0x3F,         // Numpad 9

  // 0x40 - 0x4F
  AMIGA_KEY_SPACE = 0x40,        // Spacebar
  AMIGA_KEY_BACKSPACE = 0x41,    // Backspace
  AMIGA_KEY_TAB = 0x42,          // Tab
  AMIGA_KEY_NUMPAD_ENTER = 0x43, // Numpad Enter
  AMIGA_KEY_RETURN = 0x44,       // Return
  AMIGA_KEY_ESCAPE = 0x45,       // Escape
  AMIGA_KEY_DELETE = 0x46,       // Delete
  AMIGA_KEY_UNDEFINED_47 = 0x47, // Undefined
  AMIGA_KEY_UNDEFINED_48 = 0x48, // Undefined
  AMIGA_KEY_UNDEFINED_49 = 0x49, // Undefined
  AMIGA_KEY_NUMPAD_MINUS = 0x4A, // Numpad -
  AMIGA_KEY_UNDEFINED_4B = 0x4B, // Undefined
  AMIGA_KEY_ARROW_UP = 0x4C,     // Up Arrow
  AMIGA_KEY_ARROW_DOWN = 0x4D,   // Down Arrow
  AMIGA_KEY_ARROW_RIGHT = 0x4E,  // Right Arrow
  AMIGA_KEY_ARROW_LEFT = 0x4F,   // Left Arrow

  // 0x50 - 0x5F
  AMIGA_KEY_F1 = 0x50,                     // F1
  AMIGA_KEY_F2 = 0x51,                     // F2
  AMIGA_KEY_F3 = 0x52,                     // F3
  AMIGA_KEY_F4 = 0x53,                     // F4
  AMIGA_KEY_F5 = 0x54,                     // F5
  AMIGA_KEY_F6 = 0x55,                     // F6
  AMIGA_KEY_F7 = 0x56,                     // F7
  AMIGA_KEY_F8 = 0x57,                     // F8
  AMIGA_KEY_F9 = 0x58,                     // F9
  AMIGA_KEY_F10 = 0x59,                    // F10
  AMIGA_KEY_NUMPAD_NUMLOCK_LPAREN = 0x5A,  // Numpad ( Numlock
  AMIGA_KEY_NUMPAD_SCRLOCK_RPAREN = 0x5B,  // Numpad ) Scroll Lock
  AMIGA_KEY_NUMPAD_SLASH = 0x5C,           // Numpad /
  AMIGA_KEY_NUMPAD_ASTERISK_PTRSCR = 0x5D, // Numpad * PtrScr
  AMIGA_KEY_NUMPAD_PLUS = 0x5E,            // Numpad +
  AMIGA_KEY_HELP = 0x5F,                   // Help

  // 0x60 - 0x67 (Modifiers and special keys)
  AMIGA_KEY_SHIFT_LEFT = 0x60,   // Left Shift
  AMIGA_KEY_SHIFT_RIGHT = 0x61,  // Right Shift
  AMIGA_KEY_CAPS_LOCK = 0x62,    // Caps Lock
  AMIGA_KEY_CONTROL_LEFT = 0x63, // Left Control
  AMIGA_KEY_ALT_LEFT = 0x64,     // Left Alt
  AMIGA_KEY_ALT_RIGHT = 0x65,    // Right Alt
  AMIGA_KEY_AMIGA_LEFT = 0x66,   // Left Amiga (Windows key)
  AMIGA_KEY_AMIGA_RIGHT = 0x67,  // Right Amiga
  AMIGA_KEY_COUNT                // Total number of Amiga keys
};

const uint8_t keyTable[AMIGA_KEY_COUNT] = {
    // US Keyboard Layout Amiga500 to HID

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
    0x02, // 0x60: AMIGA_KEY_SHIFT_LEFT                  -> HID KEY_MODIFIER_LEFT_SHIFT
    0x20, // 0x61: AMIGA_KEY_SHIFT_RIGHT                 -> HID KEY_MODIFIER_RIGHT_SHIFT
    0x00, // 0x62: AMIGA_KEY_CAPS_LOCK                   -> Undefined
    0x01, // 0x63: AMIGA_KEY_CONTROL_LEFT                -> HID KEY_MODIFIER_LEFT_CONTROL
    0x04, // 0x64: AMIGA_KEY_ALT_LEFT                    -> HID KEY_MODIFIER_LEFT_ALT
    0x40, // 0x65: AMIGA_KEY_ALT_RIGHT                   -> HID KEY_MODIFIER_RIGHT_ALT
    0x08, // 0x66: AMIGA_KEY_AMIGA_LEFT                  -> HID KEY_MODIFIER_LEFT_GUI (Windows/Command)
    0x10  // 0x67: AMIGA_KEY_AMIGA_RIGHT                 -> HID KEY_MODIFIER_RIGHT_GUI (Windows/Command)
};

enum MultimediaKey
{
  MMKEY_NEXT_TRACK = 1 << 0, // 0x01
  MMKEY_PREV_TRACK = 1 << 1, // 0x02
  MMKEY_STOP = 1 << 2,       // 0x04
  MMKEY_PLAY_PAUSE = 1 << 3, // 0x08
  MMKEY_MUTE = 1 << 4,       // 0x10
  MMKEY_VOLUME_UP = 1 << 5,  // 0x20
  MMKEY_VOLUME_DOWN = 1 << 6 // 0x40
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

// Macro structure
struct Macro
{
  KeyReport keyReports[MAX_MACRO_LENGTH];
  uint8_t length;
};

struct MacroPlayStatus
{
  bool playing;
  bool loop;
  uint8_t macroIndex;
};

struct MultimediaKeyReport
{
  uint8_t reportId;
  uint8_t keys;
};

MultimediaKeyReport multimediaKeyReport = {5, 0};

// Global variables
KeyReport keyReport;
KeyReport prevkeyReport;
uint32_t handshakeTimer = 0;
Macro macros[MACRO_SLOTS]; // 5 macro slots
MacroPlayStatus macroPlayStatus[MACRO_SLOTS];
bool recording = false;
bool recordingSlot = false;
bool macro_looping = false;
uint8_t recordingMacroSlot = 0;
uint8_t recordingMacroIndex = 0;

#if ENABLE_JOYSTICKS
// Joystick states
uint8_t currentJoy1State = 0;
uint8_t currentJoy2State = 0;
uint8_t previousJoy1State = 0xFF; // Initialize to 0xFF so that initial state triggers update
uint8_t previousJoy2State = 0xFF;

const uint8_t joystick1Descriptor[] PROGMEM = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x05, // USAGE (Game Pad)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x03, // REPORT_ID (3)
    0x09, 0x01, // USAGE (Pointer)
    0xA1, 0x00, // COLLECTION (Physical)
    0x09, 0x30, // USAGE (X)
    0x09, 0x31, // USAGE (Y)
    0x15, 0xFF, // LOGICAL_MINIMUM (-1)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x02, // REPORT_COUNT (2)
    0x75, 0x02, // REPORT_SIZE (2)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0xC0,       // END_COLLECTION
    0x05, 0x09, // USAGE_PAGE (Button)
    0x19, 0x01, // USAGE_MINIMUM (Button 1)
    0x29, 0x02, // USAGE_MAXIMUM (Button 2)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x02, // REPORT_COUNT (2)
    0x75, 0x01, // REPORT_SIZE (1)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0xC0        // END_COLLECTION
};

const uint8_t joystick2Descriptor[] PROGMEM = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x05, // USAGE (Game Pad)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x04, // REPORT_ID (4)
    0x09, 0x01, // USAGE (Pointer)
    0xA1, 0x00, // COLLECTION (Physical)
    0x09, 0x30, // USAGE (X)
    0x09, 0x31, // USAGE (Y)
    0x15, 0xFF, // LOGICAL_MINIMUM (-1)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x02, // REPORT_COUNT (2)
    0x75, 0x02, // REPORT_SIZE (2)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0xC0,       // END_COLLECTION
    0x05, 0x09, // USAGE_PAGE (Button)
    0x19, 0x01, // USAGE_MINIMUM (Button 1)
    0x29, 0x02, // USAGE_MAXIMUM (Button 2)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x02, // REPORT_COUNT (2)
    0x75, 0x01, // REPORT_SIZE (1)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0xC0        // END_COLLECTION
};

// Wrap Descriptors in HIDSubDescriptor
HIDSubDescriptor joystick1HID(joystick1Descriptor, sizeof(joystick1Descriptor));
HIDSubDescriptor joystick2HID(joystick2Descriptor, sizeof(joystick2Descriptor));
#endif

#if ENABLE_MULTIMEDIA_KEYS
const uint8_t multimediaDescriptor[] PROGMEM = {
    0x05, 0x0C, // USAGE_PAGE (Consumer Devices)
    0x09, 0x01, // USAGE (Consumer Control)
    0xA1, 0x01, // COLLECTION (Application)
    0x85, 0x05, // REPORT_ID (5)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x07, // REPORT_COUNT (7)
    0x09, 0xB5, // USAGE (Next Track)
    0x09, 0xB6, // USAGE (Previous Track)
    0x09, 0xB7, // USAGE (Stop)
    0x09, 0xCD, // USAGE (Play/Pause)
    0x09, 0xE2, // USAGE (Mute)
    0x09, 0xE9, // USAGE (Volume Up)
    0x09, 0xEA, // USAGE (Volume Down)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x81, 0x03, // INPUT (Const,Var,Abs) - Padding
    0xC0        // END_COLLECTION
};

HIDSubDescriptor multimediaHID(multimediaDescriptor, sizeof(multimediaDescriptor));
#endif

// Keyboard state machine variables
KeyboardState keyboardState = SYNCH_HI;
uint8_t bitIndex = 0;
uint8_t currentKeyCode = 0;
bool functionMode = false; // Indicates if 'Help' key is active
bool isKeyDown = false;

#if PERSISTENT_MACRO
// Function to calculate macros checksum
uint8_t calculateChecksum(Macro *macros, int length)
{
  uint8_t checksum = 0;
  uint8_t *data = (uint8_t *)macros;
  for (int i = 0; i < length; i++)
  {
    checksum ^= data[i];
  }
  return checksum;
}

// Function to save macros to EEPROM
void saveMacrosToEEPROM()
{
#if DEBUG_MODE
  Serial.println("Saving macros to EEPROM");
#endif
  int address = EEPROM_START_ADDRESS;
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    for (int j = 0; j < MAX_MACRO_LENGTH; j++)
    {
      EEPROM.put(address, macros[i].keyReports[j]);
      address += sizeof(KeyReport);
    }
    EEPROM.put(address, macros[i].length);
    address += sizeof(macros[i].length);
  }
  // Calculate and save checksum
  uint8_t checksum = calculateChecksum(macros, sizeof(macros));
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
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    for (int j = 0; j < MAX_MACRO_LENGTH; j++)
    {
      EEPROM.get(address, macros[i].keyReports[j]);
      address += sizeof(KeyReport);
    }
    EEPROM.get(address, macros[i].length);
    address += sizeof(macros[i].length);
  }
  uint8_t storedChecksum;
  EEPROM.get(address, storedChecksum);
  uint8_t calculatedChecksum = calculateChecksum(macros, sizeof(macros));
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

void setup()
{
#if DEBUG_MODE
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  } // Wait for Serial to be ready
  Serial.println("Debug mode enabled");
#endif
  noInterrupts(); // Disable interrupts to enter critical section
  loadMacrosFromEEPROM();
  interrupts(); // Enable interrupts to exit critical section

  memset(&keyReport, 0x00, sizeof(KeyReport));
  memset(&prevkeyReport, 0xFF, sizeof(KeyReport));

#if ENABLE_JOYSTICKS
  HID().AppendDescriptor(&joystick1HID);
  HID().AppendDescriptor(&joystick2HID);
  // Initialize Joystick 1 (Port D)
  DDRD = (uint8_t)(~BITMASK_JOY1); // Set pins as INPUT
  PORTD = BITMASK_JOY1;            // Enable internal PULL-UP resistors

  // Initialize Joystick 2 (Port F)
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
  handleJoystick1();
  handleJoystick2();
#endif
  handleKeyboard();
  playMacro();
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

void handleKeyboard()
{
  uint8_t pinB = PINB;

  if (((pinB & BITMASK_A500RES) == 0) && keyboardState != WAIT_RES)
  {
    // Reset detected
    interrupts();
    keystroke(0x4C, 0x05); // Send CTRL+ALT+DEL
    functionMode = false;
    keyboardState = WAIT_RES;
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
      handshakeTimer = millis();
    }
    else if (millis() - handshakeTimer > 10)
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
        isKeyDown = ((pinB & BITMASK_A500SP) != 0); // true if key down
        interrupts();
        keyboardState = HANDSHAKE;
        processKeyCode();
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

void resetReport()
{
  keyReport.modifiers = 0;
  for (uint8_t i = 0; i < 6; i++)
  {
    keyReport.keys[i] = 0;
  }
}

void sendReport()
{
  if (memcmp(&keyReport, &prevkeyReport, sizeof(KeyReport)) != 0)
  {
    HID().SendReport(2, &keyReport, sizeof(KeyReport));
    memcpy(&prevkeyReport, &keyReport, sizeof(KeyReport));
  }
}

void releaseAll()
{
  resetReport();
  sendReport();
}

void sendMultimediaKey(uint8_t keyBit)
{
  multimediaKeyReport.keys |= keyBit; // Set the key bit
  HID().SendReport(multimediaKeyReport.reportId, &multimediaKeyReport.keys, sizeof(multimediaKeyReport.keys));
}

void releaseMultimediaKey(uint8_t keyBit)
{
  multimediaKeyReport.keys &= ~keyBit; // Clear the key bit
  HID().SendReport(multimediaKeyReport.reportId, &multimediaKeyReport.keys, sizeof(multimediaKeyReport.keys));
}

void processKeyCode()
{
#if DEBUG_MODE
  Serial.print("Processing key code: ");
  Serial.println(currentKeyCode, HEX);
#endif
  if (currentKeyCode == AMIGA_KEY_HELP)
  {
    // 'Help' key toggles function mode
    functionMode = isKeyDown;
  }
  else if (currentKeyCode == AMIGA_KEY_CAPS_LOCK)
  {
    // CapsLock key
    keystroke(0x39, 0x00);
    return;
  }
  else
  {
    if (isKeyDown)
    {
      // Key down message received
      if (functionMode)
      {
        // Special function with 'Help' key
        handleFunctionModeKey();
        return;
      }
      else
      {

        if (recording && !recordingSlot)
        {
          if (currentKeyCode >= AMIGA_KEY_F6 && currentKeyCode <= AMIGA_KEY_F10)
          {
            noInterrupts(); // Disable interrupts to enter critical section
            recordingMacroSlot = macroSlotFromKeyCode(currentKeyCode);
            memset(macros[recordingMacroSlot].keyReports, 0, sizeof(macros[recordingMacroSlot].keyReports)); // Clear macro slot
            macros[recordingMacroSlot].length = 0;
            recordingMacroIndex = 0;
            recordingSlot = true;
            interrupts(); // Enable interrupts to exit critical section
#if DEBUG_MODE
            Serial.print("Recording slot selected: ");
            Serial.println(currentMacroSlot, HEX);
#endif
          }
          return;
        }

        if (currentKeyCode == AMIGA_KEY_NUMPAD_NUMLOCK_LPAREN)
        {
          keystroke(0x53, 0); // NumLock
        }
        else if (currentKeyCode == AMIGA_KEY_NUMPAD_SCRLOCK_RPAREN)
        {
          keystroke(0x47, 0); // ScrollLock
        }
        else if (currentKeyCode < AMIGA_KEY_COUNT)
        {
          keyPress(currentKeyCode);
        }
      }
    }
    else
    {
      // Key release message received
      if (currentKeyCode < AMIGA_KEY_COUNT)
      {
        keyRelease(currentKeyCode);
      }
    }
  }
}

void handleFunctionModeKey()
{
#if DEBUG_MODE
  Serial.print("Handling function mode key: ");
  Serial.println(currentKeyCode, HEX);
#endif
  switch (currentKeyCode)
  {
  case AMIGA_KEY_F1:
    keystroke(0x44, 0);
    break; // F11
  case AMIGA_KEY_F2:
    keystroke(0x45, 0);
    break; // F12
  case AMIGA_KEY_NUMPAD_ASTERISK_PTRSCR:
    keystroke(0x46, 0);
    break; // PrtSc
  case AMIGA_KEY_NUMPAD_0:
    keystroke(0x49, 0);
    break; // Ins
  case AMIGA_KEY_F3:
    startRecording();
    break; // Help + F3: Start recording macro
  case AMIGA_KEY_F4:
    stopRecording();
    break; // Help + F4: Stop recording and save
  case AMIGA_KEY_F5:
    macro_looping = !macro_looping;
    break; // Help + F5: Toggle loop play macro mode
  case AMIGA_KEY_BACKSPACE:
    stopAllMacros();
    break; // Help + Backspace: Stop any playing macro
  case AMIGA_KEY_DELETE:
    resetMacros();
    break; // Help + Del: Stop any playing macro and reset all macros including eeprom
  case AMIGA_KEY_F6:
  case AMIGA_KEY_F7:
  case AMIGA_KEY_F8:
  case AMIGA_KEY_F9:
  case AMIGA_KEY_F10:
    playMacroSlot(macroSlotFromKeyCode(currentKeyCode));
    break; // Help + F6 to F10: Play macro in corresponding slot
#if ENABLE_MULTIMEDIA_KEYS
  case AMIGA_KEY_ARROW_UP:                // HELP + Arrow Up: Volume Up
    multimediaKeystroke(MMKEY_VOLUME_UP); // Bit 5: Volume Up
    break;
  case AMIGA_KEY_ARROW_DOWN:                // HELP + Arrow Down: Volume Down
    multimediaKeystroke(MMKEY_VOLUME_DOWN); // Bit 6: Volume Down
    break;
  case AMIGA_KEY_ARROW_RIGHT:              // HELP + Arrow Right: Next Track
    multimediaKeystroke(MMKEY_NEXT_TRACK); // Bit 0: Next Track
    break;
  case AMIGA_KEY_ARROW_LEFT:               // HELP + Arrow Left: Previous Track
    multimediaKeystroke(MMKEY_PREV_TRACK); // Bit 1: Previous Track
    break;
  case AMIGA_KEY_RETURN:                   // HELP + Enter: Play/Pause
    multimediaKeystroke(MMKEY_PLAY_PAUSE); // Bit 3: Play/Pause
    break;
  case AMIGA_KEY_SPACE:              // HELP + Space: Stop
    multimediaKeystroke(MMKEY_STOP); // Bit 2: Stop
    break;
  case AMIGA_KEY_M:                  // HELP + Right ALT: Mute
    multimediaKeystroke(MMKEY_MUTE); // Bit 4: Mute
    break;
#endif
  default:
    break;
  }
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

void keyPress(uint8_t keyCode)
{
#if DEBUG_MODE
  Serial.print("Key press: ");
  Serial.println(keyCode, HEX);
#endif
  uint8_t hidCode = keyTable[keyCode];
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
  sendReport();
#if DEBUG_MODE
  printKeyReport();
#endif
  record_last_report();
}

void keyRelease(uint8_t keyCode)
{
#if DEBUG_MODE
  Serial.print("Key release: ");
  Serial.println(keyCode, HEX);
#endif
  uint8_t hidCode = keyTable[keyCode];
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
  sendReport();
#if DEBUG_MODE
  printKeyReport();
#endif
}

void keystroke(uint8_t keyCode, uint8_t modifiers)
{
#if DEBUG_MODE
  Serial.print("Keystroke: ");
  Serial.print(keyCode, HEX);
  Serial.print(" with modifiers: ");
  Serial.println(modifiers, HEX);
#endif
  uint8_t originalModifiers = keyReport.modifiers;
  for (uint8_t i = 0; i < 6; i++)
  {
    if (keyReport.keys[i] == 0)
    {
      keyReport.keys[i] = keyCode;
      keyReport.modifiers = modifiers;
      sendReport();
      delay(PROGRAMMATIC_KEYS_RELEASE);
      keyReport.keys[i] = 0;
      keyReport.modifiers = originalModifiers;
      sendReport();
      break;
    }
  }
#if DEBUG_MODE
  printKeyReport();
#endif
}

void multimediaKeystroke(uint8_t keyCode)
{
  sendMultimediaKey(keyCode);
  delay(PROGRAMMATIC_KEYS_RELEASE); // Pause for release
  releaseMultimediaKey(keyCode);
}

void startRecording()
{
  if (!recording)
  {
#if DEBUG_MODE
    Serial.println("Start recording macro");
#endif
    noInterrupts(); // Disable interrupts to enter critical section
    stopAllMacros();
    recordingMacroIndex = 0;
    recordingMacroSlot = 0;
    recordingSlot = false;
    recording = true;
    interrupts(); // Enable interrupts to exit critical section
  }
}

void stopRecording()
{
  if (recording)
  {
#if DEBUG_MODE
    Serial.println("Stop recording macro");
#endif
    noInterrupts(); // Disable interrupts to enter critical section
    recording = false;
    recordingSlot = false;
    // Save macros to EEPROM
    saveMacrosToEEPROM();
    interrupts(); // Enable interrupts to exit critical section
  }
}

void cleanMacros()
{
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    memset(macros[i].keyReports, 0, sizeof(macros[i].keyReports));
    macros[i].length = 0;
  }
}

void resetMacros()
{
#if DEBUG_MODE
  Serial.println("Reset macros");
#endif
  noInterrupts(); // Disable interrupts to enter critical section
  stopAllMacros();
  releaseAll();
  cleanMacros();
  saveMacrosToEEPROM();
  interrupts(); // Enable interrupts to exit critical section
}

void playMacroSlot(uint8_t slot)
{
  noInterrupts(); // Disable interrupts to enter critical section
  if (!macroPlayStatus[slot].playing && nMacrosPlaying()<CONCURENT_MACROS)
  {
#if DEBUG_MODE
    Serial.print("Play macro slot: ");
    Serial.println(slot);
#endif
    macroPlayStatus[slot].macroIndex = 0;
    if (macro_looping)
    {
      macroPlayStatus[slot].loop = true;
    }
    else
    {
      macroPlayStatus[slot].loop = false;
    }
    macroPlayStatus[slot].playing = true;
  }
  else
  {
// togle playing
#if DEBUG_MODE
    Serial.print("Stop Play macro slot: ");
    Serial.println(slot);
#endif
    macroPlayStatus[slot].playing = false;
    macroPlayStatus[slot].loop = false;
    macroPlayStatus[slot].macroIndex = 0;
  }
  interrupts(); // Enable interrupts to exit critical section
}

// Check if any macro is playing
bool isMacroPlaying()
{
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    if (macroPlayStatus[i].playing)
    {
      return true;
    }
  }
  return false;
}

uint8_t nMacrosPlaying()
{
  uint8_t n = 0;
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    if (macroPlayStatus[i].playing)
    {
      n++;
    }
  }
  return n;
}

// Stop all playing macros
void stopAllMacros()
{
  for (int i = 0; i < MACRO_SLOTS; i++)
  {
    if (macroPlayStatus[i].playing)
    {
      macroPlayStatus[i].playing = false;
      macroPlayStatus[i].loop = false;
      macroPlayStatus[i].macroIndex = 0;
    }
  }
  releaseAll();
}

void playMacro()
{
  static uint32_t lastMacroTime = 0;

  if (millis() - lastMacroTime >= MACRO_DELAY)
  {
    for (uint8_t macro_slot = 0; macro_slot < MACRO_SLOTS; macro_slot++)
    {
      // Check if the macro is currently playing
      if (macroPlayStatus[macro_slot].playing)
      {
        if (macroPlayStatus[macro_slot].macroIndex < macros[macro_slot].length)
        {
          // Add keys from the macro report
          for (uint8_t i = 0; i < 6; i++)
          {
            uint8_t macroKey = macros[macro_slot].keyReports[macroPlayStatus[macro_slot].macroIndex].keys[i];
            if (macroKey != 0)
            {
              // Find an empty slot in the merged report
              for (uint8_t j = 0; j < 6; j++)
              {
                if (keyReport.keys[j] == 0)
                {
                  keyReport.keys[j] = macroKey;
                  break;
                }
              }
            }
          }

          // Merge modifiers
          keyReport.modifiers |= macros[macro_slot].keyReports[macroPlayStatus[macro_slot].macroIndex].modifiers;

          // Send the merged key report
          sendReport();
          delay(PROGRAMMATIC_KEYS_RELEASE);

          // Remove the keys added by the macro
          for (uint8_t i = 0; i < 6; i++)
          {
            uint8_t macroKey = macros[macro_slot].keyReports[macroPlayStatus[macro_slot].macroIndex].keys[i];
            if (macroKey != 0)
            {
              // Find and remove the key from the merged report
              for (uint8_t j = 0; j < 6; j++)
              {
                if (keyReport.keys[j] == macroKey)
                {
                  keyReport.keys[j] = 0;
                  break;
                }
              }
            }
          }

          // Remove the modifiers added by the macro
          keyReport.modifiers &= ~macros[macro_slot].keyReports[macroPlayStatus[macro_slot].macroIndex].modifiers;

          // Send the updated report to release the keys
          sendReport();
          // Move to the next report in the macro
          macroPlayStatus[macro_slot].macroIndex++;
        }

        // Check if the macro has completed
        if (macroPlayStatus[macro_slot].macroIndex >= macros[macro_slot].length)
        {
          if (macroPlayStatus[macro_slot].loop)
          {
            // Reset to the beginning of the macro if looping
            macroPlayStatus[macro_slot].macroIndex = 0;
          }
          else
          {
            // Stop playing if not looping
            macroPlayStatus[macro_slot].playing = false;
            macroPlayStatus[macro_slot].macroIndex = 0;
          }
        }
      }
    }

    // Update the last macro time for the next iteration
    lastMacroTime = millis();
  }
}

void record_last_report()
{
  if (recording && recordingSlot && recordingMacroIndex < MAX_MACRO_LENGTH)
  {
    noInterrupts(); // Disable interrupts to enter critical section
#if DEBUG_MODE
    Serial.print("Recording key report at index: ");
    Serial.println(macroIndex);
#endif
    memcpy(&macros[recordingMacroSlot].keyReports[recordingMacroIndex], &keyReport, sizeof(KeyReport));
    recordingMacroIndex++;
    macros[recordingMacroSlot].length = recordingMacroIndex;
    // Check if the last index was recorded
    if (recordingMacroIndex >= MAX_MACRO_LENGTH)
    {
      stopRecording();
    }
    else
    {
      interrupts(); // Enable interrupts to exit critical section
    }
  }
}

uint8_t macroSlotFromKeyCode(uint8_t keyCode)
{
  uint8_t slot = keyCode - 0x55;
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