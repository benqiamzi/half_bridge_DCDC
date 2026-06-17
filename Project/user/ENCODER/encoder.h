#ifndef __ENCODER_H
#define __ENCODER_H

#include <stdint.h>

#define ENCODER_A GPIO_PIN_14
#define ENCODER_B GPIO_PIN_15

/* 初始化编码器输入捕获（PB15 -> TIM11_CH0） */
void encoder_init(void);

/* 获取当前计数值（可正可负，取决于方向判断逻辑） */
int32_t encoder_get_count(void);
void encoder_set_count(int32_t count);
void encoder_set_step(uint16_t step);

/* 重置计数值为 0 */
void encoder_reset_count(void);

#endif