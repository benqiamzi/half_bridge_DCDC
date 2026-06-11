#ifndef __CTLLOOP_H
#define __CTLLOOP_H	 

#include "sys_bsp.h"

#define PERIOD 3000

#define MAX_DUTY 2700
#define MIN_DUTY 100


typedef struct
{
	uint16_t Vyset_q15;	// 电压参考数字量
	uint16_t Iyset_q15;	// 电压参考数字量
	uint16_t Vxset_q15;	// 电压参考数字量
	uint16_t Ixset_q15;	// 电压参考数字量

	
	float Vyset_f32;	// 电压参考有效值
	float Iyset_f32;	// 电压参考有效值
	float Vxset_f32;	// 电压参考有效值
	float Ixset_f32;	// 电压参考有效值

	uint16_t offset;

	q7_t Ri_sample;
	float Gv;	// 电压环增益
	float Gi;	 // 电流环增益
	
	float Gv_re;	// 1/电压环增益
	float Gi_re;	 // 1/电流环增益
	
	float k_iy_a;
	float k_iy_b;
	float k_ix_a;
	float k_ix_b;
	float k_vx_a;
	float k_vx_b;
	float k_vy_a;
	float k_vy_b;

}CtlValue_t;

typedef struct
{	
	uint16_t 		Q1MaxDuty;//Buck最大占空比
	uint16_t		Q2MaxDuty;//Boost最大占空比
	uint16_t		Q1Duty;//上管MOS的占空比		
	uint16_t		Q2Duty;//下管MOS的占空比

}PWM_VALUE_t;



extern CtlValue_t my_ctrvalue;
extern PWM_VALUE_t pwm_value;

void duty_change(uint16_t duty,uint8_t ch);
void ctr_pwm_ch_start(void);
void ctr_pwm_chn_start(void);
void ctr_pwm_start(void);
void ctr_pwm_stop(void);

void loop_set_vol_x(float vol);
void loop_set_vol_y(float vol);
void loop_set_cur_x(float cur);
void loop_set_cur_y(float cur);

void BuckOpenLoopTest(void);
void LoopCtl(void);
void CtlValue_Init(void);



#endif
