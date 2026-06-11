//PA8->TA1 HA1
//PA9->TA2 LA1
//PA10->TB1 HB2
//PA11->TB2 LB2
#include "sys_bsp.h"
#include "CtlLoop.h"

PWM_VALUE_t pwm_value ={
	.Q1Duty = MIN_DUTY,
	.Q2Duty = MIN_DUTY,
	.Q1MaxDuty = MIN_DUTY,
	.Q2MaxDuty = MIN_DUTY
};

#define DEADTIME 400

CtlValue_t my_ctrvalue;

void CtlValue_Init(void)
{
	my_ctrvalue.Vyset_f32 = 12.0f;
	my_ctrvalue.Iyset_f32 = 0.5f;

	my_ctrvalue.Vxset_f32 = 24.0f;
	my_ctrvalue.Ixset_f32 = 0.5f;
	
	my_ctrvalue.Ri_sample = 10;
	
	my_ctrvalue.k_iy_a = 1.014f;
	my_ctrvalue.k_iy_b = -0.04f;

	my_ctrvalue.offset = 2048;
	my_ctrvalue.Gv = (4095.0f/3.3f)/(13.0f/1.0f);//实际电压/数字量
	my_ctrvalue.Gv_re = 1.0f/my_ctrvalue.Gv;
	
	my_ctrvalue.Gi_re = 3.3f/4095/(33.0f/1.0f)/my_ctrvalue.Ri_sample*1000.0f;
	my_ctrvalue.Gi = 1.0f/my_ctrvalue.Gi_re;	 // 电流环增益
	
	
	my_ctrvalue.Vyset_q15 = my_ctrvalue.Vyset_f32 * my_ctrvalue.Gv;
	my_ctrvalue.Iyset_q15 = (my_ctrvalue.Iyset_f32*my_ctrvalue.k_iy_a+my_ctrvalue.k_iy_b) \
				* my_ctrvalue.Gi+my_ctrvalue.offset;

	my_ctrvalue.Vxset_q15 = my_ctrvalue.Vxset_f32 * my_ctrvalue.Gv;
	my_ctrvalue.Ixset_q15 = my_ctrvalue.offset - (my_ctrvalue.Ixset_f32*my_ctrvalue.k_iy_a+my_ctrvalue.k_iy_b) \
				* my_ctrvalue.Gi;
}

void BoostOpenLoopTest(void)
{

}

void BuckOpenLoopTest(void)
{


}

uint16_t duty_limit(PWM_VALUE_t * pwm_value,uint16_t value, CTR_MODE mode)
{

	if(mode == CTR_BUCK)
	{
		if(value>pwm_value->Q1MaxDuty)
		{
			value=pwm_value->Q1MaxDuty;
		}
		if(value<MIN_DUTY)
		{
			value=MIN_DUTY;
		}
	}
	else if(mode == CTR_BOOST)
	{
		if(value>pwm_value->Q2MaxDuty)
		{
			value=pwm_value->Q2MaxDuty;
		}
		if(value<MIN_DUTY)
		{
			value=MIN_DUTY;
		}
		
	}

	return value;
}

void duty_change(uint16_t duty,uint8_t ch)
{
	timer_channel_output_pulse_value_config(TIMER0, ch, duty);
}

void ctr_pwm_ch_start(void)
{
	TIMER_CHCTL2(TIMER0) &= (~(uint32_t)TIMER_CHCTL2_CH0EN);
	TIMER_CHCTL2(TIMER0) |= (uint32_t)TIMER_CCX_ENABLE;
}

void ctr_pwm_chn_start(void)
{

	TIMER_CHCTL2(TIMER0) &= (~(uint32_t)TIMER_CHCTL2_CH0NEN);
	TIMER_CHCTL2(TIMER0) |= (uint32_t)TIMER_CCXN_ENABLE;
}

void ctr_pwm_start(void)
{
	// ctr_pwm_ch_start();
	// ctr_pwm_chn_start();
}

void ctr_pwm_stop(void)
{
	if(ctrState.ctr_mode == CTR_BUCK)
	{
		duty_change(MIN_DUTY, TIMER_CH_0);
	}
	else if(ctrState.ctr_mode == CTR_BOOST)
	{
		duty_change(PERIOD-MIN_DUTY, TIMER_CH_0);
	}
}

void loop_set_vol_x(float vol)
{
	my_ctrvalue.Vxset_f32 = vol;
	my_ctrvalue.Vxset_q15 = my_ctrvalue.Vxset_f32 * my_ctrvalue.Gv;
}

