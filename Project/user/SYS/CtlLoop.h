#ifndef __CTLLOOP_H
#define __CTLLOOP_H	 

#include "sys_bsp.h"

#define PERIOD 3000

#define MAX_DUTY 2700
#define BOOST_MAX_DUTY 2500
#define MIN_DUTY 100


typedef enum
{
	CTR_PID = 0,
	CTR_PID_WITH_iL,
}CTR_ALGO;

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

	float iL_min_threshold;
	float iL_max_threshold;
	CTR_ALGO ctr_algo; 
}CtlValue_t;

typedef struct
{	
	uint16_t 		Q1MaxDuty;//Buck最大占空比
	uint16_t		Q2MaxDuty;//Boost最大占空比
	uint16_t		Q1Duty;//上管MOS的占空比		
	uint16_t		Q2Duty;//下管MOS的占空比

}PWM_VALUE_t;

extern CtlValue_t ctr_value;
extern PWM_VALUE_t pwm_value;

void duty_change(uint16_t duty,uint8_t ch);
void ctr_pwm_ch(_Bool state);
void ctr_pwm_chn(_Bool state);
void ctr_pwm_start(void);
void ctr_pwm_stop(void);

void loop_set_vol_x(float vol);
void loop_set_vol_y(float vol);
void loop_set_cur_x(float cur);
void loop_set_cur_y(float cur);

void auto_get_ctr_mode(void);
void LoopCtl(void);
void CtlValue_Init(void);



#endif
