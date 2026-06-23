#include "function.h"
#include "CtlLoop.h"
#include "protection.h"

#define _DEBUG 


char tx_buf[50] = {0};

uint16_t curr_cnt = 0;
uint16_t volt_cnt = 0;

uint8_t param_set_idx = 0;
const float param_set_step[] = {0.1f, 0.2f, 0.5f, 1.0f, 2.0f};
const float vol_range_x[] = {15.0f, 27.0f};
const float vol_range_y[] = {5.0f, 16.0f};
const float cur_range_x[] = {0.0f, 2.5f};
const float cur_range_y[] = {0.0f, 5.0f};

void oled_show(void);
void key_Proc(void);
void led_Proc(void);
void fsm_Proc(void);
void encoder_Proc(void);

CtrState_t ctrState = {
	.ui = UI_PARAM,
	.ctr_mode = CTR_BUCK,
	.out_mode = OUT_CV,
	.SMFlag = Init,
	.STState = SSInit,
	.run_flag = RUN_MODE_STOP,
	.pwm_output_flag = 0,
	.openloop_flag = 0
};


extern usb_core_driver cdc_acm;
void my_sys_init(void)
{
	encoder_init();
	ctr_pwm_ch(0);
	ctr_pwm_chn(0);
	ctr_pwm_stop();
	my_Pid_Init();
	CtlValue_Init();
	usb_rcu_config();

    usb_timer_init();
	
	
    usb_intr_config();

	usbd_init(&cdc_acm, &cdc_desc, &cdc_class);

	OLED_Init();
}

void TIMER5_IRQHandler(void)
{
	// gpio_bit_set(GPIOB,GPIO_PIN_11|GPIO_PIN_10);
		static uint8_t cnt = 0;
		cnt++;
		if(cnt == 10)
		{
			cnt = 0;
			fsm_Proc();	
			auto_get_ctr_mode();
		}
	
	send_param_data();
	// gpio_bit_reset(GPIOB,GPIO_PIN_11|GPIO_PIN_10);
	timer_interrupt_flag_clear(TIMER5, TIMER_INT_UP);
}

uint32_t fsm_tick = 0;

void fsm_Proc(void)
{
	// if(uwTick - fsm_tick < 3)
	// 	return;
	
	// fsm_tick = uwTick;
	
	switch (ctrState.SMFlag)
	{	
		case Init:
			/* 完成初始化后进入等待态 */
			ctrState.SMFlag = Wait;

			adc_filter();
			break;

		case Wait:
			adc_filter();
			/* 停止时保持最小占空比，等待启动命令 */
			if(ctrState.run_flag == RUN_MODE_STOP)
			{
				protect_handle_reset();
				ctr_pwm_stop();
				PID_Param_Reset();
				pwm_value.Q1MaxDuty = MIN_DUTY;
				pwm_value.Q2MaxDuty = MIN_DUTY;
				ctrState.pwm_output_flag = 0;
				ctrState.STState = SSInit;
			}
			else if(ctrState.run_flag == RUN_MODE_RUN)
			{
				ctrState.STState = SSInit;
				ctrState.SMFlag = Rise;
			}
			break;

		case Rise:
			adc_filter();
			PWM_Rise();
			break;
		case Run:
			adc_filter();
			/* 正常运行态：检查保护并在需要时跳转到错误或等待态 */
			break;

		case Err:
			ctrState.ui = UI_ERR;
			ctr_pwm_stop();
			break;

		default:
			break;
	}

	
}

