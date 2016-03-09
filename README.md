Altilog
========

This Arduino project should implement an altitude logger for my RC plane.

Hardware Setup:
----------------
- Arduino Nano
- Micro SD Card shield connected via SPI, CS on pin 10.
- BMP180 pressure sensor connected via I2C bus
- Piezzo buzzer on D5

Software features:
------------------
- Baseline pressure measurement during startup
- Relative altitude determination & logging to MicroSD card
- Serial (USB) command interface to download & delete log files (no need to remove SD card to get data)
- Buzzer to signal successful initialisation

Notes:
-------
- BMP180 needs 3.3V, Arduino Nano only provides 5V. I use a voltage divider with 2 resistors to power the BMP180.
- Based on Sparkfun BMP180 and SdFat libraries
- No RTC. All recorded times are relative to startup.

Development Plans:
------------------
- Linux client to download log files
- potential integration with flightstab stabilisator for logging of current gyro & stick values (not sure yet if this is possible)
