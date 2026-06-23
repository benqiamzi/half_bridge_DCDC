#include "gd32e10x.h"
#include "gd32e10x_eval.h"
#include "systick.h"

volatile uint16_t adc_value[ADC_CH_NUM];


/* 函数声明 */
void gpio_config(void);
void timer5_config(void);
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

    rcu_periph_clock_enable(RCU_CRC);

    /* 测试功能引脚 */
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    // 输出化蜂鸣器引脚
    gpio_init(BEEP_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, BEEP_PIN);
    gpio_bit_reset(BEEP_GPIO_PORT, BEEP_PIN);
	
	/* 按键引脚 */
    gpio_init(KEY1_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY1_PIN);
    gpio_init(KEY2_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY2_PIN);
    gpio_init(KEY3_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY3_PIN);
    gpio_init(KEY4_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY4_PIN);
    gpio_init(ENCODER_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, ENCODER_PIN);

    /* 互补 PWM 引脚 */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUCK_PWM_HIGH_PIN);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUCK_PWM_LOW_PIN);
    /* ADC 采样引脚 */
	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX, \
				VOLT_VY_PIN|VOLT_IY_PIN|VOLT_IL_PIN|VOLT_VX_PIN|VOLT_IX_PIN);
    
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX,VOLT_REF_PIN);
   
    /* LED 引脚 */
    gpio_init(LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OCP_LED_PIN | OVP_LED_PIN | SCP_LED_PIN);
    gpio_bit_set(LED_PORT, OCP_LED_PIN | OVP_LED_PIN | SCP_LED_PIN);   // 初始熄灭
	
	/* USB 引脚 */
	gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_11|GPIO_PIN_12);
	
}

/*!
    \brief      TIMER0 互补 PWM 配置（无刹车功能）
    - PWM 频率：约 40kHz（period = 2399, prescaler = 1, CK_TIMER = 60MHz）
    - 死区时间：约 100ns
*/
void timer5_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;
	timer_break_parameter_struct timer_breakpara;
    
    rcu_periph_clock_enable(RCU_TIMER5);
    rcu_periph_reset_enable(RCU_TIMER5RST);
    rcu_periph_reset_disable(RCU_TIMER5RST);

    /* 1ms 定时器，定时器时钟 60MHz，预分频 60，计数频率 1MHz，ARR = 999 */
    timer_deinit(TIMER5);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 2;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 3999;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER5, &timer_initpara);

    /* 使能更新中断 */
    timer_interrupt_flag_clear(TIMER5, TIMER_INT_UP);
    timer_interrupt_enable(TIMER5, TIMER_INT_UP);

	/* TIM 更新事件作为 TRGO */
	timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);
    
    /* 使能主输出、自动重载影子寄存器，启动定时器 */
    timer_primary_output_config(TIMER5, ENABLE);
    timer_auto_reload_shadow_enable(TIMER5);
    timer_enable(TIMER5);
}

/*!
    \brief      TIM7 从定时器配置（配合TIM0实现PWM半周期触发ADC）
    \details    1. 架构：TIM0(主PWM) → 内部触发 → TIM7(从定时器)，双定时器严格同步
                2. 计数参数：与TIM0完全一致(预分频0、周期2999)，保证同频
                3. 工作模式：
                   - 从模式：由TIM0更新事件同步启动计数
                   - 比较模式：计数到1499(PWM半周期)产生比较匹配事件
                   - 主模式：比较匹配事件作为TRGO输出，触发ADC注入通道
                4. 用途：PWM周期中点采样，等效采集电感平均电流
    \param[in]  none
    \param[out] none
    \retval     none
*/
void timer7_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;   // 通道比较/输出配置结构体
    timer_parameter_struct timer_initpara;      // 定时器时基结构体

    /* ========== 1. 开启时钟 + 软件复位TIM7 ========== */
    rcu_periph_clock_enable(RCU_TIMER2);
    rcu_periph_reset_enable(RCU_TIMER2RST);
    rcu_periph_reset_disable(RCU_TIMER2RST);

    timer_deinit(TIMER7);                         // 复位TIM7所有寄存器
    timer_struct_para_init(&timer_initpara);    // 初始化结构体默认值

    /* ========== 2. 时基配置（与TIM0完全一致，保证同步） ========== */
    timer_initpara.prescaler         = 0;       // 不分频，时钟与TIM0一致
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP; // 向上计数
    timer_initpara.period            = 2999;     // 自动重装载值，和TIM0相同
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;       // 通用定时器固定为0
    timer_init(TIMER7, &timer_initpara);

    /* ========== 3. 配置TIM7 内部触发源：选择TIM0(ITR0) ========== */
    // TIM7 内部触发输入 = ITR0，硬件绑定为 TIM0
    timer_input_trigger_source_select(TIMER7, TIMER_SMCFG_TRGSEL_ITI0);

    // /* ========== 4. 配置TIM7 从模式：触发模式（由TIM0信号同步启动计数） ========== */
    // // 从模式：检测到内部触发信号后，TIM7开始计数
    // timer_slave_mode_config(TIMER7, TIMER_SLAVE_MODE_TRIGGER);

    /* ========== 5. 配置TIM7_CH0 为比较模式（内部比较，不输出引脚） ========== */
    timer_channel_output_struct_para_init(&timer_ocintpara);
    timer_ocintpara.outputstate    = TIMER_CCX_DISABLE;    // 关闭引脚输出（无需PWM波形）
    timer_ocintpara.outputnstate   = TIMER_CCXN_DISABLE;   // 无互补通道，固定关闭
    timer_ocintpara.ocpolarity     = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity    = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.ocidlestate    = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate   = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER7, TIMER_CH_0, &timer_ocintpara);

    // 设置为【冻结比较模式】：仅内部比较，不改变引脚电平
    timer_channel_output_mode_config(TIMER7, TIMER_CH_0, TIMER_OC_MODE_INACTIVE);
    // // 比较值 = 1499 （2999/2，PWM半周期中点）
    // timer_channel_output_pulse_value_config(TIMER7, TIMER_CH_0, 1499);
    // 开启影子寄存器，保证比较值更新无毛刺
    timer_channel_output_shadow_config(TIMER7, TIMER_CH_0, TIMER_OC_SHADOW_ENABLE);

    /* ========== 6. 配置TIM7 主模式：比较匹配事件作为TRGO输出 ========== */
    // TRGO 触发源 = CH0 比较匹配事件（计数到1499时输出触发信号）
    timer_master_output_trigger_source_select(TIMER7, TIMER_TRI_OUT_SRC_O0CPRE);

    /* ========== 7. 使能模块并启动TIM7 ========== */
    timer_primary_output_config(TIMER7, ENABLE);    // 使能通道主输出
    timer_auto_reload_shadow_enable(TIMER7);        // 自动重装载影子寄存器
    timer_enable(TIMER7);                           // 启动TIM7
}

