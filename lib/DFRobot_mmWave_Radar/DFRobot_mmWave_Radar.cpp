/*!
   @file  DFRobot_mmWave_Radar.cpp
   @brief  Implement the basic structure of class mmwave rader sensor-human presence detection 
   @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
   @licence  The MIT License (MIT)
   @author  huyujie(yujie.hu@dfrobot.com)
   @version  V1.0
   @date  2020-3-25
   @get  from https://www.dfrobot.com
   @url  https://github.com/DFRobot
*/

#include <DFRobot_mmWave_Radar.h>

DFRobot_mmWave_Radar::DFRobot_mmWave_Radar(Stream *s)
{
  _s = s;
}

size_t DFRobot_mmWave_Radar::readN(uint8_t *buf, size_t len)
{
  size_t offset = 0, left = len;
  int16_t Tineout = 1500;
  uint8_t  *buffer = buf;
  long curr = millis();
  while (left) {
    if (_s -> available()) {
      buffer[offset] = _s->read();
      offset++;
      left--;
    }
    if (millis() - curr > Tineout) {
      break;
    }
  }
  return offset;
}

bool DFRobot_mmWave_Radar::recdData(uint8_t *buf)
{

  int16_t Tineout = 50000;
  long curr = millis();
  uint8_t ch;
  bool ret = false;
  while (!ret) {
    if (millis() - curr > Tineout) {
      break;
    }

    if (readN(&ch, 1) == 1) {
      if (ch == '$') {
        buf[0] = ch;
        if (readN(&ch, 1) == 1) {
          if (ch == 'J') {
            buf[1] = ch;
            if (readN(&ch, 1) == 1) {
              if (ch == 'Y') {
                buf[2] = ch;
                if (readN(&ch, 1) == 1) {
                  if (ch == 'B') {
                    buf[3] = ch;
                    if (readN(&ch, 1) == 1) {
                      if (ch == 'S') {
                        buf[4] = ch;
                        if (readN(&ch, 1) == 1) {
                          if (ch == 'S') {
                            buf[5] = ch;
                            if (readN(&buf[6], 9) == 9) {
                              ret = true;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}


bool DFRobot_mmWave_Radar::readPresenceDetection(void)
{
  uint8_t dat[15] = {0};
  
  bool ret;
  if (recdData(dat)) {
    if (dat[7] == '1') {
      ret = true;
    } else {
      if (dat[7] == '0') {
        ret = false;
      }
    }
  } else {
    while (1);
  }
  return ret;
}


void DFRobot_mmWave_Radar::DetRangeCfg(float parA_s, float parA_e)
{
  char comDetRangeCfg[22] = {0};
  int16_t parA_S = parA_s / 0.15;
  int16_t parA_E = parA_e / 0.15;
  sprintf(comDetRangeCfg, "detRangeCfg -1 %d %d", parA_S, parA_E);


  _s->write(comStop);
  delay(DELAY);

  _s->write(comDetRangeCfg);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);

}

void DFRobot_mmWave_Radar::DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e)
{
  char comDetRangeCfg[30] = {0};
  int16_t parA_S = parA_s / 0.15;
  int16_t parA_E = parA_e / 0.15;
  int16_t parB_S = parB_s / 0.15;
  int16_t parB_E = parB_e / 0.15;

  sprintf(comDetRangeCfg, "detRangeCfg -1 %d %d %d %d", parA_S, parA_E, parB_S, parB_E);


  _s->write(comStop);
  delay(DELAY);

  _s->write(comDetRangeCfg);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);
}

void DFRobot_mmWave_Radar::DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e)
{
  char comDetRangeCfg[38] = {0};
  int16_t parA_S = parA_s / 0.15;
  int16_t parA_E = parA_e / 0.15;
  int16_t parB_S = parB_s / 0.15;
  int16_t parB_E = parB_e / 0.15;
  int16_t parC_S = parC_s / 0.15;
  int16_t parC_E = parC_e / 0.15;

  sprintf(comDetRangeCfg, "detRangeCfg -1 %d %d %d %d %d %d", parA_S, parA_E, parB_S, parB_E, parC_S, parC_E);


  _s->write(comStop);
  delay(DELAY);

  _s->write(comDetRangeCfg);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);

}

void DFRobot_mmWave_Radar::DetRangeCfg(float parA_s, float parA_e, float parB_s, float parB_e, float parC_s, float parC_e, float parD_s, float parD_e)
{
  char comDetRangeCfg[46] = {0};
  int16_t parA_S = parA_s / 0.15;
  int16_t parA_E = parA_e / 0.15;
  int16_t parB_S = parB_s / 0.15;
  int16_t parB_E = parB_e / 0.15;
  int16_t parC_S = parC_s / 0.15;
  int16_t parC_E = parC_e / 0.15;
  int16_t parD_S = parD_s / 0.15;
  int16_t parD_E = parD_e / 0.15;

  sprintf(comDetRangeCfg, "detRangeCfg -1 %d %d %d %d %d %d %d %d", parA_S, parA_E, parB_S, parB_E, parC_S, parC_E, parD_S, parD_E);


  _s->write(comStop);
  delay(DELAY);

  _s->write(comDetRangeCfg);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);

}


void DFRobot_mmWave_Radar::OutputLatency(float par1, float par2)
{

  char comOutputLatency[28] = {0};
  int16_t Par1 = par1 * 1000 / 25;
  int16_t Par2 = par2 * 1000 / 25;
  sprintf(comOutputLatency, "outputLatency -1 %d %d", Par1 , Par2);

  _s->write(comStop);
  delay(DELAY);

  _s->write(comOutputLatency);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);
}

void DFRobot_mmWave_Radar::factoryReset(void)
{
  _s->write(comStop);
  delay(DELAY);

  _s->write(comFactoryReset);
  delay(DELAY);

  _s->write(comSaveCfg);
  delay(DELAY);

  _s->write(comStart);
  delay(DELAY);
}
