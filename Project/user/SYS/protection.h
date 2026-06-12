#ifndef __PROTECTION_H
#define __PROTECTION_H

#include "sys_bsp.h"
#include "function.h"

typedef struct 
{
    uint16_t Vout_ovp_value; // 输出过压保护阈值
    uint16_t Vin_ovp_value;  // 输入过压保护阈值
    uint16_t Vin_uvp_value;  // 输入欠压保护阈值
    uint16_t vout_ocp_value; // 输出过流保护阈值
    uint16_t ocp_value;      // 过流保护阈值
    uint16_t short_i_value;  // 短路保护电流阈值
    uint16_t short_v_value;  // 短路保护电压阈值

    _Bool Vout_ovp_flag; // 输出过压保护标志位
    _Bool Vin_ovp_flag;  // 输入过压保护标志位
    _Bool Vin_uvp_flag;  // 输入欠压保护标志位
    _Bool ocp_flag;      // 过流保护标志位
    _Bool short_flag;    // 短路保护标志位

}protection_hadle_t;

extern protection_hadle_t protect_handle;

#define VOL_SHORT_MIN 4.0f
#define CUR_SHORT_MIN 2.0f


#define BUCK_VOL_OUT_MAX	15.0f
#define BUCK_CUR_OUT_MAX	5.0f
#define BUCK_VOL_IN_MAX	    26.0f
#define BUCK_VOL_IN_MIN	    18.0f

#define BOOST_VOL_OUT_MAX	26.0f
#define BOOST_CUR_OUT_MAX	2.0f
#define BOOST_VOL_IN_MAX	26.0f
#define BOOST_VOL_IN_MIN	10.0f

void protect_handle_reset(void);
uint8_t VoutSwOVP(protection_hadle_t *protect, uint8_t run_flag, uint16_t realValue);
uint8_t LoppSwShort(protection_hadle_t *protect, uint8_t run_flag, uint16_t voltValue, uint16_t currValue);

#endif
