#ifndef __FUNCTION_H
#define __FUNCTION_H

#define CCMRAM  __attribute__((section("RAMCODE")))

#include "sys_bsp.h"


typedef enum
{
	Init=0,
	Wait,
	Rise,
	Run,
	Err
}STATE_M;

typedef enum
{
	UI_PARAM= 0,
	UI_SET,
	
}UI_M;

typedef enum
{
	CTR_BUCK = 0,
	CTR_BOOST,
	CTR_OPEN_LOOP
}CTR_MODE;

typedef enum
{
	OUT_CV = 0,
	OUT_CC,
}OUT_MODE;

typedef enum
{
	SSInit = 0,
	SSRun
}SState_M;

typedef enum
{
	RUN_MODE_STOP = 0,
	RUN_MODE_RUN
}RUN_STATE;


typedef struct
{
	_Bool openloop_flag; // 开环标志位 0 关闭 1开启
	RUN_STATE run_flag;    // 运行标志位 0 关闭 1开启
	_Bool pwm_output_flag; // PWM输出标志位 0 关闭 1开启
	CTR_MODE ctr_mode; // 运行状态标志位 0 buck模式 1 boost模式
	OUT_MODE out_mode; // 输出模式标志位 0 cv 1 cc

	UI_M ui;
	STATE_M SMFlag; // 状态机标志位
	SState_M STState; // 软启动阶段的标志位

}CtrState_t;

extern CtrState_t ctrState;

void my_sys_init(void);
void task_list(void);
void PWM_Rise(void);

#endif
