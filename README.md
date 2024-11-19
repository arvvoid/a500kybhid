[![Build Arduino Leonardo Kyb](https://github.com/arvvoid/Amiga500-USB-Keyboard-Leonardo/actions/workflows/verify.yml/badge.svg?branch=main)](https://github.com/arvvoid/Amiga500-USB-Keyboard-Leonardo/actions/workflows/verify.yml)

# Amiga 500 Keyboard - Arduino Leonardo

Welcome to the Amiga 500 Keyboard interfacing project! This guide will help you connect and map the iconic Amiga 500 keyboard to an Arduino Leonardo, allowing you to bring new life to this classic piece of hardware. By following the instructions provided, you can retrofit the Amiga 500 keyboard for modern applications while preserving its unique layout and feel.

For a demonstration of the original Amiga 500 keyboard in action, visit the [Amiga Undead GitHub repository](https://github.com/arvvoid/amiga.undead).

## Wiring Information

This section describes the wiring for connecting the Amiga 500 keyboard to an Arduino Leonardo.

To connect the Amiga 500 keyboard to the Arduino Leonardo, refer to the following table:

| Connector Pin | Function | Wire Color | Arduino Leonardo IO Pin |
|---------------|----------|------------|--------------------------|
| 1             | KBDATA   | Black      | 8                        |
| 2             | KBCLK    | Brown      | 9                        |
| 3             | KBRST    | Red        | 10                       |
| 4             | 5V       | Orange     | 5V                       |
| 5             | NC       | -          | -                        |
| 6             | GND      | Green      | GND                      |
| 7             | LED1     | Blue       | 5V                       |
| 8             | LED2     | Purple     | -                        |

- **KBDATA (Black, Pin 1)**: Connects to Arduino Leonardo digital pin **8**. This line transmits data from the keyboard to the Arduino.
- **KBCLK (Brown, Pin 2)**: Connects to Arduino Leonardo digital pin **9**. This line provides the clock signal for synchronization.
- **KBRST (Red, Pin 3)**: Connects to Arduino Leonardo digital pin **10**. This line allows the Arduino to send a reset signal to the keyboard.
- **5V (Orange, Pin 4)**: Connects to the **5V** power supply on the Arduino.
- **NC (Pin 5)**: Not connected.
- **GND (Green, Pin 6)**: Connects to the **GND** pin on the Arduino.
- **LED1 (Blue, Pin 7)**: Connects to **5V** for indicating power.
- **LED2 (Purple, Pin 8)**: Not connected.

---

## Amiga 500 Keyboard Layout

![Amiga 500 Keyboard Layout](https://wiki.amigaos.net/w/images/7/79/DevFig7-1.png)

Credit: [AmigaOS Wiki](https://wiki.amigaos.net/wiki/Keyboard_Device)

## Help Key Special Functions

The **Help** key on the Amiga 500 keyboard is used as a modifier in this implementation, enabling additional functions when combined with other keys. Below are the available combinations and their corresponding functions.

| Key Combination               | Function                |
|-------------------------------|-------------------------|
| **Help + F1**                 | F11                    |
| **Help + F2**                 | F12                    |
| **Help + Ptr Sc** (on numpad) | Print Screen           |
| **Help + F3**                 | Record macro           |
| **Help + F4**                 | Save macro             |
| **Help + F5**                 | Toggle looping macro   |
| **Help + Backspace**          | Stop all playing macros       |
| **Help + Del**                | Reset all macros (delete all) |
| **Help + F6**                 | Play macro slot 1      |
| **Help + F7**                 | Play macro slot 2      |
| **Help + F8**                 | Play macro slot 3      |
| **Help + F9**                 | Play macro slot 4      |
| **Help + F10**                | Play macro slot 5      |

### NumLock

When **NumLock** is turned **off**, the following keys on the numeric keypad function as navigation keys by default:
- **Insert**, **Delete**, **Home**, **End**, **Page Up**, and **Page Down**.
- The **arrow keys** on the numeric keypad (2, 4, 6, and 8) also function as cursor movement keys.

With **NumLock** turned **on**, these keys will function as standard numeric keys instead.

## Macro Recording and Playback Guide

This section explains how to use the macro recording and playback functionality of the Amiga 500 Keyboard to USB HID Converter. Macros allow you to record a sequence of key presses and play them back with a single key combination.

### Macro Slots

There are 5 macro slots available, each capable of storing up to 24 key reports. The macros are stored in EEPROM, so they persist across power cycles.
24 to keep withing the EEPROM 1kb size of the Leonardo. If you disable persistent macros flag you can go up to 45 per slot on the Leonardo but macros will not persist power cycles.

### **Recording a Macro**

1. **Start Recording**:
   - Press **Help + F3** to begin recording a macro.
   - Normal key presses will be temporarily blocked until you select a recording slot.

2. **Select a Recording Slot**:
   - Press one of the keys **F6** to **F10** to choose a recording slot (slots 1 to 5).
   - After selecting a slot, the keyboard will function normally, but every key press will be recorded until the slot is full or you stop recording.

3. **Stop Recording**:
   - Press **Help + F4** to stop recording and save the macro in the selected slot.

### **Playing a Macro**

1. **Play a Macro**:
   - Press **Help + F6** to **Help + F10** to play the macro stored in the corresponding slot (slots 1 to 5).
   - The macro will replay the recorded key presses at fixed intervals.

2. **Activate Looping** (BETA):
   - Press **Help + F5** to toggle looping mode for macros.
   - When looping is active:
     - A macro will repeat continuously once started.
     - You can deactivate looping mode to allow other macros to play just once.
   - To stop a looping macro, press the corresponding slot key again (e.g., **Help + F6**) while the macro is playing.
   - **Note:** Running too many loops with many key presses may interfere with normal keyboard functionality and introduce delays to standard key presses. Use with caution.
   - **Note:** This feature is work in progress

3. **Stop All Macros**:
   - Press **Help + Backspace** to stop all currently playing macros.

### Resetting Macros

1. **Reset Macros**:
   - Press **Help + F5** to stop any playing macro and reset all macros. WARNING this cleans and reset the EEPROM.

### Example Usage

1. **Recording a Macro**:
   - Press **Help + F3** to start recording.
   - Press **F6** to select slot 1.
   - Type the sequence of keys you want to record.
   - Press **Help + F4** to stop recording and save the macro.

2. **Playing a Macro**:
   - Press **Help + F6** to play the macro stored in slot 1.

### Notes

- Macros are stored in EEPROM, so they will persist across power cycles.
- Each macro slot can store up to 24 key reports.
- The recording will stop automatically if the macro slot is full.

# Build and Upload Guide

This guide will help you compile and upload the Amiga 500 Keyboard converter code to an Arduino Leonardo. Choose between the **Arduino CLI** (for command-line users) or the **Arduino IDE** (beginner-friendly) to get started.

---

## Option 1: Arduino CLI (Command Line)

This method is ideal for users comfortable with the command line. **Arduino CLI** allows for efficient building and uploading.

### Requirements
- Arduino Leonardo connected via USB
- Arduino CLI installed

### Steps

1. **Install Arduino CLI:**
   ```bash
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   sudo mv bin/arduino-cli /usr/local/bin/
   ```
   
2. **Configure Arduino CLI:**
   ```bash
   arduino-cli config init
   arduino-cli core update-index
   arduino-cli core install arduino:avr
   arduino-cli lib install "Keyboard"
   ```

3. **Connect Arduino Leonardo via USB** and identify the port:
   ```bash
   arduino-cli board list
   ```
   - Note your board’s port (e.g., `/dev/ttyACM0`) and FQBN (`arduino:avr:leonardo`).

4. **Compile the Sketch**:
   Navigate to the directory containing your `.ino` file (e.g., `Amiga500-USB-Keyboard-Leonardo.ino`) and run:
   ```bash
   arduino-cli compile --fqbn arduino:avr:leonardo Amiga500-USB-Keyboard-Leonardo.ino
   ```

5. **Upload the Sketch**:
   ```bash
   arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:leonardo Amiga500-USB-Keyboard-Leonardo.ino
   ```
   - Replace `/dev/ttyACM0` with your actual port if different.

6. **Test the Uploaded Sketch**:
   - Connect your Amiga 500 keyboard and verify functionality in a text editor.

---

## Option 2: Arduino IDE (Beginner-Friendly)

The **Arduino IDE** provides a graphical interface for writing, compiling, and uploading Arduino sketches.

### Steps

1. **Install the Arduino IDE**:
   - Download from [Arduino Software](https://www.arduino.cc/en/software).
  
2. **Install the Keyboard Library**:
   - In the Arduino IDE, go to Tools > Manage Libraries....
   - In the Library Manager, search for "Keyboard" and install the Keyboard library.

3. **Open Your Sketch**:
   - Launch the Arduino IDE.
   - Go to `File` > `New`, paste your code, and save it as `Amiga500-USB-Keyboard-Leonardo.ino`.

4. **Select Board and Port**:
   - Go to `Tools` > `Board` > **Arduino Leonardo**.
   - Go to `Tools` > `Port` and select the port for your Arduino Leonardo.

5. **Compile and Upload**:
   - Click **Verify** (checkmark icon) to compile.
   - Click **Upload** (arrow icon) to upload the sketch to the board.

6. **Test the Keyboard**:
   - Connect your Amiga 500 keyboard to the Arduino Leonardo.
   - Open a text editor and verify functionality.

---

### Additional Notes

- **Linux Permissions**: If you encounter permission issues on Linux, add your user to the `dialout` group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  Log out and back in for changes to take effect.

- **Troubleshooting**:
  - Ensure correct board and port selection.
  - Double-check wiring connections.

## TODO

- [ ] Add an optional Piezo Buzzer to the Leonardo to produce tones for better macro recording user feedback
