#include "mypid.h"
#include <math.h>

FloatPID pid_CV_Buck;
FloatPID pid_CC_Buck;
FloatPID pid_CV_Boost;
FloatPID pid_CC_Boost;
FloatPID pid_iL;

void my_Pid_Init(void)
{
	PID_Init(&pid_CC_Buck, 10.0f, 400.0f, 0.001f, MIN_DUTY, MAX_DUTY, 0, MAX_DUTY, 0.95f);
	PID_Init(&pid_CV_Buck, 5.0f, 60.0f, 0.001f, 0.0f, 6.0f, 0.0f, 6.0f, 0.98f);

	PID_Init(&pid_CC_Boost, 10.0f, 400.0f, 0.001f, MIN_DUTY, MAX_DUTY, 0, MAX_DUTY, 0.95f);
	PID_Init(&pid_CV_Boost, 1.0f, 15.0f, 0.001f, 0.0f, 6.0f, 0.0f, 6.0f, 0.98f);
    // PID_Init(&pid_CV_Boost, 4.0f, 50.0f, 0.001f,MIN_DUTY, MAX_DUTY,MIN_DUTY, MAX_DUTY,0.98f);
    PID_Init(&pid_iL, 10, 200, 0.001f, MIN_DUTY, MAX_DUTY, 0, MAX_DUTY, 0.98f);
}

void PID_Param_Reset(void)
{
    PID_Reset(&pid_CC_Buck);
    PID_Reset(&pid_CV_Buck);
    PID_Reset(&pid_CC_Boost);
    PID_Reset(&pid_CV_Boost);
    PID_Reset(&pid_iL);

}


// 初始化PID控制器
void PID_Init(FloatPID* pid,
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max,
              float filter_alpha) {
    // 设置参数
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    // 设置限幅
    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;

    // 设置滤波系数
    pid->filter_alpha = filter_alpha;

    // 初始化状态
    PID_Reset(pid);

    // 启用抗积分饱和
    pid->anti_windup = true;
}

// 更新PID控制器
CCMRAM float PID_Update(FloatPID* pid, float setpoint, float measurement, float dt) {
    // 计算当前误差
    float error = setpoint - measurement;

    // 保存误差历史
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = error;

    // 计算积分项（带抗饱和）
    float integral_temp = pid->integral + (error * pid->ki * dt);

    // 积分限幅（抗积分饱和）
    if (pid->anti_windup) {
        if (integral_temp > pid->integral_max) {
            integral_temp = pid->integral_max;
        } else if (integral_temp < pid->integral_min) {
            integral_temp = pid->integral_min;
        }
    }

    // 计算微分项（带低通滤波）
    float derivative_raw = (error - pid->error[1]) / dt;
    pid->derivative = pid->filter_alpha * pid->derivative +
                     (1.0f - pid->filter_alpha) * derivative_raw;

    // 计算PID输出
    float p_term = pid->kp * error;
    float i_term = integral_temp;
    float d_term = pid->kd * pid->derivative;

    float output = p_term + i_term + d_term;

    // 输出限幅
    if (output > pid->output_max) {
        output = pid->output_max;
        // 若启用抗积分饱和，当输出达到上限且误差为正时，冻结积分
        if (pid->anti_windup && error > 0) {
            integral_temp = pid->integral;
        }
    } else if (output < pid->output_min) {
        output = pid->output_min;
        // 若启用抗积分饱和，当输出达到下限时且误差为负时，冻结积分
        if (pid->anti_windup && error < 0) {
            integral_temp = pid->integral;
        }
    }

    // 更新积分项
    pid->integral = integral_temp;
    pid->output = output;

    return output;
}

// 重置PID控制器状态
CCMRAM void PID_Reset(FloatPID* pid) {
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
    pid->initialized = true;
}

// 设置PID参数
void PID_SetTunings(FloatPID* pid, float kp, float ki, float kd) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

// 设置输出限幅
void PID_SetOutputLimits(FloatPID* pid, float min, float max) {
    pid->output_min = min;
    pid->output_max = max;

    // 确保当前输出在新的限幅范围内
    if (pid->output > max) {
        pid->output = max;
    } else if (pid->output < min) {
        pid->output = min;
    }
}

// 设置积分限幅
void PID_SetIntegralLimits(FloatPID* pid, float min, float max) {
    pid->integral_min = min;
    pid->integral_max = max;

    // 确保当前积分在新的限幅范围内
    if (pid->integral > max) {
        pid->integral = max;
    } else if (pid->integral < min) {
        pid->integral = min;
    }
}

// 设置微分滤波系数
void PID_SetFilterAlpha(FloatPID* pid, float alpha) {
    // 确保alpha在有效范围内
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    pid->filter_alpha = alpha;
}

// 启用/禁用抗积分饱和
void PID_SetAntiWindup(FloatPID* pid, bool enabled) {
    pid->anti_windup = enabled;
}
