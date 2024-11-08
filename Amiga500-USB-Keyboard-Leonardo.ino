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

#include <Arduino.h>
#include <avr/iom32u4.h>
#include <Keyboard.h>
#include <HID.h>

// Define bit masks for keyboard and joystick inputs
#define BITMASK_A500CLK 0b00010000    // IO 8
#define BITMASK_A500SP  0b00100000    // IO 9
#define BITMASK_A500RES 0b01000000    // IO 10
#define BITMASK_JOY1    0b10011111    // IO 0..4,6
#define BITMASK_JOY2    0b11110011    // IO A0..A5    

// Enumerate keyboard states
enum KeyboardState {
  SYNCH_HI = 0,
  SYNCH_LO,
  HANDSHAKE,
  READ,
  WAIT_LO,
  WAIT_RES
};

// Global variables
KeyReport keyReport;
uint32_t handshakeTimer = 0;

// Joystick states
uint8_t currentJoy1State = 0;
uint8_t currentJoy2State = 0;
uint8_t previousJoy1State = 0xFF;  // Initialize to 0xFF so that initial state triggers update
uint8_t previousJoy2State = 0xFF;

// Keyboard state machine variables
KeyboardState keyboardState = SYNCH_HI;
uint8_t bitIndex = 0;
uint8_t currentKeyCode = 0;
bool functionMode = false;  // Indicates if 'Help' key is active
bool isKeyDown = false;

const uint8_t keyTable[0x68] = {
  // Tilde, 1-9, 0, sz, Accent, backslash, Num 0 (00 - 0F)
  0x35, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x2D, 0x2E, 0x31,    0, 0x62,
  // Q to '+', '-', Num 1, Num 2, Num3 (10 - 1F)
  0x14, 0x1A, 0x08, 0x15, 0x17, 0x1C, 0x18, 0x0C,
  0x12, 0x13, 0x2F, 0x30,    0, 0x59, 0x5A, 0x5B,
  // A to '#', '-', Num 4, Num 5, Num 6 (20 - 2F)
  0x04, 0x16, 0x07, 0x09, 0x0A, 0x0B, 0x0D, 0x0E,
  0x0F, 0x33, 0x34, 0x32,    0, 0x5C, 0x5D, 0x5E,
  // '<>', Y to '-', '-', Num '.', Num 7, Num 8, Num 9 (30 - 3F)
  0x64, 0x1D, 0x1B, 0x06, 0x19, 0x05, 0x11, 0x10,
  0x36, 0x37, 0x38,    0, 0x63, 0x5F, 0x60, 0x61,
  // Space, Backspace, Tab, Enter, Return, ESC, Delete, '-', '-', '-', Num '-', '-', Up, Down, Right, Left (40 - 4F)
  0x2C, 0x2A, 0x2B, 0x58, 0x28, 0x29, 0x4C,    0,
     0,    0, 0x56,    0, 0x52, 0x51, 0x4F, 0x50,
  // F1-F10, '-', '-', Num '/', Num '*', Num '+', '-' (50 - 5F)
  0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
  0x42, 0x43,    0,    0, 0x54, 0x55, 0x57,    0,
  // Modifiers: Shift left, Shift right, '-', Ctrl left, Alt left, Alt right, Win (Amiga) left, Ctrl (Amiga) right
  0x02, 0x20, 0x00, 0x01, 0x04, 0x40, 0x08, 0x10
};

void setup() {
  // Initialize Joystick 1 (Port D)
  DDRD = (uint8_t)(~BITMASK_JOY1); // Set pins as INPUT
  PORTD = BITMASK_JOY1;            // Enable internal PULL-UP resistors
  
  // Initialize Joystick 2 (Port F)
  DDRF = (uint8_t)(~BITMASK_JOY2); // Set pins as INPUT
  PORTF = BITMASK_JOY2;            // Enable internal PULL-UP resistors

  // Initialize Keyboard (Port B)
  DDRB &= ~(BITMASK_A500CLK | BITMASK_A500SP | BITMASK_A500RES);  // Set pins as INPUT
}

void loop() {
  handleJoystick1();
  handleJoystick2();
  handleKeyboard();
}

void handleJoystick1() {
  uint8_t currentJoyState = ~PIND & BITMASK_JOY1;
  if (currentJoyState != previousJoy1State) {
    HID().SendReport(3, &currentJoyState, 1);
    previousJoy1State = currentJoyState;
  }
}

void handleJoystick2() {
  uint8_t currentJoyState = ~PINF & BITMASK_JOY2;
  if (currentJoyState != previousJoy2State) {
    HID().SendReport(4, &currentJoyState, 1);
    previousJoy2State = currentJoyState;
  }
}

