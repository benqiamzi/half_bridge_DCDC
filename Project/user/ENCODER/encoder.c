#include "encoder.h"
#include "gd32e10x.h"

/* 静态计数值（在中断中更新，加 volatile 防止优化） */
static volatile int32_t encoder_count = 120;
static volatile uint16_t encoder_step = 1;

/*!
 * \brief 初始化编码器外部中断
 * - 使能 GPIOB 时钟
 * - 配置 PB15 为上拉输入模式
 * - 配置 EXTI15 为上升沿/下降沿触发
 * - 使能 NVIC 中断
 */
void encoder_init(void)
{
    /* 1. 使能 GPIOB 和 AFIO 时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);           // AFIO 时钟，用于 EXTI 端口映射

    /* 2. 配置 PB14/PB15 为上拉输入模式 */
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, ENCODER_A|ENCODER_B);

    /* 3. 将 EXTI15 映射到 PB15（默认映射到 PA15，不配则永远收不到信号） */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_15);

    /* 4. 配置 EXTI15：上升沿触发 */
    exti_init(EXTI_15, EXTI_INTERRUPT, EXTI_TRIG_RISING);

    /* 5. 清除可能挂起的中断标志 */
    exti_interrupt_flag_clear(EXTI_15);

    /* 6. 使能 EXTI15 中断并设置优先级 */
    nvic_irq_enable(EXTI10_15_IRQn, 2, 0);
}

/*!
 * \brief 获取当前计数值
 */
int32_t encoder_get_count(void)
{
    return encoder_count;
}

/*!
 * \brief 设置当前计数值
 */
void encoder_set_count(int32_t count)
{
    encoder_count = count;
}

/*!
 * \brief 设置计数步进
 */
void encoder_set_step(uint16_t step)
{
    encoder_step = (step>0)?encoder_step:1;
}

/*!
 * \brief 重置计数值为 0
 */
void encoder_reset_count(void)
{
    encoder_count = 0;
}

/*!
 * \brief EXTI10~15 中断服务函数
 *        每次 PB15 边沿触发时，计数值 +1 或 -1（取决于方向判断）
 */
void EXTI10_15_IRQHandler(void)
{
    static uint8_t cnt = 0;
    /* 检查是否为 EXTI15 中断 */
    if (exti_interrupt_flag_get(EXTI_15) != RESET) {
        /* 单相计数：每次触发计数值 +1 */
        cnt++;
        if(cnt == 1)
        {
            cnt = 0;
            if(gpio_input_bit_get(GPIOB, ENCODER_A))
                encoder_count+=encoder_step;
            else
                encoder_count-=encoder_step;
        }

        /* 清除中断标志 */
        exti_interrupt_flag_clear(EXTI_15);
    }
}