#include "sys_bsp.h"
#include "adc_dc_sample.h"


ADC_SAMPLE_t my_adc_sample;

uint16_t open_value = 0;

static uint8_t cntSCP = 0;
static uint8_t LoopCnt = 0;
static _Bool cnt_flag = 0;
void ADC_ScpProtection(void)
{
	if(adc_value[0]<1300&&adc_value[2]<1000)
	{
		cnt_flag = 1;
		
	}
	if(cnt_flag)
	{
		LoopCnt++;
		
		if(adc_value[0]<1300&&adc_value[2]<1000)
		{
			cntSCP++;
		}
		if(LoopCnt>15)
		{
			LoopCnt = 0;
			cnt_flag = 0;
			if(cntSCP>10)
			{
				ctrState.run_flag = RUN_MODE_STOP;
				cntSCP = 0;
//				HRTIM1->sTimerxRegs[1].CMP1xR = 100; // 装载比较值
//				HRTIM1->sTimerxRegs[1].CMP2xR = 23040; // 装载比较值
//				HRTIM1->sTimerxRegs[1].CMP3xR = 23040; // 装载比较值
//				HRTIM1->sTimerxRegs[1].CMP4xR = 50; // 装载比较值
				
			}
			else
			{
				cntSCP = 0;
			}
			
		}
	}
	else
	{
		LoopCnt = 0;
		cnt_flag = 0;
	}
}



void ADC_ocpProtection(void)
{
	if(ctrState.ctr_mode !=CTR_BUCK)
		return;
	
	
	
}


void adc_sample_start(void)
{
	adc_calibration_enable(ADC0);
	
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
	
}

const float ka = 1.00f,kb = 0.0f;
static uint32_t VxSum = 0, IxSum = 0, VySum = 0, IySum = 0;
CCMRAM void adc_filter(void)
{
	const uint8_t filter_size = 2;

	VxSum = VxSum+my_adc_sample.Vx_raw - (VxSum>>filter_size);
	my_adc_sample.Vx_q15 = VxSum>>filter_size;
	
	VySum = VySum+my_adc_sample.Vy_raw - (VySum>>filter_size);
	my_adc_sample.Vy_q15 = VySum>>filter_size;
	
	IxSum = IxSum+my_adc_sample.Ix_raw - (IxSum>>filter_size);
	my_adc_sample.Ix_q15 = (IxSum>>filter_size);
	
	IySum = IySum+my_adc_sample.Iy_raw - (IySum>>filter_size);
	my_adc_sample.Iy_q15 = (IySum>>filter_size);
	
	
	
}

void adc_disp(void)
{
	q15_t tmp = 0;
	//adc_filter();

	//计算Ix电流
	tmp = ((my_adc_sample.Ix_q15- my_ctrvalue.offset)>0)?\
	(my_adc_sample.Ix_q15- my_ctrvalue.offset):(my_ctrvalue.offset-my_adc_sample.Ix_q15);
	my_adc_sample.Ix_f32 = tmp*my_ctrvalue.Gi_re;
	
	//计算Iy电流
	tmp = ((my_adc_sample.Iy_q15- my_ctrvalue.offset)>0)?\
	(my_adc_sample.Iy_q15- my_ctrvalue.offset):(my_ctrvalue.offset-my_adc_sample.Iy_q15);
	my_adc_sample.Iy_f32 = tmp*my_ctrvalue.Gi_re;
	if(my_adc_sample.Iy_f32<-0.001f)
	{
		my_adc_sample.Iy_f32 = 0.00f;
	}
	
	my_adc_sample.Vx_f32 = my_adc_sample.Vx_q15*my_ctrvalue.Gv_re;
	// my_adc_sample.Vx_f32 = (my_adc_sample.Vx_f32<-0.001f)?0:my_adc_sample.Vx_f32;
	my_adc_sample.Vy_f32 = my_adc_sample.Vy_q15*my_ctrvalue.Gv_re;

}



const uint8_t cnt = 32;
void adc_AutoGetOffset(void)
{
	uint8_t i = 0;
	
	
	uint32_t sum = 0;
	
	for(i = 0;i < cnt;i++)
	{
		sum += adc_value[2];
		//HAL_Delay(1);
	}
	my_ctrvalue.offset = sum/cnt;
}

