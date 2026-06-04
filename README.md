# 🌡️ ATmega328P Digital Thermometer (DS18B20 & Multiplexed Display)

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Microcontroller](https://img.shields.io/badge/MCU-ATmega328P-orange.svg)
![Protocol](https://img.shields.io/badge/Protocol-1--Wire-green.svg)

A bare-metal embedded C project for the **ATmega328P** microcontroller that reads precise temperature data from a **DS18B20 1-Wire Digital Sensor** and outputs the formatted data to a custom **Multiplexed 4-Digit 7-Segment Display**. 

Developed for the **EE3032 Embedded Systems Lab**, this firmware demonstrates how to handle strictly-timed digital communication protocols alongside hardware display multiplexing without causing timing collisions or screen flickering.

---

## 📸 Hardware Setup

*(Upload your images to the repository and replace the filenames below)*

<div align="center">
  <img src="board_photo.jpg" alt="EE3032 Lab Board Setup" width="45%">
  <img src="sensor_photo.jpg" alt="DS18B20 Wiring" width="45%">
</div>

> **Left:** The custom EE3032 lab board showing the active multiplexed display rendering the temperature.  
> **Right:** The waterproof DS18B20 probe wired with the mandatory 4.7kΩ pull-up resistor.

---

## 🧠 Key Technical Features

* **Bare-Metal 1-Wire Implementation:** Implements the complete 1-Wire protocol (`reset`, `read_bit`, `write_bit`, `read_byte`, `write_byte`) from scratch without external libraries, using precise `_delay_us()` timings.
* **Non-Blocking Active Display Loop:** The DS18B20 requires ~750ms to perform a temperature conversion. Standard blocking delays would turn off the multiplexed display. This firmware uses an interleaved active delay loop to constantly refresh the `ssdDisplay()` while the sensor computes, keeping the UI perfectly stable.
* **Complex Pin Mapping & Masking:** The custom lab board splits the 7-segment display pins across `PORTD` and `PORTC`. The software uses advanced bit-masking to route the segment bits correctly without disrupting other pins.
* **Custom Character Rendering:** Includes a custom hex array to render alphanumeric characters (e.g., `24°C` or `Err `) on the 7-segment display.
* **Hardware Fault Detection:** Catches failed 1-Wire initializations (e.g., sensor unplugged or missing pull-up resistor) and automatically displays an `Err ` (Error) state.

---

## ⚙️ Hardware Pinout & Wiring

### Microcontroller
* **MCU:** ATmega328P (DIP-28)
* **Clock Source:** 1 MHz Internal Oscillator (`F_CPU 1000000UL`)

### DS18B20 Sensor
| Sensor Wire | Connection | Notes |
| :--- | :--- | :--- |
| **GND (Black)** | Ground | |
| **VCC (Red)** | 5V | |
| **DQ / Data (Yellow)** | `PB0` (Pin 14) | **Critical:** Requires a 4.7kΩ pull-up resistor connected between DQ and 5V. |

### Multiplexed 4-Digit 7-Segment Display
* **Digit Selectors (T30-T33):** `PC0`, `PC1`, `PC2`, `PC3`
* **Segments (Lower 6 Bits):** `PD0` to `PD5`
* **Segments (Upper 2 Bits):** `PC4`, `PC5`

---

## 🛠️ Software Architecture

1. **`onewire_*` Functions:** Handles all microsecond-level GPIO toggling required to talk to the DS18B20.
2. **`setSegments(uint8_t seg_data)`:** A custom routing function that takes a standard 8-bit segment byte and splits it safely across `PORTD` and `PORTC`.
3. **`ssdDisplay()`:** The multiplexing engine. It rapidly cycles through digits 0 to 3, turning them on and off with a 2ms delay persistence of vision (POV) effect.
4. **Main Loop:** Initiates temperature conversion, actively runs the display multiplexer for ~760ms, reads the scratchpad, formats the data into tens/ones/symbols, and handles error states.

---

## 🚀 How to Compile & Flash

Ensure you have the AVR toolchain (`avr-gcc`, `avr-libc`, `avrdude`) installed.

**1. Compile the code:**
```bash
avr-gcc -Os -DF_CPU=1000000UL -mmcu=atmega328p -c -o main.o main.c
avr-gcc -mmcu=atmega328p main.o -o main.elf
avr-objcopy -O ihex -R .eeprom main.elf main.hex
