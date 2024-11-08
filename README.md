
# Amiga 500 Keyboard - Arduino Leonardo Wiring and Key Mappings

This section describes the wiring for connecting the Amiga 500 keyboard to an Arduino Leonardo, including the functions of each special key, particularly when used with the **Help** key.

---

## Wiring Information

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

<h1>Amiga 500 Keyboard Layout</h1>

<table>
    <tr>
        <td>Esc<br>(0x45)</td><td>F1<br>(0x50)</td><td>F2<br>(0x51)</td><td>F3<br>(0x52)</td>
        <td>F4<br>(0x53)</td><td>F5<br>(0x54)</td><td>F6<br>(0x55)</td><td>F7<br>(0x56)</td>
        <td>F8<br>(0x57)</td><td>F9<br>(0x58)</td><td>F10<br>(0x59)</td>
    </tr>
</table>

<table>
    <tr>
        <td>` ~<br>(0x00)</td><td>1 !<br>(0x01)</td><td>2 @<br>(0x02)</td><td>3 #<br>(0x03)</td>
        <td>4 $<br>(0x04)</td><td>5 %<br>(0x05)</td><td>6 ^<br>(0x06)</td><td>7 &<br>(0x07)</td>
        <td>8 *<br>(0x08)</td><td>9 (<br>(0x09)</td><td>0 )<br>(0x0A)</td><td>- _<br>(0x0B)</td>
        <td class="wide">Backspace<br>(0x41)</td>
    </tr>
</table>

<table>
    <tr>
        <td>Tab<br>(0x42)</td><td>Q<br>(0x10)</td><td>W<br>(0x11)</td><td>E<br>(0x12)</td>
        <td>R<br>(0x13)</td><td>T<br>(0x14)</td><td>Y<br>(0x15)</td><td>U<br>(0x16)</td>
        <td>I<br>(0x17)</td><td>O<br>(0x18)</td><td>P<br>(0x19)</td><td>[ {<br>(0x1A)</td><td>] }<br>(0x1B)</td>
    </tr>
</table>

<table>
    <tr>
        <td>Ctrl<br>(0x63)</td><td>Caps Lock<br>(0x62)</td><td>A<br>(0x20)</td><td>S<br>(0x21)</td>
        <td>D<br>(0x22)</td><td>F<br>(0x23)</td><td>G<br>(0x24)</td><td>H<br>(0x25)</td>
        <td>J<br>(0x26)</td><td>K<br>(0x27)</td><td>L<br>(0x28)</td><td>; :<br>(0x29)</td><td>' "<br>(0x2A)</td><td>Return<br>(0x44)</td>
    </tr>
</table>

<table>
    <tr>
        <td>Shift<br>(0x60)</td><td>Z<br>(0x31)</td><td>X<br>(0x32)</td><td>C<br>(0x33)</td>
        <td>V<br>(0x34)</td><td>B<br>(0x35)</td><td>N<br>(0x36)</td><td>M<br>(0x37)</td>
        <td>, <<br>(0x38)</td><td>. ><br>(0x39)</td><td>/ ?<br>(0x3A)</td><td>Shift<br>(0x61)</td>
    </tr>
</table>

<table>
    <tr>
        <td>Alt<br>(0x64)</td><td>Amiga (Left)<br>(0x66)</td><td class="space" colspan="5">Space<br>(0x40)</td>
        <td>Amiga (Right)<br>(0x67)</td><td>Alt<br>(0x64)</td><td>Del<br>(0x46)</td><td>Help<br>(0x5F)</td>
    </tr>
</table>

<table>
    <tr>
        <td>↑<br>(0x4C)</td>
        <td>←<br>(0x4F)</td><td>↓<br>(0x4D)</td><td>→<br>(0x4E)</td>
    </tr>
</table>

<h2>Numeric Keypad</h2>

<table>
    <tr>
        <td>( NumL<br>(0x5A)</td><td>) ScrL<br>(0x5B)</td><td>/ <br>(0x5C)</td><td>* PtrSc<br>(0x5D)</td>
    </tr>
    <tr>
        <td>7 Home<br>(0x3D)</td><td>8<br>(0x3E)</td><td>9 Pg Up<br>(0x3F)</td><td>- <br>(0x4A)</td>
    </tr>
    <tr>
        <td>4<br>(0x2D)</td><td>5<br>(0x2E)</td><td>6<br>(0x2F)</td><td>+ <br>(0x5E)</td>
    </tr>
    <tr>
        <td>1 End<br>(0x1D)</td><td>2<br>(0x1E)</td><td>3 Pg Dn<br>(0x1F)</td><td rowspan="2">Enter<br>(0x43)</td>
    </tr>
    <tr>
        <td colspan="2">0 Ins<br>(0x0F)</td><td>. Del<br>(0x3C)</td>
    </tr>
</table>


## Help Key Special Functions

The **Help** key on the Amiga 500 keyboard serves as a modifier, enabling additional functions when combined with other keys. Below are the available combinations and their corresponding functions.

| Key Combination               | Function                |
|-------------------------------|-------------------------|
| **Help**                      | Help function          |
| **Help + F1**                 | F11                    |
| **Help + F2**                 | F12                    |
| **Help + NumL** (on numpad)   | Toggle NumLock         |
| **Help + Scr L** (on numpad)  | Toggle ScrollLock      |
| **Help + Ptr Sc** (on numpad) | Print Screen           |
| **Help + Ins** (on numpad)    | Insert                 |
| **Help + Del** (on numpad)    | Delete                 |
| **Help + Pg Dn** (on numpad)  | Page Down              |
| **Help + Pg Up** (on numpad)  | Page Up                |
| **Help + Home** (on numpad)   | Home                   |
| **Help + End** (on numpad)    | End                    |
| **Help + F3**                 | Mute                   |
| **Help + F4**                 | Volume Down            |
| **Help + F5**                 | Volume Up              |
| **Help + F6**                 | Play/Pause             |
| **Help + F7**                 | Stop                   |
| **Help + F8**                 | Previous Track         |
| **Help + F9**                 | Next Track             |
| **Help + F10**                | Application/Special Key|

### Important Note about NumLock

When **NumLock** is turned **off**, the following keys on the numeric keypad function as navigation keys by default:
- **Insert**, **Delete**, **Home**, **End**, **Page Up**, and **Page Down**.
- The **arrow keys** on the numeric keypad (2, 4, 6, and 8) also function as cursor movement keys.

With **NumLock** turned **on**, these keys will function as standard numeric keys instead. Con be conbined with Help key for alternative function.

### Key Function Descriptions

- **Help**: Activates specific special functions or multimedia controls when used in combination with other keys.
- **Help + Navigation Keys**:
  - **Help + Ins**: Insert.
  - **Help + Del**: Delete.
  - **Help + Pg Dn**: Page Down.
  - **Help + Pg Up**: Page Up.
  - **Help + Home**: Home.
  - **Help + End**: End.
- **Help + F3 to F10**: Controls multimedia functions:
  - **Help + F3**: Mute the system audio.
  - **Help + F4**: Decrease the volume.
  - **Help + F5**: Increase the volume.
  - **Help + F6**: Toggle Play/Pause for media.
  - **Help + F7**: Stop media playback.
  - **Help + F8**: Go to the previous track.
  - **Help + F9**: Go to the next track.
  - **Help + F10**: Application or Special Key, can be used for opening context menus or other system functions.
- **Help + F1 and F2**: Standard F11 and F12 functions.

This wiring and mapping setup allows the Amiga 500 keyboard to interface with the Arduino Leonardo effectively, bringing additional functionality with the **Help** key for multimedia and navigation control. The setup is ideal for retrofitting the keyboard for modern applications while retaining its unique layout and feel.
