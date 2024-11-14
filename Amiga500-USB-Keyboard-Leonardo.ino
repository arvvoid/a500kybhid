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

#define MAX_MACRO_LENGTH 24 // Maximum number of key reports in a macro
#define MACRO_SLOTS 5
#define MACRO_DELAY 10 //ms
#define PERSISTENT_MACRO 1 // Save macros to EEPROM

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

// Global variables
KeyReport keyReport;
uint32_t handshakeTimer = 0;
Macro macros[MACRO_SLOTS]; // 5 macro slots
bool recording = false;
bool recordingSlot = false;
bool playing = false;
uint8_t currentMacroSlot = 0;
uint8_t macroIndex = 0;
uint32_t lastKeyPressTime = 0;

#if ENABLE_JOYSTICKS
  // Joystick states
  uint8_t currentJoy1State = 0;
  uint8_t currentJoy2State = 0;
  uint8_t previousJoy1State = 0xFF; // Initialize to 0xFF so that initial state triggers update
  uint8_t previousJoy2State = 0xFF;
#endif

// Keyboard state machine variables
KeyboardState keyboardState = SYNCH_HI;
uint8_t bitIndex = 0;
uint8_t currentKeyCode = 0;
bool functionMode = false; // Indicates if 'Help' key is active
bool isKeyDown = false;

const uint8_t keyTable[0x68] = {
    // Tilde, 1-9, 0, sz, Accent, backslash, Num 0 (00 - 0F)
    0x35, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x2D, 0x2E, 0x31, 0, 0x62,
    // Q to '+', '-', Num 1, Num 2, Num3 (10 - 1F)
    0x14, 0x1A, 0x08, 0x15, 0x17, 0x1C, 0x18, 0x0C,
    0x12, 0x13, 0x2F, 0x30, 0, 0x59, 0x5A, 0x5B,
    // A to '#', '-', Num 4, Num 5, Num 6 (20 - 2F)
    0x04, 0x16, 0x07, 0x09, 0x0A, 0x0B, 0x0D, 0x0E,
    0x0F, 0x33, 0x34, 0x32, 0, 0x5C, 0x5D, 0x5E,
    // '<>', Y to '-', '-', Num '.', Num 7, Num 8, Num 9 (30 - 3F)
    0x64, 0x1D, 0x1B, 0x06, 0x19, 0x05, 0x11, 0x10,
    0x36, 0x37, 0x38, 0, 0x63, 0x5F, 0x60, 0x61,
    // Space, Backspace, Tab, Enter, Return, ESC, Delete, '-', '-', '-', Num '-', '-', Up, Down, Right, Left (40 - 4F)
    0x2C, 0x2A, 0x2B, 0x58, 0x28, 0x29, 0x4C, 0,
    0, 0, 0x56, 0, 0x52, 0x51, 0x4F, 0x50,
    // F1-F10, '-', '-', Num '/', Num '*', Num '+', '-' (50 - 5F)
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
    0x42, 0x43, 0, 0, 0x54, 0x55, 0x57, 0,
    // Modifiers: Shift left, Shift right, '-', Ctrl left, Alt left, Alt right, Win (Amiga) left, Ctrl (Amiga) right
    0x02, 0x20, 0x00, 0x01, 0x04, 0x40, 0x08, 0x10};

