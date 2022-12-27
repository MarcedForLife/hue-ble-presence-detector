/*!
   @file  DFRobot_mmWave_Radar.h
   @brief  Define the basic structure of class mmWave radar sensor-human presence detection 
   @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
   @licence  The MIT License (MIT)
   @author  huyujie(yujie.hu@dfrobot.com)
   @version  V1.0
   @date  2020-2-25
   @get  from https://www.dfrobot.com
   @url  https://github.com/DFRobot
*/


#ifndef __DFRobot_mmWave_Radar_H__
#define __DFRobot_mmWave_Radar_H__

#include <Arduino.h>




class DFRobot_mmWave_Radar
{
  public:

    /**
      @brief Constructor 
      @param Stream  Software serial port interface 
    */
    DFRobot_mmWave_Radar(Stream *s);

    /**
      @brief  Configure sensor detection area  
      @param parA_s The sensing area distance starting value of the first segment, unit: m  
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e);


    /**
      @brief  Configure sensor detection area 
      @param parA_s The sensing area distance starting value of the first segment, unit: m
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must greater than the starting value of the current sensing area)
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greater than the ending value of the previous segment sensing area) 
      @param parB_e The sensing area ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e);


    /**
      @brief  Configure sensor detection area 
      @param parA_s The sensing area distance starting value of the first segment, unit: m
      @param parA_e The sensing area distance ending value of the first segment, unit: m (Must be greater than the starting value of the current sensing area)
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parB_e The sensing area distance ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parC_s The sensing area distance starting value of the third segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parC_e The sensing area distance ending value of the third segment, unit: m(Must be greaer than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e);


    /**
      @brief  Configure sensor detection area 
      @param parA_s The sensing area distance starting value of the first segment, unit: m 
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greatet than the ending value of the previous segment sensing area)
      @param parB_e The sensing area distance ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parC_s The sensing area distance starting value of the third segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parC_e The sensing area distance ending value of the third segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parD_s The sensing area distance starting value of the fourth segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parD_e The sensing area ditance ending value of the fourth segment, unit: m(Must be greater than the starting value of the current sensin area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e, float parD_s, float parD_e);


    /**
      @brief  Read whether there is people or object moving in the sensing area
      @return  Returning true means that there is people or animal moving in the detection range; false means the opposite
    */
    bool readPresenceDetection(void);


    /**
      @brief  Configure sensor output delay time 
      @param par1 When a target detected, delay the output time of sensing result, range：0~1638.375, unit: s 
      @param par2 When the target disappears, delay the output time of sensing result, range: 0~1638.375, unit: s
    */
    void OutputLatency(float par1, float par2);


    /**
      @brief  Restore the sensor current configuration to the factory settings. 
    */
    void factoryReset(void);

  private:


    /**
      @brief  Read data in serial port 
      @param  buf Store the read data 
      @param  len The byte length to be read 
      @return  Return the actual read bytes
    */
    size_t readN(uint8_t *buf, size_t len);


    /**
      @brief  Read complete data packet 
      @param  buf Store the read data
      @return Return true means it succeeds, false means it failed 
    */
    bool recdData(uint8_t *buf);


#define DELAY 1000      //Command sending interval (The test states that the interval between two commands must be larger than 1000ms)

    Stream *_s;
    char *comStop = "sensorStop";     //Sensor stop command. Stop the sensor when it is still running 
    char *comStart = "sensorStart";     //Sensor start command. When the sensor is not started and there are no set parameters to save, start the sensor to run 
    char *comSaveCfg = "saveCfg 0x45670123 0xCDEF89AB 0x956128C6 0xDF54AC89";     //Parameter save command. When the sensor parameter is reconfigured via serialport but no tsaved, use this command to save the new configuration into sensor Flash 
    char *comFactoryReset = "factoryReset 0x45670123 0xCDEF89AB 0x956128C6 0xDF54AC89";     //Factory settings restore command. Restore the sensor to the factory default settings 
};

#endif