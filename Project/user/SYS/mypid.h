#ifndef FLOAT_PID_H
#define FLOAT_PID_H

#include "sys_bsp.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // PID参数
    float kp;            // 比例增益
    float ki;            // 积分增益
    float kd;            // 微分增益
    
    // 积分限幅
    float integral_max;  // 积分项上限
    float integral_min;  // 积分项下限
    
    // 输出限幅
    float output_max;    // 输出上限
    float output_min;    // 输出下限
    
    // 微分滤波
    float filter_alpha;  // 低通滤波系数(0.0~1.0)
    
    // 内部状态
    float error[3];      // 误差历史：[0]=当前，[1]=上一次，[2]=上上次
    float integral;      // 积分项
    float derivative;    // 微分项（滤波后）
    float output;        // 输出值
    
    // 辅助标志
    bool initialized;    // 初始化标志
    bool anti_windup;    // 抗积分饱和使能
} FloatPID;

extern FloatPID pid_CC_Buck;
extern FloatPID pid_CV_Buck;
extern FloatPID pid_CV_Boost;
extern FloatPID pid_CC_Boost;
extern FloatPID pid_iL;

void my_Pid_Init(void);//主要调用的函数

void PID_Param_Reset(void);
// 初始化PID控制器
void PID_Init(FloatPID* pid, 
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max,
              float filter_alpha);

// 更新PID控制器（返回当前输出值）
float PID_Update(FloatPID* pid, float setpoint, float measurement, float dt);

// 重置PID控制器状态
void PID_Reset(FloatPID* pid);

// 设置PID参数
void PID_SetTunings(FloatPID* pid, float kp, float ki, float kd);

// 设置输出限幅
void PID_SetOutputLimits(FloatPID* pid, float min, float max);

// 设置积分限幅
void PID_SetIntegralLimits(FloatPID* pid, float min, float max);

// 设置微分滤波系数
void PID_SetFilterAlpha(FloatPID* pid, float alpha);

// 启用/禁用抗积分饱和
void PID_SetAntiWindup(FloatPID* pid, bool enabled);

#endif // FLOAT_PID_H