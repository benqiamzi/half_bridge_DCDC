//该头文件包含了对adc数据进行采集和处理的相关函数：
#ifndef __ADC_DC_SAMPLE_H
#define __ADC_DC_SAMPLE_H

#include "sys_bsp.h"

#ifndef CCMRAM
#define CCMRAM  __attribute__((section("ccmram")))
#endif

typedef struct
{
    float Vx_f32;
	float Vy_f32;
	float Ix_f32;
	float Iy_f32;
	float iL_f32;
	
    int16_t Vx_q15;     // X侧端电压
    int16_t Ix_q15;     // X侧端电流
    int16_t Vy_q15;     // Y侧端电压
    int16_t Iy_q15;     // Y侧端电流
	int16_t iL_q15;     // 电感电流

    uint16_t Vx_raw;     // X侧端电压原始值
    uint16_t Ix_raw;     // X侧端电流原始值
    uint16_t Vy_raw;     // Y侧端电压原始值
    uint16_t Iy_raw;     // Y侧端电流原始值
    uint16_t iL_raw;     // Y侧端电流原始值

    float k_vx_a;
    float k_vx_b;
    float k_vy_a;
    float k_vy_b;
    float k_ix_a;
    float k_ix_b;
    float k_iy_a;
    float k_iy_b;


} ADC_SAMPLE_t;


typedef struct
{
    _Bool Scpflag;
	_Bool Ovpflag;
	
	

} ADC_PROTECT_t;



extern ADC_SAMPLE_t my_adc_sample;
extern uint16_t open_value;

//运放增益
#define Kx_I 20.0f
#define Kx_V 17.5f
#define Ky_I 20.0f
#define Ky_V 17.5f
#define Rs 0.01f // 电流采样电阻

#ifndef ADC_TRANFER_FUCTION
#define VsetToVref(x) ((uint16_t)((x) / Kx_V / 3.3f*4096.0f)) // 电压设置值转化为ADC参考值
#define IsetToIref(x) ((uint16_t)((x) / Kx_I*Rs / 3.3f*4096.0f+2048)) // 电流设置值转化为ADC参考值

#endif

void adc_sample_start(void);
void adc_filter(void);
void adc_disp(void);
void adc_AutoGetOffset(void);

#endif
