/*
 This file contains config options for the Hue BLE presence sensor.
*/

#ifndef config_h
#define config_h

#include <string>

/*
 Sets the log level for the project.
 See https://github.com/thijse/Arduino-Log for different levels.
*/
const int LOG_LEVEL = 5;

// The MAC address of the bulb you want to control
const std::string BULB_MAC_ADDRESS = "fe:2e:97:4e:16:ba";

// Configures the range at which the mmWave sensor starts and ends in meters (max 9m)
const float SENSOR_DISTANCE_START = 0.0;
const float SENSOR_DISTANCE_END = 4.0;

/*
 Configures the latency of the mmWave sensor in seconds
 i.e. how long before the sensor starts/stops reporting presence.
*/
const float SENSOR_PRESENCE_LATENCY = 0.0;
const float SENSOR_ABSENCE_LATENCY = 5.0;

/* 
 Enabling this stops the sensor from controlling the bulb if it detects that the bulb was turned on/off by something else.
 The sensor will resume control once it detects that the bulb has been turned back on/off.
 This is useful if used in a bedroom and occupants don't want the bulb accidentally turning on while sleeping.
*/
const bool PAUSE_ON_EXTERNAL_CONTROL = false;

#endif
