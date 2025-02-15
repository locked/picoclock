# RP2040 based Alarm Clock

## Presentation

This repo is an attempt at building a Pico based alarm clock. It was created for my kids and features:

- date/time (obviously)
- alarm (plays wav file on sdcard)
- weather (current and forecast)
- epaper screen
- 3d printable case

The configuration is done remotely, and the clock synchronize from time to time (it can be forced too). The time is also fetched from a remote server.

## Repository Architecture

- **3d**: contains printable models for the case, buttons
- **fw**: the firmware, based on a lightweight "rtos"
- **pcb**: the PCBs:
  - the main PCB receiving the RP2040 board
  - the front PCB with the backlight leds
- **api**: remote api basic php script

## Pictures

![case](https://github.com/user-attachments/assets/271e02c0-0fdb-4c54-b3e2-e96bcef8c645)