CCMRAM void PWM_Rise(void)
{
	const static uint8_t step = 30;
	const static uint8_t boost_step = 5;
	static  uint16_t	Q1Cnt=0,Q2Cnt=0;//5mS计数器，PWM软起的计时器

	switch(ctrState.STState)
	{
		case SSInit:
		{
			ctr_pwm_stop();
			pwm_value.Q1Duty = MIN_DUTY;
			pwm_value.Q2Duty = MIN_DUTY;
			pwm_value.Q1MaxDuty = MIN_DUTY;
			pwm_value.Q2MaxDuty = MIN_DUTY;
			gpio_bit_reset(GPIOB,GPIO_PIN_11);

			// ctrState.pwm_output_flag = 1;
			Q1Cnt=0;
			Q2Cnt=0;
			ctrState.STState = SSRun;

			
			break;
		}
		case SSRun:
		{
			
			if(ctrState.ctr_mode == CTR_BUCK)//工作于BUCK模式，上管Q2先软起，Q2软起后同步管Q2再软起
			{
				if(ctrState.pwm_output_flag == 0)
				{
					PID_Param_Reset();
					ctr_pwm_ch(1);
					
				}
				ctrState.pwm_output_flag = 1;
				if(pwm_value.Q1MaxDuty < MAX_DUTY)
				{
					Q1Cnt++;//计数器正式开始计数，5mS加1
					pwm_value.Q1MaxDuty = MIN_DUTY + Q1Cnt*step;//Q1管占空比幅值增加
					
					if(pwm_value.Q1MaxDuty >= MAX_DUTY )//累加到最大值
					{
						pwm_value.Q1MaxDuty = MAX_DUTY;
						ctr_pwm_chn(1);
					}
					
				}
				else
				{
					Q2Cnt++;//计数器正式开始计数，5mS加1	
					pwm_value.Q2MaxDuty= Q2Cnt*step;//Q2占空比限制从最小开始		
					if(pwm_value.Q2MaxDuty > MAX_DUTY)//累加到最大值
						pwm_value.Q2MaxDuty  = MAX_DUTY ;	
				}		
				if(pwm_value.Q1MaxDuty==MAX_DUTY&&pwm_value.Q2MaxDuty==MAX_DUTY)//当Q1和Q2的最大占空比限制达到最大，则认为软启结束			
				{
					ctrState.SMFlag = Run;
					ctrState.STState = SSInit;
					Q1Cnt=0;
					Q2Cnt=0;//主状态机跳转至正常运行运行状态
				}
			
								
			}
			else if(ctrState.ctr_mode == CTR_BOOST)//工作BOOST模式，下管Q2先软起，Q2软起后同步管Q1再软起
			{
				if(ctrState.pwm_output_flag == 0)
				{
					PID_Param_Reset();
					ctr_pwm_chn(1);
					ctr_pwm_ch(1);
					
				}
				ctrState.pwm_output_flag = 1;
				if(pwm_value.Q2MaxDuty < BOOST_MAX_DUTY)
				{
					Q2Cnt++;//计数器正式开始计数，5mS加1	
					pwm_value.Q2MaxDuty= MIN_DUTY+Q2Cnt*boost_step;//Q2管占空比幅值增加
					if(pwm_value.Q2MaxDuty > BOOST_MAX_DUTY)//累加到最大值
					{
						// ctr_pwm_ch(1);
						pwm_value.Q2MaxDuty  = BOOST_MAX_DUTY ;//下管Q2软起结束后，同步管Q1正式发波
						pwm_value.Q1MaxDuty  = BOOST_MAX_DUTY ;
					}

				}
				// else
				// {
				// 	Q1Cnt++;//计数器正式开始计数，5mS加1	
				// 	pwm_value.Q1MaxDuty= Q1Cnt*boost_step;//Q2占空比限制从最小开始		
				// 	if(pwm_value.Q1MaxDuty > BOOST_MAX_DUTY)//累加到最大值
				// 		pwm_value.Q1MaxDuty  = BOOST_MAX_DUTY;	
				// }			
				if(pwm_value.Q1MaxDuty==BOOST_MAX_DUTY&&pwm_value.Q2MaxDuty==BOOST_MAX_DUTY)//当Q1和Q2的最大占空比限制达到最大，则认为软启结束			
				{
					ctrState.SMFlag = Run;
					ctrState.STState = SSInit;
					Q1Cnt=0;
					Q2Cnt=0;//主状态机跳转至正常运行运行状态
				}
			}
			
			break;
		}
		default:
			break;
	}

}

void task_list(void)
{
	key_Proc();
	oled_show();
	led_Proc();
	
}

void led_spark(uint32_t led_pin)
{
	uint8_t state = gpio_output_bit_get(LED_PORT, led_pin);
	gpio_bit_write(LED_PORT,led_pin, (bit_status)(!state));
}

void protect_led_reset(void)
{
	gpio_bit_set(LED_PORT, OCP_LED_PIN | OVP_LED_PIN | SCP_LED_PIN);   // 初始熄灭
}

uint32_t led_tick = 0;

void led_Proc(void)
{
	if(uwTick - led_tick < 100)
		return;
	
	led_tick = uwTick;
	if(protect_handle.ocp_flag == 1)
	{
		led_spark(OCP_LED_PIN);
	}
	else if(protect_handle.short_flag == 1)
	{
		led_spark(SCP_LED_PIN);
	}
	else if(protect_handle.Vout_ovp_flag == 1||protect_handle.Vin_ovp_flag == 1)
	{
		led_spark(OVP_LED_PIN);
	}
	else
	{
		protect_led_reset();
	}

}

uint32_t oled_tick = 0;