void encoder_config(void)
{
    timer_parameter_struct timer_initpara;
    timer_ic_parameter_struct icpara;

    /* 1) 使能 GPIOB 和定时器时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_periph_clock_enable(RCU_AF);

    /* 2) PB14/PB15 作为输入引脚，建议上拉 */
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_14 | GPIO_PIN_15);

    /* 3) 定时器基础配置 */
    timer_deinit(TIMER1);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 0;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 25;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 3;
    timer_init(TIMER1, &timer_initpara);

    /* 4) 配置通道 2/3 为输入捕获，直接 TI 模式 */
    timer_channel_input_struct_para_init(&icpara);
    icpara.icpolarity  = TIMER_IC_POLARITY_RISING;
    icpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    icpara.icprescaler = TIMER_IC_PSC_DIV1;
    icpara.icfilter    = 0;
    timer_input_capture_config(TIMER1, TIMER_CH_0, &icpara);
    timer_input_capture_config(TIMER1, TIMER_CH_1, &icpara);

    /* 5) 启用编码器模式 */
    timer_quadrature_decoder_mode_config(TIMER1,
        TIMER_QUAD_DECODER_MODE0,
        TIMER_IC_POLARITY_RISING,
        TIMER_IC_POLARITY_RISING);

    timer_enable(TIMER1);
}

