#ifndef FLOAT_PID_H
#define FLOAT_PID_H

#include "sys_bsp.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float kp;            // 比例增益
    float ki;            // 积分增益
    float kd;            // 微分增益

    float integral_max;  // 积分项上限
    float integral_min;  // 积分项下限

    float output_max;    // 输出上限
    float output_min;    // 输出下限

    float filter_alpha;  // 微分低通滤波系数(0.0~1.0)

    float error[3];      // 误差历史：[0]=当前，[1]=上一次，[2]=上上次
    float integral;      // 积分项
    float derivative;    // 微分项（滤波后）
    float output;        // 输出值

    bool initialized;
    bool anti_windup;    // 抗积分饱和使能
} FloatPID;

extern FloatPID pid_CC_Buck;
extern FloatPID pid_CV_Buck;
extern FloatPID pid_CV_Buck_With_iL;
extern FloatPID pid_CV_Boost;
extern FloatPID pid_CC_Boost;
extern FloatPID pid_Boost_iL;
extern FloatPID pid_Buck_iL;

void my_Pid_Init(void);
void PID_Param_Reset(void);

void PID_Init(FloatPID* pid,
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max,
              float filter_alpha);

float PID_Update(FloatPID* pid, float setpoint, float measurement, float dt);
void  PID_Reset(FloatPID* pid);

void PID_SetTunings(FloatPID* pid, float kp, float ki, float kd);
void PID_SetOutputLimits(FloatPID* pid, float min, float max);
void PID_SetIntegralLimits(FloatPID* pid, float min, float max);
void PID_SetFilterAlpha(FloatPID* pid, float alpha);
void PID_SetAntiWindup(FloatPID* pid, bool enabled);

#endif
