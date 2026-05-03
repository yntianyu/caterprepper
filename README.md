# 🐛 CaterPrepper

**A smart modular coat rack shaped like a caterpillar that physically reacts to live weather and your personal schedule — helping you prepare for the day.**

CaterPrepper uses reed switches to detect whether items (coats, umbrellas, keys) are hanging on its hooks, and provides physical feedback through light, vibration, and sound to remind you to grab everything you need before heading out. Instead of easy-to-ignore phone notifications, CaterPrepper turns your coat rack into an emotionally engaging companion that guides you through your morning departure routine.

---

## Libraries Used

| Library | Install | What It Does |
|---------|---------|-------------|
| **SoftwareSerial** | Built-in (no install needed) | Creates a serial connection on digital pins 10 (RX) and 11 (TX) to communicate with the DFPlayer Mini MP3 module. This keeps the hardware serial port (pins 0/1) free for debugging output via the Serial Monitor. |
| **DFRobotDFPlayerMini** | Install via Library Manager | Provides a high-level interface to control the DFPlayer Mini — setting volume, playing specific MP3 tracks from an SD card by number, stopping playback, and detecting when a track finishes so it can be looped or advanced. |

---

## Setup Instructions

### 1. Install the Arduino IDE

Download and install the [Arduino IDE](https://www.arduino.cc/en/software) (version 2.x recommended).

### 2. Install Required Libraries

Open the Arduino IDE Library Manager (**Sketch → Include Library → Manage Libraries**), search for `DFRobotDFPlayerMini`, and install it. The `SoftwareSerial` library ships with the Arduino IDE and requires no separate installation.

### 3. Connect the Hardware

Wire the components to your Arduino UNO as follows:

| Component | Arduino Pin | Notes |
|-----------|------------|-------|
| Reed Switch 1 | A3 | `INPUT_PULLUP` — reads LOW when magnet present |
| Reed Switch 2 | A0 | `INPUT_PULLUP` — reads LOW when magnet present |
| Reed Switch 3 | A1 | `INPUT_PULLUP` — reads LOW when magnet present |
| Reed Switch 4 | A2 | `INPUT_PULLUP` — reads LOW when magnet present |
| Touch Sensor | 2 | Capacitive touch — reads HIGH when touched |
| Vibration Motor | 9 | Digital output via transistor/MOSFET |
| Red LED | 5 | With current-limiting resistor |
| Green LED | 6 | With current-limiting resistor |
| DFPlayer Mini RX | 10 | SoftwareSerial TX → DFPlayer RX (use 1kΩ resistor) |
| DFPlayer Mini TX | 11 | SoftwareSerial RX ← DFPlayer TX |

### 4. Prepare the SD Card

1. Format a micro SD card as **FAT32**
2. Create a folder named `mp3` in the root directory
3. Place your audio files inside, named sequentially:
   - `0001.mp3` — Welcome / startup sound
   - `0002.mp3` — Voice prompt for hook 2 item (e.g., "Don't forget your umbrella!")
   - `0003.mp3` — Voice prompt for hook 3 item
   - `0004.mp3` — Voice prompt for hook 4 item
   - `0005.mp3` — Voice prompt for hook 1 item
   - `0006.mp3` — All-clear success chime

### 5. Upload and Run

1. Open `caterprepper.ino` in the Arduino IDE
2. Select **Tools → Board → Arduino UNO**
3. Select your serial port under **Tools → Port**
4. Click **Upload**
5. Open **Serial Monitor** (Tools → Serial Monitor) at **9600 baud** to see real-time reed switch readings for debugging

---

## Usage Instructions

### How to Interact with CaterPrepper

1. **Hang your items** on the caterpillar's hooks. Each hook has a reed switch, and each item should have a small magnet attached — when the magnet is close to the hook, the reed switch detects it.

2. **Tap the touch sensor** on the caterpillar's head when you're ready to leave the house.

3. **Watch the startup sequence**: CaterPrepper plays a welcome sound (track 1) and blinks the green LED for about 3 seconds.

4. **Follow the prompts**: After 3 seconds, CaterPrepper checks each hook in sequence:
   - If an item is still hanging → the **red LED** turns on, the **vibration motor** activates, and a **voice prompt** plays telling you which item to grab.
   - **Remove the item** from the hook to silence the prompt and advance to the next hook.

5. **All clear**: Once every item has been collected (or if nothing was hanging to begin with), the **green LED** turns on and a **success chime** plays. You're ready to go!

6. **Override**: At any point during the checking sequence, you can tap the touch sensor again to skip directly to the "all clear" state.

7. **Reset**: Tap the touch sensor one more time from the green "done" state to return to idle.

---

## State Machine

The firmware uses a finite state machine with 10 states:

| State | Outputs | Next State |
|-------|---------|------------|
| `IDLE` | All off | Touch → `START_DELAY` (items present) or `DONE_GREEN` (clear) |
| `START_DELAY` | Green LED blinks, track 1 plays | After 3s → first reed check |
| `ACTIVE_REED2` | Red LED + motor + track 2 | Reed 2 cleared → `WAIT_REED3` |
| `WAIT_REED3` | Red LED + motor | Reed 3 detected → `ACTIVE_REED3`; all clear → `DONE_GREEN` |
| `ACTIVE_REED3` | Red LED + motor + track 3 | Reed 3 cleared → `WAIT_REED4` |
| `WAIT_REED4` | Red LED + motor | Reed 4 detected → `ACTIVE_REED4`; all clear → `DONE_GREEN` |
| `ACTIVE_REED4` | Red LED + motor + track 4 | Reed 4 cleared → `WAIT_REED1` |
| `WAIT_REED1` | Red LED + motor | Reed 1 detected → `ACTIVE_REED1`; all clear → `DONE_GREEN` |
| `ACTIVE_REED1` | Red LED + motor + track 5 | Reed 1 cleared & all clear → `DONE_GREEN` |
| `DONE_GREEN` | Green LED + track 6 | Touch → `IDLE` |

Touching the sensor during any active/wait state skips directly to `DONE_GREEN`.

---

## Credits

**CS 320 — Team 5**

- **Shannon Li** 
- **Eva Theetge** 
- **Yixi Gao** 
- **Elizabeth Yan**

### External Resources

- [DFRobotDFPlayerMini Library](https://github.com/DFRobot/DFRobotDFPlayerMini) — MP3 module control
- [Arduino SoftwareSerial](https://docs.arduino.cc/learn/built-in-libraries/software-serial/) — Serial communication on digital pins
- Caterpillar mascot graphic — sourced from pngtree.com