void handleKeyboard() {
  uint8_t pinB = PINB;

  if (((pinB & BITMASK_A500RES) == 0) && keyboardState != WAIT_RES) {
    // Reset detected
    interrupts();
    keystroke(0x4C, 0x05);  // Send CTRL+ALT+DEL
    functionMode = false;
    keyboardState = WAIT_RES;
  } else if (keyboardState == WAIT_RES) {
    // Waiting for reset end
    if ((pinB & BITMASK_A500RES) != 0) {
      keyboardState = SYNCH_HI;
    }
  } else if (keyboardState == SYNCH_HI) {
    // Sync Pulse High
    if ((pinB & BITMASK_A500CLK) == 0) {
      keyboardState = SYNCH_LO;
    }
  } else if (keyboardState == SYNCH_LO) {
    // Sync Pulse Low
    if ((pinB & BITMASK_A500CLK) != 0) {
      keyboardState = HANDSHAKE;
    }
  } else if (keyboardState == HANDSHAKE) {
    // Handshake
    if (handshakeTimer == 0) {
      DDRB |= BITMASK_A500SP;    // Set SP pin as OUTPUT
      PORTB &= ~BITMASK_A500SP;  // Set SP pin LOW
      handshakeTimer = millis();
    } else if (millis() - handshakeTimer > 10) {
      handshakeTimer = 0;
      DDRB &= ~BITMASK_A500SP;   // Set SP pin as INPUT
      keyboardState = WAIT_LO;
      currentKeyCode = 0;
      bitIndex = 7;
    }
  } else if (keyboardState == READ) {
    // Read key message (8 bits)
    if ((pinB & BITMASK_A500CLK) != 0) {
      if (bitIndex--) {
        currentKeyCode |= ((pinB & BITMASK_A500SP) == 0) << bitIndex;  // Accumulate bits
        keyboardState = WAIT_LO;
      } else {
        // Read last bit (key down/up)
        isKeyDown = ((pinB & BITMASK_A500SP) != 0);  // true if key down
        interrupts();
        keyboardState = HANDSHAKE;
        processKeyCode();
      }
    }
  } else if (keyboardState == WAIT_LO) {
    // Waiting for the next bit
    if ((pinB & BITMASK_A500CLK) == 0) {
      noInterrupts();
      keyboardState = READ;
    }
  }
}

void processKeyCode() {
  if (currentKeyCode == 0x5F) {
    // 'Help' key toggles function mode
    functionMode = isKeyDown;
  } else if (currentKeyCode == 0x62) {
    // CapsLock key
    keystroke(0x39, 0x00);
  } else {
    if (isKeyDown) {
      // Key down message received
      if (functionMode) {
        // Special function with 'Help' key
        handleFunctionModeKey();
      } else {
        if (currentKeyCode == 0x5A) {
          keystroke(0x26, 0x20);  // '('
        } else if (currentKeyCode == 0x5B) {
          keystroke(0x27, 0x20);  // ')'
        } else if (currentKeyCode < 0x68) {
          keyPress(currentKeyCode);
        }
      }
    } else {
      // Key release message received
      if (currentKeyCode < 0x68) {
        keyRelease(currentKeyCode);
      }
    }
  }
}

void handleFunctionModeKey() {
  switch (currentKeyCode) {
    case 0x50: keystroke(0x44, 0); break; // F11
    case 0x51: keystroke(0x45, 0); break; // F12
    case 0x5A: keystroke(0x53, 0); break; // NumLock
    case 0x5B: keystroke(0x47, 0); break; // ScrollLock
    case 0x5D: keystroke(0x46, 0); break; // PrtSc
    case 0x0F: keystroke(0x49, 0); break; // Insert
    case 0x3C: keystroke(0x4C, 0); break; // Delete
    case 0x1F: keystroke(0x4E, 0); break; // Page Down
    case 0x3F: keystroke(0x4B, 0); break; // Page Up
    case 0x3D: keystroke(0x4A, 0); break; // Home
    case 0x1D: keystroke(0x4D, 0); break; // End
    case 0x52: keystroke(0x7F, 0); break; // Mute
    case 0x53: keystroke(0x81, 0); break; // Volume Down
    case 0x54: keystroke(0x80, 0); break; // Volume Up
    case 0x55: keystroke(0x82, 0); break; // Play/Pause
    case 0x56: keystroke(0x85, 0); break; // Stop
    case 0x57: keystroke(0x86, 0); break; // Previous Track
    case 0x58: keystroke(0x87, 0); break; // Next Track
    case 0x59: keystroke(0x65, 0); break; // Application/Special Key
    default: break;
  }
}

void keyPress(uint8_t keyCode) {
  uint8_t hidCode = keyTable[keyCode];
  if (keyCode > 0x5F) {
    keyReport.modifiers |= hidCode;  // Modifier key
  } else {
    for (uint8_t i = 0; i < 6; i++) {
      if (keyReport.keys[i] == 0) {
        keyReport.keys[i] = hidCode;
        break;
      }
    }
  }
  HID().SendReport(2, &keyReport, sizeof(keyReport));
}

void keyRelease(uint8_t keyCode) {
  uint8_t hidCode = keyTable[keyCode];
  if (keyCode > 0x5F) {
    keyReport.modifiers &= ~hidCode;  // Modifier key
  } else {
    for (uint8_t i = 0; i < 6; i++) {
      if (keyReport.keys[i] == hidCode) {
        keyReport.keys[i] = 0;
      }
    }
  }
  HID().SendReport(2, &keyReport, sizeof(keyReport));
}

void keystroke(uint8_t keyCode, uint8_t modifiers) {
  uint8_t originalModifiers = keyReport.modifiers;
  for (uint8_t i = 0; i < 6; i++) {
    if (keyReport.keys[i] == 0) {
      keyReport.keys[i] = keyCode;
      keyReport.modifiers = modifiers;
      HID().SendReport(2, &keyReport, sizeof(keyReport));
      keyReport.keys[i] = 0;
      keyReport.modifiers = originalModifiers;
      HID().SendReport(2, &keyReport, sizeof(keyReport));
      break;
    }
  }
}