void loop_set_vol_y(float vol)
{
	my_ctrvalue.Vyset_f32 = vol;
	my_ctrvalue.Vyset_q15 = my_ctrvalue.Vyset_f32 * my_ctrvalue.Gv;
}

void loop_set_cur_x(float cur)
{
	my_ctrvalue.Ixset_f32 = cur;
	my_ctrvalue.Ixset_q15 = my_ctrvalue.offset - (my_ctrvalue.Ixset_f32*my_ctrvalue.k_iy_a+my_ctrvalue.k_iy_b) \
				* my_ctrvalue.Gi;
}

void loop_set_cur_y(float cur)
{
	my_ctrvalue.Iyset_f32 = cur;
	my_ctrvalue.Iyset_q15 = (my_ctrvalue.Iyset_f32*my_ctrvalue.k_iy_a+my_ctrvalue.k_iy_b) \
				* my_ctrvalue.Gi+my_ctrvalue.offset;
}

void LoopCtl(void)
{
	float tmp = 0;
	
	//降压电流环:
	if(ctrState.ctr_mode == CTR_BUCK)
	{
		if(ctrState.out_mode == OUT_CV)
		{
			
			tmp = PID_Update(&pid_CV_Buck, my_ctrvalue.Vyset_q15, my_adc_sample.Vy_q15, 1.0f/40e3);
			
			pwm_value.Q1Duty = (uint16_t)tmp;
			
			//pwm_value.Q2Duty = 3000 - pwm_value.Q2Duty;
			
			duty_limit(&pwm_value, pwm_value.Q1Duty, CTR_BUCK);
			
			duty_change(pwm_value.Q1Duty,TIMER_CH_0);
		}
		else if(ctrState.out_mode == OUT_CC)
		{
			tmp = PID_Update(&pid_CC_Buck, my_ctrvalue.Iyset_q15, my_adc_sample.Iy_raw, 1.0f/40e3);
			pwm_value.Q1Duty = (uint16_t)tmp;
			
			//pwm_value.Q2Duty = 3000 - pwm_value.Q2Duty;
			
			duty_limit(&pwm_value, pwm_value.Q1Duty, CTR_BUCK);
			
			duty_change(pwm_value.Q1Duty,TIMER_CH_0);
			//duty_change(1500,TIMER_CH_0);
		}
	}
	else if(ctrState.ctr_mode == CTR_BOOST)
	{
		if(ctrState.out_mode == OUT_CV)
		{
			tmp = PID_Update(&pid_CV_Boost, my_ctrvalue.Vxset_q15, my_adc_sample.Vx_q15, 1.0f/40e3);

			pwm_value.Q2Duty = duty_limit(&pwm_value, (uint16_t)tmp, CTR_BOOST);

			pwm_value.Q2Duty =3000 - pwm_value.Q2Duty;
			
			duty_change(pwm_value.Q2Duty,TIMER_CH_0);
			// duty_change(1500,TIMER_CH_0);
		}
		else if(ctrState.out_mode == OUT_CC)
		{
			tmp = PID_Update(&pid_CC_Boost, \
			my_ctrvalue.offset - my_ctrvalue.Ixset_q15, \
			my_ctrvalue.offset -my_adc_sample.Ix_raw, 1.0f/40e3);
			
			pwm_value.Q2Duty = duty_limit(&pwm_value, (uint16_t)tmp, CTR_BOOST);
			pwm_value.Q2Duty = 3000 - pwm_value.Q2Duty;
			duty_change(pwm_value.Q2Duty,TIMER_CH_0);
			//  duty_change(1500,TIMER_CH_0);
		}
	}
		
		
	
}

/*!
    \brief      this function handles DMA0_Channel0_IRQHandler interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel0_IRQHandler(void)
{
    if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)){
        gpio_bit_set(GPIOA,GPIO_PIN_10);
        my_adc_sample.Ix_raw =  adc_value[2];
		my_adc_sample.Iy_raw =  adc_value[1];
		
        my_adc_sample.Vy_raw = adc_value[3];
		my_adc_sample.Vx_raw = adc_value[4];
		
		if(ctrState.pwm_output_flag == 1)
        {
			/* 先清标志再执行 LoopCtl，避免耗时处理期间新 FTF 到达后连带被清 */
        	//adc_filter();
        	LoopCtl();
		}

        gpio_bit_reset(GPIOA,GPIO_PIN_10);
		dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
    }
}

