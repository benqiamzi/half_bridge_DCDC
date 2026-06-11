#ifndef __SYS_BSP_H
#define __SYS_BSP_H

//#include "sys_bsp.h"

#define V_MAX 40.0f 

#define CCMRAM  __attribute__((section("RAMCODE")))

#ifndef q15_t
#define q7_t int8_t
#define q15_t int16_t
#define q31_t int32_t

#define float32_t float
#endif 

#include <stdio.h>
#include <stdint.h>
#include "gd32e10x.h"
#include "systick.h"
#include "gd32e10x_eval.h"

#include "function.h"
//#include "ringBuf.h"
#include "adc_dc_sample.h"
#include <stdio.h>
#include "mypid.h"
#include "ctlloop.h"
#include "jkd_oled.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"
#include <string.h>
#include "function.h"





#endif