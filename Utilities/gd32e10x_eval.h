/*!
    \file    gd32e10x_eval.h
    \brief   definitions for GD32E10X_EVAL's leds, keys and COM ports hardware resources
    
    \version 2026-02-03, V1.9.0, firmware for GD32E10x
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#ifndef GD32E10X_EVAL_H
#define GD32E10X_EVAL_H

#ifdef cplusplus
 extern "C" {
#endif

#include "gd32e10x.h"

#define BUCK_PWM_HIGH_PORT        GPIOA
#define BUCK_PWM_HIGH_PIN         GPIO_PIN_8      /* TIMER0_CH0 */
#define BUCK_PWM_LOW_PORT         GPIOB
#define BUCK_PWM_LOW_PIN          GPIO_PIN_13     /* TIMER0_CH0N */
#define VOLT_IY_PIN				GPIO_PIN_0
#define VOLT_VY_PIN				GPIO_PIN_1
#define VOLT_IL_PIN				GPIO_PIN_2
#define VOLT_VX_PIN				GPIO_PIN_3
#define VOLT_IX_PIN				GPIO_PIN_6

#define KEY1_PIN                        GPIO_PIN_1
#define KEY1_PORT                        GPIOB
#define KEY2_PIN                        GPIO_PIN_0
#define KEY2_PORT                        GPIOB
#define KEY3_PIN                        GPIO_PIN_7
#define KEY3_PORT                        GPIOA
#define KEY4_PIN                        GPIO_PIN_12
#define KEY4_PORT                        GPIOB

#define LED_PORT                  GPIOC
#define LED1_PIN                         GPIO_PIN_13
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK                    RCU_GPIOC
  
#define LED2_PIN                         GPIO_PIN_14
#define LED2_GPIO_PORT                   GPIOC
#define LED2_GPIO_CLK                    RCU_GPIOC
  
#define LED3_PIN                         GPIO_PIN_15
#define LED3_GPIO_PORT                   GPIOC
#define LED3_GPIO_CLK                    RCU_GPIOC

extern volatile uint16_t adc_value[5];

     
void peripheral_config(void);


#ifdef cplusplus
}
#endif

#endif /* GD32E10X_EVAL_H */