/*!
    \brief      TIM0 高级定时器互补PWM配置
    \details    1. 定时器时钟：TIM0挂载APB2总线，内核时钟 = 60MHz
                2. PWM频率计算：60MHz / (0 + 1) / (2999 + 1) = 20 KHz
                3. 配置互补输出 + 死区保护，适用于半桥/全桥DCDC拓扑
                4. 定时器更新事件作为TRGO信号，供给ADC做外部触发
                5. 初始化默认关闭PWM引脚输出，定时器内核持续运行，不影响ADC采样
                6. 开启PWM影子寄存器，占空比同步更新，波形无毛刺
    \param[in]  none
    \param[out] none
    \retval     none
*/
void timer0_pwm_config(void)
{
    // 定时器通道输出配置结构体
    timer_oc_parameter_struct timer_ocintpara;
    // 定时器时基配置结构体
    timer_parameter_struct timer_initpara;
    // 定时器死区/刹车配置结构体
	timer_break_parameter_struct timer_breakpara;

    /* ========== 1. 开启TIM0时钟 + 软件复位外设 ========== */
    rcu_periph_clock_enable(RCU_TIMER0);        // 使能TIM0外设时钟
    rcu_periph_reset_enable(RCU_TIMER0RST);     // 开启TIM0软件复位
    rcu_periph_reset_disable(RCU_TIMER0RST);    // 关闭复位，定时器恢复正常状态


    /* ========== 2. 定时器时基初始化 ========== */
    timer_deinit(TIMER0);                       // 复位TIM0所有寄存器为默认值
    timer_struct_para_init(&timer_initpara);    // 结构体成员初始化为默认值

    timer_initpara.prescaler         = 0;       // 预分频器：不分频，定时器时钟 = 60MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE; // 边沿对齐模式（标准PWM）
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;  // 向上计数模式
    timer_initpara.period            = 2999;    // 自动重装载值：计数范围 0 ~ 2999
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;  // 定时器时钟不分频
    timer_initpara.repetitioncounter = 0;       // 高级定时器重复计数器，通用场景固定为0
    timer_init(TIMER0, &timer_initpara);        // 写入时基配置，完成定时器基础设置


    /* ========== 3. TIM0_CH0 互补通道输出配置 ========== */
    timer_channel_output_struct_para_init(&timer_ocintpara);

    // 核心配置：初始化【关闭主通道、互补通道输出】
    // 仅关闭引脚驱动，定时器内部计数/比较/更新事件正常运行，ADC触发不受影响
    timer_ocintpara.outputstate  = TIMER_CCX_DISABLE;    // 关闭CH0主输出(PA8)
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;   // 关闭CH0互补输出(PB13)

    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;  // 主通道有效电平：高电平
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH; // 互补通道有效电平：高电平
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW; // 空闲状态：主通道输出低电平
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;// 空闲状态：互补通道输出低电平
    timer_channel_output_config(TIMER0, TIMER_CH_0, &timer_ocintpara); // 写入通道配置

    // 1. 通道1输出配置（禁用引脚输出，仅保留内部比较事件）
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;   // 关闭CH1引脚输出(PA9)，不影响ADC触发
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_channel_output_config(TIMER0, TIMER_CH_1, &timer_ocintpara);

    // 2. 设置通道1比较值 = PWM半周期，产生ADC固定的中点触发
    // 公式: CH1CV = TIMER0周期 / 2 = 2999 / 2 = 1499
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_1, timer_initpara.period / 2);

    // 3. 配置主触发源为通道1比较事件（TRGO信号，与ADC触发无关）
    timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_O1CPRE);


    /* ========== 4. PWM模式 + 初始占空比配置 ========== */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 100); // 初始比较值(占空比基准)：10
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0); // 选择PWM模式0
    timer_channel_output_mode_config(TIMER0, TIMER_CH_1, TIMER_OC_MODE_PWM1); // 选择PWM模式1（上升沿在比较点）
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_ENABLE); // 开启影子寄存器
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_1, TIMER_OC_SHADOW_ENABLE); // CH1影子寄存器
    // 影子寄存器作用：更新占空比时，在下一个PWM周期统一生效，避免波形跳变


    /* ========== 5. 死区 + 刹车功能配置（半桥拓扑防直通） ========== */
    timer_break_struct_para_init(&timer_breakpara);

    timer_breakpara.runoffstate      = TIMER_ROS_STATE_DISABLE;  // 禁止运行态输出
    timer_breakpara.ideloffstate     = TIMER_IOS_STATE_DISABLE ; // 禁止空闲态输出
    timer_breakpara.deadtime         = 50;                      // 死区时间配置：60个定时器时钟周期
                                                                // 60 / 60MHz = 1us 死区，防止上下管直通
    timer_breakpara.breakpolarity    = TIMER_BREAK_POLARITY_LOW; // 刹车引脚低电平有效（本工程未使用刹车）
    timer_breakpara.outputautostate  = TIMER_OUTAUTO_ENABLE;     // 自动输出使能：刹车解除后自动恢复PWM输出
    timer_breakpara.protectmode      = TIMER_CCHP_PROT_0;        // 保护等级：无写保护
    timer_breakpara.breakstate       = TIMER_BREAK_DISABLE;      // 关闭外部刹车功能（本工程不使用硬件刹车）
    timer_break_config(TIMER0, &timer_breakpara);               // 写入死区/刹车配置

    /* ========== 7. 使能主输出 + 启动定时器 ========== */
    timer_primary_output_config(TIMER0, ENABLE);  // 高级定时器互补输出必须开启主输出使能
    timer_auto_reload_shadow_enable(TIMER0);      // 开启自动重装载影子寄存器
    timer_enable(TIMER0);                         // 启动TIM0定时器内核，开始计数
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
    dma_data_parameter.number = ADC_CH_NUM;
    dma_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH0, &dma_data_parameter);

    dma_circulation_enable(DMA0, DMA_CH0);
	
	dma_interrupt_enable(DMA0,DMA_CH0,DMA_INT_FTF);//全部搬运完成后触发中断
  
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH0);
	
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
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_CH_NUM);
    /* ADC regular channel config */
	//采样顺序：IL、IY、IX、VY、VX
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_2, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_0, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_6, ADC_SAMPLETIME_7POINT5);
	adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_1, ADC_SAMPLETIME_7POINT5);
    adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_3, ADC_SAMPLETIME_7POINT5);
	
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T0_CH1);
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
    nvic_irq_enable(TIMER5_IRQn, 2, 0);
}



/*!
    \brief      汇聚所有外设初始化
*/
void peripheral_config(void)
{
    gpio_config();
   
	dma_config();
    adc0_config();
	timer0_pwm_config();

    timer5_config();
    encoder_config();
	
	nvic_config();
}