#if PERSISTENT_MACRO
    // Function to calculate macros checksum
    uint8_t calculateChecksum(Macro* macros, int length)
    {
      uint8_t checksum = 0;
      uint8_t* data = (uint8_t*)macros;
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
    while (!Serial) { ; } // Wait for Serial to be ready
    Serial.println("Debug mode enabled");
  #endif
  noInterrupts(); // Disable interrupts to enter critical section
  loadMacrosFromEEPROM();
  interrupts(); // Enable interrupts to exit critical section

#if ENABLE_JOYSTICKS
  // Initialize Joystick 1 (Port D)
  DDRD = (uint8_t)(~BITMASK_JOY1); // Set pins as INPUT
  PORTD = BITMASK_JOY1;            // Enable internal PULL-UP resistors

  // Initialize Joystick 2 (Port F)
  DDRF = (uint8_t)(~BITMASK_JOY2); // Set pins as INPUT
  PORTF = BITMASK_JOY2;            // Enable internal PULL-UP resistors
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
  if (playing)
  {
    playMacro();
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

void processKeyCode()
{
#if DEBUG_MODE
  Serial.print("Processing key code: ");
  Serial.println(currentKeyCode, HEX);
#endif
  if (currentKeyCode == 0x5F)
  {
    // 'Help' key toggles function mode
    functionMode = isKeyDown;
  }
  else if (currentKeyCode == 0x62)
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

        if(recording && !recordingSlot){
          if(currentKeyCode >= 0x55 && currentKeyCode <= 0x59){
            noInterrupts(); // Disable interrupts to enter critical section
            currentMacroSlot = macroSlotFromKeyCode(currentKeyCode);
            memset(macros[currentMacroSlot].keyReports, 0, sizeof(macros[currentMacroSlot].keyReports)); // Clear macro slot
            macros[currentMacroSlot].length = 0;
            macroIndex = 0;
            recordingSlot = true;
            interrupts(); // Enable interrupts to exit critical section
            #if DEBUG_MODE
              Serial.print("Recording slot selected: ");
              Serial.println(currentMacroSlot, HEX);
            #endif
            return;
          }
        }

        if (currentKeyCode == 0x5A)
        {
          keystroke(0x53, 0); // NumLock
        }
        else if (currentKeyCode == 0x5B)
        {
          keystroke(0x47, 0); // ScrollLock
        }
        else if (currentKeyCode < 0x68)
        {
          keyPress(currentKeyCode);
        }
      }
    }
    else
    {
      // Key release message received
      if (currentKeyCode < 0x68)
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
  case 0x50:
    keystroke(0x44, 0);
    break; // F11
  case 0x51:
    keystroke(0x45, 0);
    break; // F12
  case 0x5D:
    keystroke(0x46, 0);
    break; // PrtSc
  case 0x52:
    startRecording();
    break; // Help + F3: Start recording
  case 0x53:
    stopRecording();
    break; // Help + F4: Stop recording and save
  case 0x54:
    cleanMacros();
    break; // Help + F5: Stop any playing macro and reset all key presses
  case 0x55:
  case 0x56:
  case 0x57:
  case 0x58:
  case 0x59:
    playMacroSlot(macroSlotFromKeyCode(currentKeyCode));
    break; // Help + F6 to F10: Play macro in corresponding slot
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
  uint8_t hidCode = keyTable[keyCode];
  if (keyCode > 0x5F)
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
  HID().SendReport(2, &keyReport, sizeof(keyReport));
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
  if (keyCode > 0x5F)
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
  HID().SendReport(2, &keyReport, sizeof(keyReport));
  #if DEBUG_MODE
    printKeyReport();
  #endif
  record_last_report();
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
      HID().SendReport(2, &keyReport, sizeof(keyReport));
      keyReport.keys[i] = 0;
      keyReport.modifiers = originalModifiers;
      HID().SendReport(2, &keyReport, sizeof(keyReport));
      break;
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
      noInterrupts(); // Disable interrupts to enter critical section
      playing = false;
      macroIndex = 0;
      currentMacroSlot = 0;
      recordingSlot = false;
      recording = true;
      interrupts(); // Enable interrupts to exit critical section
  }
}

void stopRecording()
{
  if(recording){
    #if DEBUG_MODE
      Serial.println("Stop recording macro");
    #endif
      noInterrupts(); // Disable interrupts to enter critical section
      macros[currentMacroSlot].length = macroIndex;
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
  playing = false;
  Keyboard.releaseAll();
  cleanMacros();
  saveMacrosToEEPROM();
  interrupts(); // Enable interrupts to exit critical section
}

void playMacroSlot(uint8_t slot)
{
#if DEBUG_MODE
  Serial.print("Play macro slot: ");
  Serial.println(slot);
#endif
  noInterrupts(); // Disable interrupts to enter critical section
  if(!playing){
    currentMacroSlot = slot;
    macroIndex = 0;
    playing = true;
  }
  interrupts(); // Enable interrupts to exit critical section
}

void playMacro()
{

  if(!playing){
    return;
  }
  if (macroIndex < macros[currentMacroSlot].length)
  {
    if (millis() - lastKeyPressTime >= MACRO_DELAY)
    {
      noInterrupts(); // Disable interrupts to enter critical section
      HID().SendReport(2, &macros[currentMacroSlot].keyReports[macroIndex], sizeof(KeyReport));
      macroIndex++;
      lastKeyPressTime = millis();
      interrupts(); // Enable interrupts to exit critical section
    }
  }
  else
  {
   noInterrupts(); // Disable interrupts to enter critical section
   playing = false;
   interrupts(); // Enable interrupts to exit critical section
  }
}

void record_last_report(){
  if (recording && recordingSlot && macroIndex < MAX_MACRO_LENGTH)
  {
    noInterrupts(); // Disable interrupts to enter critical section
    #if DEBUG_MODE
      Serial.print("Recording key report at index: ");
      Serial.println(macroIndex);
    #endif
    memcpy(&macros[currentMacroSlot].keyReports[macroIndex], &keyReport, sizeof(KeyReport));
    macroIndex++;
    macros[currentMacroSlot].length = macroIndex;
    // Check if the last index was recorded
    if (macroIndex >= MAX_MACRO_LENGTH)
    {
      stopRecording();
    }
    else{
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