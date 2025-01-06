# M5-SYNTH-library
This is a library for MIDI playback on the M5NanoC6, designed to play sounds on the M5SYNTH.

---

### 1. Overview and Purpose
This library allows you to read and play `.mid` files stored in the SPIFFS of M5 microcontrollers. By connecting the M5NanoC6 to the M5SYNTH, you can play `.mid` files saved in the SPIFFS of the M5NanoC6.  
Additionally, you can specify the TXD and RXD pins for serial communication, enabling MIDI playback even on other microcontrollers instead of the M5NanoC6.

---

### 2. Features
- Read `.mid` files from SPIFFS.
- Send MIDI data to M5SYNTH.
- Adjustable MIDI playback volume.
- Adjustable MIDI playback speed.
- `play()` to start playback and play one event at a time when `loop()` is called.
- `stop()` to end playback.
- `isRunning()` to check if playback is in progress.
- If the number of MIDI tracks exceeds 16, the number of tracks can be specified.

---

### 3. Hardware and Software Requirements
- **Hardware**:  
  - M5SYNTH  
  - A microcontroller that supports SPIFFS.  

- **Software Libraries**:  
  - `SoftwareSerial.h`  
  - `FS.h`  
  - `SPIFFS.h`

---

### 4. Installation
*Installation instructions are omitted.*

---

### 5. Sample Code
The sample code is available in the [examples/SPIFFS_MIDI_player_example](examples/SPIFFS_MIDI_player_example) folder.  
You can open it directly in the Arduino IDE for reference and testing.

---

### 6. API Documentation
*Detailed API explanations are omitted. Please refer to the sample code.*

---

### 7. License
This library is released under the MIT License.  
