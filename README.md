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

## Configuration
Configuration options for the project can be found/changed in the file [./include/config.h](./include/config.h).

## Philips Hue BLE Bulb Pairing/Bonding
If the project can't bond to your bulb, chances are the bulb has used up all of its bonds.
To fix this the bulb needs to be reset (you can pair other devices again after bonding successfully).
I found resetting the bulb via the Philips Hue app didn't work and I needed to follow the instructions found [here](https://www.reddit.com/r/esp32/comments/drfn9u/comment/hmbmxrk/?utm_source=share&utm_medium=web2x&context=3).
> Start with light off for at least 5 seconds. Turn on for 8 seconds. Turn off for 2 seconds.
> Repeat this process 5 more times, or until the light bulb flashes. The light will flash 3 times if it has been successfully reset.

## Useful Resources & Thanks
Below are some of the resources I found really helpful when creating this.
Thanks to all mentioned/responsible and others I may have missed!

* [crlogic and all of their posts on mmWave sensors](https://community.home-assistant.io/t/mmwave-wars-one-sensor-module-to-rule-them-all/453260)
* [NimBLE examples](https://github.com/h2zero/NimBLE-Arduino/tree/release/1.4/examples)
* [A list of Philips Hue BLE Services and Characteristics](https://gist.github.com/shinyquagsire23/f7907fdf6b470200702e75a30135caf3)
