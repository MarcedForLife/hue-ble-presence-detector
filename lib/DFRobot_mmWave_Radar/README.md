# DFRobot_mmWave_Radar
- This 24GHz millimeter-wave radar sensor employs FMCW, CW multi-mode modulation and separate transmitter and receiver antenna structure. In working, the sensor first emits FMCW and CW radio waves to the sensing area. Next, the radio waves, reflected by all targets which are in moving, micro-moving, or extremely weak moving state in the area, are converted into electrical signals by the millimeter-wave MMIC circuit in the sensor system. After that, these signals will be sent to the processor and processed through the related signal and data algorithms. Then, the target information can be solved out.
- The millimeter-wave radar can sense the human presence, stationary and moving people within the detection area. Moreover, it can even detect static or stationary human presence such as a sleeping person. There are two ways provided to output detection result: serial port and I/O port switch quantity. Besides that, the sensor module features strong sensing reliability, high sensitivity, small size, easy to be used or embedded in applications.

## Table of Contents

* [Installation](#installation)
* [Methods](#methods)
* [History](#history)
* [Credits](#credits)

## Installation

To use this library, first download the library file, paste it into the \Arduino\libraries directory, then open the examples folder and run the demo in the folder.

## Methods

```C++
    /**
      @brief Constructor
      @param Stream Software serial port interface 
    */
    DFRobot_mmWave_Radar(Stream *s);

    /**
      @brief  Configure sensor detection area
      @param parA_s The sensing area distance starting value of the first segment, unit: m 
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing distance area) 
    */
    void DetRangeCfg(float parA_s, float parA_e);


    /**
      @brief  Configure sensor detection area 
      @param parA_s The sensing area distance starting value of the first segment, unit: m
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing distance area) 
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parB_e The sensing area distance ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e);


    /**
      @brief  Configure sensor sensing area
      @param parA_s The sensing area distance starting value of the first segment, unit: m
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing distance area)
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parB_e The sensing area distance ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parC_s The sensing area distance starting value of the third segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parC_e The sensing area distance ending value of the third segment, unit: m(Must be greater than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e);


    /**
      @brief  Configure sensor sensing area
      @param parA_s The sensing area distance starting value of the first segment, unit: m
      @param parA_e The sensing area distance ending value of the first segment, unit: m(Must be greater than the starting value of the current sensing distance area)
      @param parB_s The sensing area distance starting value of the second segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parB_e The sensing area distance ending value of the second segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parC_s The sensing area distance starting value of the third segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parC_e The sensing area distance ending value of the third segment, unit: m(Must be greater than the starting value of the current sensing area)
      @param parD_s The sensing area distance starting value of the fourth segment, unit: m(Must be greater than the ending value of the previous segment sensing area)
      @param parD_e The sensing area distance ending value of the fourth segment, unit: m(Must be greater than the starting value of the current sensing area)
    */
    void DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e, float parD_s, float parD_e);


    /**
      @brief  Read whether there is anyone or something moving in the detection range
      @return  Returning true means that there is someone or something moving in the detection range; returning false represents the oppsite. 
    */
    bool readPresenceDetection(void);


    /**
      @brief Configure sensor output delay time 
      @param par1 When the target is detected, delay the output time of the sensing result, range: 0~1638.375, unit: s 
      @param par2 When the target disappears, delay the output time of the sensing result, range: 0~1638.375, unit: s 
    */
    void OutputLatency(float par1, float par2);


    /**
      @brief Restore the sensor current configuration to the factory default value  
    */
    void factoryReset(void);
```

## Compatibility

| MCU           | Work Well | Work Wrong | Untested | Remarks |
| ------------- | :-------: | :--------: | :------: | ------- |
| Arduino uno   |     √     |            |          |         |
| Mega2560      |     √     |            |          |         |
| Leonardo      |           |            |      √   |         |
| ESP32         |     √     |            |          |         |
|               |           |            |          |         |


## History

- data 2020-03-25
- version V1.0


## Credits

Written by huyujie(yujie.hu@dfrobot.com), 2020. (Welcome to our [website](https://www.dfrobot.com/))

# DFRobot_mmWave_Radar
