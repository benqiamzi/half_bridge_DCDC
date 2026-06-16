#include "protection.h"

uint16_t vol_f32_to_u16(float value_f32)
{
	return (uint16_t)((value_f32) * my_ctrvalue.Gv);
}

uint16_t cur_x_f32_to_u16(float value_f32)
{
	return (uint16_t)((value_f32) * my_ctrvalue.Gi);
}

uint16_t cur_y_f32_to_u16(float value_f32)
{
	return (uint16_t)((value_f32) * my_ctrvalue.Gi);
}

protection_hadle_t protect_handle = {
	.Vout_ovp_flag = 0,
	.Vin_ovp_flag = 0,
	.Vin_uvp_flag = 0,
	.ocp_flag = 0,
	.short_flag = 0
};

void protect_handle_reset(void)
{
	// 根据不同的模式设置不同的保护阈值
	if(ctrState.ctr_mode == CTR_BUCK)
	{
		//转成数字量
		protect_handle.Vout_ovp_value = vol_f32_to_u16(BUCK_VOL_OUT_MAX);
		protect_handle.vout_ocp_value = cur_y_f32_to_u16(BUCK_CUR_OUT_MAX);
		protect_handle.Vin_ovp_value = vol_f32_to_u16(BUCK_VOL_IN_MAX);
		protect_handle.Vin_uvp_value = vol_f32_to_u16(BUCK_VOL_IN_MIN);
		protect_handle.short_i_value = cur_y_f32_to_u16(CUR_SHORT_MIN);
	}
	else if(ctrState.ctr_mode == CTR_BOOST)
	{
		protect_handle.Vout_ovp_value = vol_f32_to_u16(BOOST_VOL_OUT_MAX);
		protect_handle.vout_ocp_value = cur_x_f32_to_u16(BOOST_CUR_OUT_MAX);
		protect_handle.Vin_ovp_value = vol_f32_to_u16(BOOST_VOL_IN_MAX);
		protect_handle.Vin_uvp_value = vol_f32_to_u16(BOOST_VOL_IN_MIN);
		protect_handle.short_i_value = cur_x_f32_to_u16(CUR_SHORT_MIN);
	}
	else
	{

	}
	protect_handle.short_v_value = vol_f32_to_u16(VOL_SHORT_MIN);

//     _Bool Vout_ovp_flag; // 输出过压保护标志位
    // _Bool Vin_ovp_flag;  // 输入过压保护标志位
    // _Bool Vin_uvp_flag;  // 输入欠压保护标志位
    // _Bool ocp_flag;      // 过流保护标志位
    // _Bool short_flag;    // 短路保护标志位

	protect_handle.ocp_flag = 0;
	protect_handle.short_flag = 0;
	protect_handle.Vin_ovp_flag = 0;
	protect_handle.Vin_uvp_flag = 0;
	protect_handle.Vout_ovp_flag = 0;
}

typedef enum
{
	PROTECT_RELEASE = 0,
	PROTECT_HOLD,
	
}PROTECT_STATE;


CCMRAM uint8_t VoutSwOVP(protection_hadle_t *protect, uint8_t run_flag, uint16_t realValue)
{
	static  int32_t  Cnt=0;//过压判据保持时间计数器
	static  int32_t  preCnt=0;//过压判据保持时间计数器的上一次值
	static PROTECT_STATE protect_state = PROTECT_RELEASE;//保护状态机状态变量

	switch (protect_state)
	{

		case PROTECT_RELEASE:
		{
			if ((realValue > protect->Vout_ovp_value) && (run_flag == 1 ))//当输出电压大于*V；排除电源初始化时还没启动采样
			{
				protect_state = PROTECT_HOLD;
				Cnt = 0;
			}
			break;
		}
		case PROTECT_HOLD:
		{
			if ((realValue > protect->Vout_ovp_value) && (run_flag == 1 ))//当输出电压大于*V；排除电源初始化时还没启动采样
			{
				Cnt++;//过压条件保持计时
				preCnt = Cnt;
				if(Cnt > 50)//过压保持*秒，则认为过输出过压
				{
					Cnt = 0;//过压判据保持时间计数器清零
					
					protect_state = PROTECT_RELEASE;
					OLED_Clear();
					ctrState.pwm_output_flag = 0;
					ctr_pwm_stop();
					protect_handle.Vout_ovp_flag = 1;
					ctrState.SMFlag = Err;
					ctrState.run_flag = RUN_MODE_STOP;

					return 2;
				}
			}
			else//过压条件不满足，
			{
				Cnt  = 0;//计数器一直复位清0
				protect_state = PROTECT_RELEASE;
				return 1;
			}
			break;
		}
	}
	return 0;
}
CCMRAM uint8_t LoppSwShort(protection_hadle_t *protect, uint8_t run_flag, uint16_t voltValue, uint16_t currValue)
{
	static  int32_t  Cnt=0;//过压判据保持时间计数器
	static  int32_t  preCnt=0;//过压判据保持时间计数器的上一次值
	static PROTECT_STATE protect_state = PROTECT_RELEASE;//保护状态机状态变量

	switch (protect_state)
	{

		case PROTECT_RELEASE:
		{
			if ((voltValue < protect->short_v_value) &&(currValue > protect->short_i_value) && (run_flag == 1 ))//当输出电压大于*V；排除电源初始化时还没启动采样
			{
				protect_state = PROTECT_HOLD;
				Cnt = 0;
			}
			break;
		}
		case PROTECT_HOLD:
		{
			if ((voltValue < protect->short_v_value) &&(currValue > protect->short_i_value) && (run_flag == 1 ))//当输出电压大于*V；排除电源初始化时还没启动采样
			{
				Cnt++;//过压条件保持计时
				preCnt = Cnt;
				if(Cnt > 10)//过压保持*秒，则认为过输出过压
				{
					Cnt = 0;//过压判据保持时间计数器清零
					
					protect_state = PROTECT_RELEASE;
					OLED_Clear();
					ctrState.pwm_output_flag = 0;
					ctr_pwm_stop();
					protect_handle.short_flag = 1;
					ctrState.SMFlag = Err;

					return 2;
				}
			}
			else//过压条件不满足，
			{
				Cnt  = 0;//计数器一直复位清0
				protect_state = PROTECT_RELEASE;
				return 1;
			}
			break;
		}
	}
	return 0;
}