#include "gd32e10x.h"
#include "gd32e10x_eval.h"
#include "systick.h"

uint16_t adc_value[5];

/* 全局变量（仅占空比，供后续应用层修改）*/
uint16_t pwm_duty_cycle = 1200;   // 0 ~ 2399

/* 函数声明 */
void gpio_config(void);
void timer0_pwm_config(void);
void adc0_config(void);
void peripheral_config(void);

/*!
    \brief      GPIO 配置
    - PA8 复用为 TIMER0_CH0
    - PB13 复用为 TIMER0_CH0N
    - PA0, PA1 为模拟输入（ADC）
    - PC6, PC7, PC8 为推挽输出（LED）
*/
void gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_ADC0);
	rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV4);
	

    
    /* 互补 PWM 引脚 */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUCK_PWM_HIGH_PIN);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUCK_PWM_LOW_PIN);
    /* ADC 采样引脚 */
	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX, \
				VOLT_VY_PIN|VOLT_IY_PIN|VOLT_IL_PIN|VOLT_VX_PIN|VOLT_IX_PIN);
   
    /* LED 引脚 */
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED1_PIN | LED2_PIN | LED3_PIN);
    gpio_bit_set(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN);   // 初始熄灭
	
	/* USB 引脚 */
	gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_11|GPIO_PIN_12);
	
}

/*!
    \brief      TIMER0 互补 PWM 配置（无刹车功能）
    - PWM 频率：约 40kHz（period = 2399, prescaler = 1, CK_TIMER = 60MHz）
    - 死区时间：约 100ns
*/
void timer0_pwm_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;
	timer_break_parameter_struct timer_breakpara;
    
    rcu_periph_clock_enable(RCU_TIMER0);
    rcu_periph_reset_enable(RCU_TIMER0RST);
    rcu_periph_reset_disable(RCU_TIMER0RST);

    
    timer_deinit(TIMER0);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 0;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 2999;          
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0, &timer_initpara);
    
    /* 通道0 配置（含互补输出）*/
    timer_channel_output_struct_para_init(&timer_ocintpara);
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.outputnstate = TIMER_CCXN_ENABLE;
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, TIMER_CH_0, &timer_ocintpara);
    
    /* PWM 模式0，占空比初始值 */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, pwm_duty_cycle);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
    
    /* 死区时间配置（100ns @ 120MHz）*/
    timer_break_struct_para_init(&timer_breakpara);
    /* automatic output enable, break, dead time and lock configuration*/
    timer_breakpara.runoffstate      = TIMER_ROS_STATE_DISABLE;
    timer_breakpara.ideloffstate     = TIMER_IOS_STATE_DISABLE ;
    timer_breakpara.deadtime         = 50;
    timer_breakpara.breakpolarity    = TIMER_BREAK_POLARITY_LOW;
    timer_breakpara.outputautostate  = TIMER_OUTAUTO_ENABLE;
    timer_breakpara.protectmode      = TIMER_CCHP_PROT_0;
    timer_breakpara.breakstate       = TIMER_BREAK_DISABLE;
    timer_break_config(TIMER0, &timer_breakpara);
	
	/*TIM更新事件作为TRGO，送给ADC外部触发*/
	timer_master_output_trigger_source_select(TIMER0,TIMER_TRI_OUT_SRC_O0CPRE);
    
    /* 使能主输出、自动重载影子寄存器，启动定时器 */
    timer_primary_output_config(TIMER0, ENABLE);
    timer_auto_reload_shadow_enable(TIMER0);
    timer_enable(TIMER0);
}

void dma_config(void)
{
	
	rcu_periph_clock_enable(RCU_DMA0);
	
    /* ADC_DMA_channel configuration */
    dma_parameter_struct dma_data_parameter;
    
    /* ADC DMA_channel configuration */
    dma_deinit(DMA0, DMA_CH0);
    
    /* initialize DMA single data mode */
    dma_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory_addr = (uint32_t)(adc_value);
    dma_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;  
    dma_data_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_data_parameter.number = 5;
    dma_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH0, &dma_data_parameter);

    dma_circulation_enable(DMA0, DMA_CH0);
	
	dma_interrupt_enable(DMA0,DMA_CH0,DMA_INT_FTF);//全部搬运完成后触发中断
  
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH0);
	
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/*!
    \brief      ADC0 配置（扫描两通道：电压/电流）
*/
void adc0_config(void)
{
    /* reset ADC */
    adc_deinit(ADC0);
    /* ADC mode config */
    adc_mode_config(ADC_MODE_FREE);
    /* ADC continuous function enable */
    //adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode disable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    
    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 5);
    /* ADC regular channel config */
	//采样顺序：IL、IY、IX、VY、VX
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_2, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_0, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_6, ADC_SAMPLETIME_7POINT5);
	adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_1, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_3, ADC_SAMPLETIME_7POINT5);
	
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T0_CH0);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	
    /* enable ADC interface */
    adc_enable(ADC0);
    delay_1ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* ADC DMA function enable */
    adc_dma_mode_enable(ADC0);
    
}

/*!
    \brief      configure DMA interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    nvic_irq_enable(DMA0_Channel0_IRQn, 0, 0);
}

/*!
    \brief      this function handles DMA0_Channel3_IRQHandler interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel0_IRQHandler(void)
{
    if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)){     
		
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
        gpio_bit_reset(LED3_GPIO_PORT, LED3_PIN);
		
    }
}

/*!
    \brief      汇聚所有外设初始化
*/
void peripheral_config(void)
{
    gpio_config();
    timer0_pwm_config();
	dma_config();
    adc0_config();
	
	nvic_config();
}