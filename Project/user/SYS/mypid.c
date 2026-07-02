#include "mypid.h"
#include <math.h>

FloatPID pid_CV_Buck;
FloatPID pid_CV_Buck_With_iL;
FloatPID pid_CC_Buck;

FloatPID pid_CV_Boost;
FloatPID pid_CC_Boost;

FloatPID pid_Boost_iL;
FloatPID pid_Buck_iL;

void my_Pid_Init(void)
{
	PID_Init(&pid_CV_Buck_With_iL, 3, 20, 0.0002, 0, 6, 0, 10, 0.98f);
    PID_Init(&pid_CV_Buck, 10, 50, 0.0002, 0, MAX_DUTY-200, 0, MAX_DUTY, 0.98f);

    PID_Init(&pid_CC_Buck, 10, 50, 0.002, 0, 6.0, 0, 6.0, 0.95f);
    PID_Init(&pid_Buck_iL, 20, 100, 0.0002, 0, MAX_DUTY-200, MIN_DUTY, MAX_DUTY, 0.98f);


   
    PID_Init(&pid_CC_Boost, 3, 25, 0.002, MIN_DUTY, MAX_DUTY, 0, MAX_DUTY, 0.95f);

    PID_Init(&pid_CV_Boost, 2, 10, 0.0002, 0, 6, 0, 10, 0.99f);
    PID_Init(&pid_Boost_iL, 10, 70, 0.0002, 0, MAX_DUTY, 0, MAX_DUTY, 0.99f);
}

void PID_Param_Reset(void)
{
    PID_Reset(&pid_CC_Buck);
    PID_Reset(&pid_CV_Buck);
    PID_Reset(&pid_CC_Boost);
    PID_Reset(&pid_CV_Boost);
    PID_Reset(&pid_Buck_iL);
    PID_Reset(&pid_Boost_iL);
}

void PID_Init(FloatPID* pid,
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max,
              float filter_alpha) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;

    pid->filter_alpha = filter_alpha;

    PID_Reset(pid);
    pid->anti_windup = true;
}

CCMRAM float PID_Update(FloatPID* pid, float setpoint, float measurement, float dt) {
    float error = setpoint - measurement;

    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = error;

    float integral_temp = pid->integral + (error * pid->ki * dt);

    if (pid->anti_windup) {
        if (integral_temp > pid->integral_max)
            integral_temp = pid->integral_max;
        else if (integral_temp < pid->integral_min)
            integral_temp = pid->integral_min;
    }

    float derivative_raw = (error - pid->error[1]) / dt;
    pid->derivative = pid->filter_alpha * pid->derivative +
                     (1.0f - pid->filter_alpha) * derivative_raw;

    float output = pid->kp * error + integral_temp + pid->kd * pid->derivative;

    if (output > pid->output_max) {
        output = pid->output_max;
        if (pid->anti_windup && error > 0)
            integral_temp = pid->integral;
    } else if (output < pid->output_min) {
        output = pid->output_min;
        if (pid->anti_windup && error < 0)
            integral_temp = pid->integral;
    }

    pid->integral = integral_temp;
    pid->output = output;
    return output;
}

CCMRAM void PID_Reset(FloatPID* pid) {
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
    pid->initialized = true;
}

void PID_SetTunings(FloatPID* pid, float kp, float ki, float kd) {
    pid->kp = kp; pid->ki = ki; pid->kd = kd;
}

void PID_SetOutputLimits(FloatPID* pid, float min, float max) {
    pid->output_min = min;
    pid->output_max = max;
    if (pid->output > max) pid->output = max;
    else if (pid->output < min) pid->output = min;
}

void PID_SetIntegralLimits(FloatPID* pid, float min, float max) {
    pid->integral_min = min;
    pid->integral_max = max;
    if (pid->integral > max) pid->integral = max;
    else if (pid->integral < min) pid->integral = min;
}

void PID_SetFilterAlpha(FloatPID* pid, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    pid->filter_alpha = alpha;
}

void PID_SetAntiWindup(FloatPID* pid, bool enabled) {
    pid->anti_windup = enabled;
}
