# Aero Oven

## What is it?

This is an *STM32F405RG* project of composite oven control board. It's based on baremetal C and CMSIS. It uses custom PCB with LCD screen, 4 controll buttons, 4 PT1000 temperature sensors, an SD Card, RTC clock and buzzer. PCB was created in Altium Designer and it's project is available on this [repo](https://github.com/plOloV2/Aero_oven_PCB).

This project is based on *CMSIS* and utalizes *FreeRTOS* and *FatFs*. Version of C used: *C23*.

## How to compile?

For copilation you will need:

- **CMake** version 4.1 or grater
- **Ninja** (I used 1.13.2)
- **Arm Toolchain for Embeded** version 23.1.0

You may need to edit `cmake/toolchain-llvm.cmake` file, as ATfE prefix will propably be different on your machine then on mine.

With tools out of the way we can build the Debug version:

```bash
git clone https://github.com/plOloV2/Heating-Chamber-Control-System.git
cd aero-oven
cmake --preset Debug
cmake --build --preset Debug 
```

Or Release:

```bash
git clone https://github.com/plOloV2/Heating-Chamber-Control-System.git
cd aero-oven
cmake --preset Release
cmake --build --preset Release 
```
