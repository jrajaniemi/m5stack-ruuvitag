# M5Stack Core2 and RuuviTag

This is a sketch for the M5Core2 microcontroller that reads the temperature, humidity, pressure, and battery voltage from a RuuviTag Bluetooth Low Energy device and displays them on the M5Core2's screen. The sketch uses the Arduino FreeRTOS library to create and manage multiple tasks.

The sketch begins by including necessary libraries and initializing some variables. It includes several custom fonts that are used to display text on the screen. It also defines a struct for storing data from the RuuviTag, and another struct for storing power consumption and battery information from the M5Core2.

The sketch then creates a BLEScan object for scanning for RuuviTags. It defines a callback function that is called when a RuuviTag is found. The callback function decodes the RuuviTag's raw data and stores the temperature, humidity, pressure, and battery voltage in the struct defined earlier.

The sketch defines five tasks:

* task1 is responsible for handling touchscreen input and waking up the screen when necessary.
* task2 is responsible for scanning for RuuviTags. It starts a scan every 180 seconds and calls the callback function defined earlier when a RuuviTag is found.
* task3 is responsible for drawing a graph of temperature data and displaying the current temperature, humidity, pressure, and power consumption on the screen. It uses the data stored in the struct defined earlier and saves the temperature data in an array for graphing.
* task4 is responsible for monitoring the battery voltage and adjusting the screen voltage accordingly. If the battery voltage falls below a certain threshold, the screen voltage is lowered to conserve power. If the battery voltage falls below another threshold, the screen is put to sleep to conserve even more power.
* task5 is responsible for monitoring power consumption and battery capacity. It updates the struct defined earlier with this information.

Finally, the setup() function initializes the M5Core2 and sets up the BLE scan. It then creates the five tasks defined earlier and sets them to run on different cores of the microcontroller. It also displays an image on the screen for 5 seconds before clearing the screen to start displaying the temperature data.

Useful links:

* https://docs.m5stack.com/en/api/core2/system
* https://github.com/m5stack/M5Stack
* https://github.com/nkolban/esp32-snippets/tree/master/Documentation
* https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
* https://www.online-utility.org/image/convert/to/XBM
* https://github.com/m5stack/M5Stack/tree/master/examples/Advanced/Display/drawXBitmap
* https://lang-ship.com/reference/unofficial/M5StickC/Class/ESP32/BLEScan/
