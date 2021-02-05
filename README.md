# ParkRTLD

This is a complete reimplementation of Animal Crossing: New Horizon's "**R**un**t**ime **L**ink E**d**itor" (rtld) binary. Based on rtld from ACNH 1.3.0.

rtld is the "first executable code belonging to the program that the system launches". For more information on the functionality of rtld, see [Switchbrew](https://switchbrew.org/wiki/Rtld).

**Note:** This was developed specifically for ACNH. It may work with other 64-bit Switch games, but your milage may vary. This will **not** work for 32-bit Switch games. For a more versatile implementation, check out [oss-rtld by Thog](https://github.com/Thog/oss-rtld).

## Compiling

1. Make sure devkitA64 is properly installed and up to date.
2. Enter the ParkRTLD directory and run `make` in your command line/terminal.

Provided you have all the requirements, ParkRTLD.nso should be outputted to the working directory.
