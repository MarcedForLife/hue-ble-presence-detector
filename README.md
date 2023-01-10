# Hue BLE Presence Detector
This project contains code for a standalone presence detector that runs on an ESP32 and uses a DFRobot mmWave sensor for turning on/off Philips Hue BLE bulbs.

While there are well established alternatives that integrate with Home Assistant i.e. ESPHome,
I created it while there was a Raspberry Pi shortage and I couldn't get my hands on one.

## Hardware
Below is a list of the hardware that was used for this project.
Note that other ESP32s and mmWave sensors could be used with relatively small changes.

* [Unexpected Maker TinyS3](https://esp32s3.com/tinys3.html)
* [DFRobot mmWave Sensor (SEN0395)](https://wiki.dfrobot.com/mmWave_Radar_Human_Presence_Detection_SKU_SEN0395)
* [Philips Hue BLE bulb](https://www.philips-hue.com/en-ca/p/hue-white-a19---e26-smart-bulb---75-w--4-pack-/046677563073)

### Wiring
For connecting the DFRobot mmWave SEN0395 to the UM TinyS3, I wired together the following pins:
| SEN0395           | UM TinyS3   |
|-------------------|-------------|
| TX                | RX          |
| RX                | TX          |
| IO1               | -           |
| IO2               | -           |
| GND               | GND         |
| V                 | 5V          |

Note that, IO1 and IO2 are unused as you can configure and read from the sensor over UART.
I use a simple fork of [this](https://github.com/DFRobotdl/DFRobot_mmWave_Radar) helper library from DFRobot to simplify the calls.

If wiring a different ESP32 and/or sensor, please check your specific board and/or sensor pin diagrams.

## Setup
This is a PlatformIO project that you should be able to clone, open, build and flash to your own hardware.
Please see [the PlatformIO for VSCode instructions](https://platformio.org/install/ide?install=vscode) if you haven't used PlatformIO before.

### Configuration
Configuration options for the project can be found/changed in the file [./include/config.h](./include/config.h).

## Philips Hue BLE Bulb Pairing/Bonding
If the project can't bond to one of more of your bulbs, chances are the bulb has used up all of its bonds.
To fix this the bulb needs to be reset (you can pair other devices again after bonding successfully).
I found that just resetting the bulb via the Philips Hue app didn't work and I also needed to follow the instructions found [here](https://www.reddit.com/r/esp32/comments/drfn9u/comment/hmbmxrk/?utm_source=share&utm_medium=web2x&context=3).
> Start with light off for at least 5 seconds. Turn on for 8 seconds. Turn off for 2 seconds.
> Repeat this process 5 more times, or until the light bulb flashes. The light will flash 3 times if it has been successfully reset.

Note: If you have your bulbs setup with at least one Google Home (and possibly Amazon Echo?), turn them off while resetting.
I found that mine would reconnect straight away and stop the ESP32 from bonding successfully.

## Useful Resources & Thanks
Below are some of the resources I found really helpful when creating this.
Thanks to all mentioned/responsible and others I may have missed!

* [crlogic and all of their posts on mmWave sensors](https://community.home-assistant.io/t/mmwave-wars-one-sensor-module-to-rule-them-all/453260)
* [NimBLE examples](https://github.com/h2zero/NimBLE-Arduino/tree/release/1.4/examples)
* [A list of Philips Hue BLE Services and Characteristics](https://gist.github.com/shinyquagsire23/f7907fdf6b470200702e75a30135caf3)