CCMRAM void oled_show(void)
{
	if(uwTick - oled_tick < 200)
		return;
	
	oled_tick = uwTick;
	adc_disp();

	sprintf(tx_buf, "cnt = %d\n", encoder_get_count());
	cdc_send_data(tx_buf, strlen(tx_buf));

	switch(ctrState.ui)
	{
		case UI_PARAM:	
		{
			OLED_Printf(0,Line0, OLED_8X16, "Vx:%0.3fV ", my_adc_sample.Vx_f32);
			if(ctrState.run_flag == RUN_MODE_RUN)
			{
				OLED_ShowString(10*8,Line0," RUN ",OLED_8X16);
			}
			else
			{
				OLED_ShowString(10*8,Line0," STOP ",OLED_8X16);
			}

			OLED_Printf(0,Line1, OLED_8X16, "Ix:%0.3fA  %d  ", my_adc_sample.Ix_f32, \
				ctrState.ctr_mode == CTR_BUCK ? pwm_value.Q1Duty : pwm_value.Q2Duty);
			if(ctrState.ctr_mode == CTR_BUCK)
			{
				OLED_ShowString(11*8,Line2,"BUCK ",OLED_8X16);
			}
			else if(ctrState.ctr_mode == CTR_BOOST)
			{
				OLED_ShowString(11*8,Line2,"BOOST ",OLED_8X16);
			}
			else if(ctrState.ctr_mode == CTR_OPEN_LOOP)
			{
				OLED_ShowString(11*8,Line2,"OPEN ",OLED_8X16);
			}

			if(ctrState.out_mode ==OUT_CV)
			{
				OLED_ShowString(10*8,Line3," CV ",OLED_8X16);
			}
			else
			{
				OLED_ShowString(10*8,Line3," CC ",OLED_8X16);
			}
			
			OLED_Printf(0,Line2, OLED_8X16, "Vy:%0.3fV ", my_adc_sample.Vy_f32);
			OLED_Printf(0,Line3, OLED_8X16, "Iy:%0.3fA ", my_adc_sample.Iy_f32);
			break;
		}
		case UI_SET:
		{
			const uint8_t out_mode_num = 4, run_flag_num = 8;
			if(ctrState.ctr_mode == CTR_BUCK)
			{
				OLED_Printf(0,Line0, OLED_8X16, "Buck ");
				OLED_Printf(0,Line1, OLED_8X16, "Vy:%0.3fV    ", my_adc_sample.Vy_f32);
				OLED_Printf(0,Line2, OLED_8X16, "Iy:%0.3fA   ", my_adc_sample.Iy_f32);
				if(ctrState.out_mode == OUT_CV)
				{
					OLED_ShowString(out_mode_num*8,Line0," CV ",OLED_8X16);
					OLED_Printf(0,Line3, OLED_8X16, "Vset:%0.1fV  ", ctr_value.Vyset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					OLED_ShowString(out_mode_num*8,Line0," CC ",OLED_8X16);
					OLED_Printf(0,Line3, OLED_8X16, "Iset:%0.1fA  ", ctr_value.Iyset_f32);
				}
			}
			else if(ctrState.ctr_mode == CTR_BOOST)
			{
				OLED_Printf(0,Line0, OLED_8X16, "Boost ");
				OLED_Printf(0,Line1, OLED_8X16, "Vx:%0.3fV    ", my_adc_sample.Vx_f32);
				OLED_Printf(0,Line2, OLED_8X16, "Ix:%0.3fA   ", my_adc_sample.Ix_f32);
				if(ctrState.out_mode == OUT_CV)
				{
					OLED_ShowString(out_mode_num*8,Line0," CV ",OLED_8X16);
					OLED_Printf(0,Line3, OLED_8X16, "Vset:%0.1fV  ", ctr_value.Vxset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					OLED_ShowString(out_mode_num*8,Line0," CC ",OLED_8X16);
					OLED_Printf(0,Line3, OLED_8X16, "Iset:%0.1fA  ", ctr_value.Ixset_f32);
				}

			}
			if(ctrState.run_flag == RUN_MODE_RUN)
			{
				OLED_ShowString(run_flag_num*8,Line0,"R ",OLED_8X16);
			}
			else
			{
				OLED_ShowString(run_flag_num*8,Line0,"S",OLED_8X16);
			}
			OLED_ShowFloatNum(11*8,Line0,param_set_step[param_set_idx],1,1,OLED_8X16);
			
			break;
		}
		case UI_ERR:
			char err_str[16] = {0};  // 数组初始化，全部清零（杜绝随机脏数据）

			if(protect_handle.Vout_ovp_flag)
			{
				strcpy(err_str, "VOUT OVP ");
			}
			else if(protect_handle.ocp_flag)
			{
				strcpy(err_str, "VOUT OCP ");
			}
			else if(protect_handle.Vin_ovp_flag)
			{
				strcpy(err_str, "VIN OVP ");
			}
			else if(protect_handle.Vin_uvp_flag)
			{
				strcpy(err_str, "VIN UVP ");
			}
			else if(protect_handle.short_flag)
			{
				strcpy(err_str, "LP SHORT");
			}
			else
			{
				strcpy(err_str, "Normal  ");  // 无故障显示正常状态
			}
			OLED_Printf(0,Line1, OLED_8X16, " Err: %s ", err_str);
			OLED_Printf(0,Line2, OLED_8X16, "press K1	");
			
			break;
		default:
		{
			ctrState.ui = UI_PARAM;
			break;
		}
	}
	OLED_Update();
}


uint32_t key_tick = 0;
__IO uint8_t key_value = 0;
__IO uint8_t key_down = 0;
__IO uint8_t key_old = 0;
void key_read(void)
{
	key_value = 0;
	if (gpio_input_bit_get(KEY1_PORT, KEY1_PIN) == 0)
	{
		key_value = 1;
	}
	else if (gpio_input_bit_get(KEY2_PORT, KEY2_PIN) == 0)
	{
		key_value = 2;
	}
	if (gpio_input_bit_get(KEY3_PORT, KEY3_PIN) == 0)
	{
		key_value = 3;
	}
	else if (gpio_input_bit_get(KEY4_PORT, KEY4_PIN) == 0)
	{
		key_value = 4;
	}
	else if(gpio_input_bit_get(ENCODER_PORT, ENCODER_PIN) == 0)
	{
		key_value = 5;
	}
	
	key_down = key_value & (key_value ^ key_old);
	key_old = key_value;
}

uint8_t show_direct = 0;
 uint8_t key_Cnt = 0;
 _Bool long_press_flag = 0;
 uint16_t key_press_cnt = 0; // 按键按下标志位
 _Bool key_res_flag = 0;
void key_Proc(void)
{
	if(uwTick - key_tick < 35)
		return;
	
	key_tick = uwTick;
	
	key_read();

	if(key_down == 1)
	{
		if(ctrState.ui == UI_PARAM)
		{
			ctrState.ui = UI_SET;
			OLED_Clear();
			cdc_send_data((uint8_t *)"key 1 pressed\n", strlen("key 1 pressed\n"));
		}
		else if(ctrState.ui == UI_SET)
		{
			ctrState.ui = UI_PARAM;
			OLED_Clear();
		}
		else if(ctrState.ui == UI_ERR)
		{
			ctrState.SMFlag = Init;
			ctrState.ui = UI_PARAM;
			ctrState.pwm_output_flag = 0;
			ctrState.run_flag = RUN_MODE_STOP;

		}
		
	}
	
	// 控制模式切换
	else if (key_down == 2)
	{
		if(ctrState.ui == UI_PARAM)
		{
			param_set_idx = 0;
			ctrState.run_flag = RUN_MODE_STOP;
			ctrState.SMFlag = Wait;
			ctrState.ctr_mode = (ctrState.ctr_mode + 1) % 2;
			cdc_send_data((uint8_t *)"key 2 pressed\n", strlen("key 2 pressed\n"));
		}
		else
		{
			if(ctrState.out_mode == OUT_CV)
			{
				param_set_idx = (param_set_idx + 1) % (sizeof(param_set_step)/sizeof(param_set_step[0]));
			}
			else
			{
				param_set_idx = (param_set_idx + 1) % 3;
			}


		}
	}
	// 输出模式切换
	else if (key_down == 3)
	{
		if(ctrState.ui == UI_PARAM)
		{
			param_set_idx = 0;
			ctrState.run_flag = RUN_MODE_STOP;
			ctrState.SMFlag = Wait;
			ctrState.out_mode = !ctrState.out_mode;
		}
		else
		{
			if(ctrState.ctr_mode == CTR_BUCK)
			{
				if(ctrState.out_mode == OUT_CV)
				{
					ctr_value.Vyset_f32 += param_set_step[param_set_idx];
					if(ctr_value.Vyset_f32 > vol_range_y[1])
						ctr_value.Vyset_f32 = vol_range_y[1];
					loop_set_vol_y(ctr_value.Vyset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					ctr_value.Iyset_f32 += param_set_step[param_set_idx];
					if(ctr_value.Iyset_f32 > cur_range_y[1])
						ctr_value.Iyset_f32 = cur_range_y[1];
					loop_set_cur_y(ctr_value.Iyset_f32);
				}
				
			}
			else if(ctrState.ctr_mode == CTR_BOOST)
			{
				if(ctrState.out_mode == OUT_CV)
				{
					ctr_value.Vxset_f32 += param_set_step[param_set_idx];
					if(ctr_value.Vxset_f32 > vol_range_x[1])
						ctr_value.Vxset_f32 = vol_range_x[1];
					loop_set_vol_x(ctr_value.Vxset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					ctr_value.Ixset_f32 += param_set_step[param_set_idx];
					if(ctr_value.Ixset_f32 > cur_range_x[1])
						ctr_value.Ixset_f32 = cur_range_x[1];
					loop_set_cur_x(ctr_value.Ixset_f32);
				}
			}
		}
	}
	
	// 启动/关闭
	else if (key_down == 4)
	{
		if(ctrState.ui == UI_SET)
		{
			if(ctrState.ctr_mode == CTR_BUCK)
			{
				if(ctrState.out_mode == OUT_CV)
				{
					ctr_value.Vyset_f32 -= param_set_step[param_set_idx];
					if(ctr_value.Vyset_f32 < vol_range_y[0])
						ctr_value.Vyset_f32 = vol_range_y[0];
					loop_set_vol_y(ctr_value.Vyset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					ctr_value.Iyset_f32 -= param_set_step[param_set_idx];
					if(ctr_value.Iyset_f32 < cur_range_y[0])
						ctr_value.Iyset_f32 = cur_range_y[0];

					loop_set_cur_y(ctr_value.Iyset_f32);
				}
				
			}
			else if(ctrState.ctr_mode == CTR_BOOST)
			{
				if(ctrState.out_mode == OUT_CV)
				{
					ctr_value.Vxset_f32 -= param_set_step[param_set_idx];
					if(ctr_value.Vxset_f32 < vol_range_x[0])
						ctr_value.Vxset_f32 = vol_range_x[0];
					loop_set_vol_x(ctr_value.Vxset_f32);
				}
				else if(ctrState.out_mode == OUT_CC)
				{
					ctr_value.Ixset_f32 -= param_set_step[param_set_idx];
					if(ctr_value.Ixset_f32 < cur_range_x[0])
						ctr_value.Ixset_f32 = cur_range_x[0];
					loop_set_cur_x(ctr_value.Ixset_f32);
				}
			}

		}
		else if(ctrState.ui == UI_PARAM)
		{
			show_direct = !show_direct;
			OLED_ShowDirectInit(show_direct);
		}
	}
	else if(key_down == 5)
	{
		// 旋转编码器按下事件
		ctrState.run_flag = !ctrState.run_flag;

		if(ctrState.run_flag == RUN_MODE_STOP)
		{
			ctrState.SMFlag = Wait;	
		}
		fsm_Proc();
		
	}
		
}

uint32_t encoder_tick = 0;
void encoder_Proc(void)
{
	if(uwTick - encoder_tick < 200)
		return;
	
	encoder_tick = uwTick;
	static int32_t last_cnt = 0;
	int32_t cnt = encoder_get_count();

	if(ctrState.ui == UI_SET)
	{
		if(ctrState.ctr_mode == CTR_BUCK)
		{
			if(ctrState.out_mode == OUT_CV)
			{
				ctr_value.Vyset_f32 = 0.1f*cnt;
				if(ctr_value.Vyset_f32 > vol_range_y[1])
					ctr_value.Vyset_f32 = vol_range_y[1];
				loop_set_vol_y(ctr_value.Vyset_f32);
			}
			else if(ctrState.out_mode == OUT_CC)
			{
				ctr_value.Iyset_f32 = cnt*0.1f;
				if(ctr_value.Iyset_f32 > cur_range_y[1])
					ctr_value.Iyset_f32 = cur_range_y[1];
				loop_set_cur_y(ctr_value.Iyset_f32);
			}
			
		}
		else if(ctrState.ctr_mode == CTR_BOOST)
		{
			if(ctrState.out_mode == OUT_CV)
			{
				ctr_value.Vxset_f32 = cnt*0.1f;
				if(ctr_value.Vxset_f32 > vol_range_x[1])
					ctr_value.Vxset_f32 = vol_range_x[1];
				loop_set_vol_x(ctr_value.Vxset_f32);
			}
			else if(ctrState.out_mode == OUT_CC)
			{
				ctr_value.Ixset_f32 = cnt*0.1f;
				if(ctr_value.Ixset_f32 > cur_range_x[1])
					ctr_value.Ixset_f32 = cur_range_x[1];
				loop_set_cur_x(ctr_value.Ixset_f32);
			}
		}
	}
	last_cnt = cnt;


}



