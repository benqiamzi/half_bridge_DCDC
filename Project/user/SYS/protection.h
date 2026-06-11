#ifndef __PROTECTION_H
#define __PROTECTION_H

#include "function.h"

#define MAX_SHORT_I	2.0
#define MIN_SHORT_V	5.0

#define MAX_OUT_V 36
#define MAX_OUT_I 2.5

//输出过压保护
uint8_t VoutSwOVP(uint8_t run_flag,int32_t ovpValue, float realValue);

uint8_t VinSwOVP(uint8_t run_flag,int32_t ovpValue, int32_t vinValue);

uint8_t VinSwUVP(uint8_t run_flag,int32_t uvpValue, int32_t vinValue);

uint8_t SwOCP(uint8_t run_flag,int32_t ocpValue, int32_t outValue);

//短路保护
uint8_t ShortOff(uint8_t run_flag,int32_t vValue,int32_t iValue, float realVoltValue,float realCurrValue);

#endif
