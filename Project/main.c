/*!
    \file    main.c
    \brief   led spark with systick, USART print and key example
    
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

#include "gd32e10x.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"
#include "gd32e10x_eval.h"
#include "jkd_oled.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"

usb_core_driver cdc_acm;


/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/

int main(void)
{
    /* configure systick */
    systick_config();
    /* initilize the LEDs, USART and key */
	peripheral_config();
	
	usb_rcu_config();

    usb_timer_init();

    usbd_init(&cdc_acm, &cdc_desc, &cdc_class);

    usb_intr_config();
	
	
	OLED_Init();
	OLED_Clear();
	OLED_Printf(0,Line0,OLED_8X16,"hello");
	OLED_Update();
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
	
    while (1){
		
		
		printf("hello\n");
		delay_1ms(500);
		
    }
}

#ifdef GD_ECLIPSE_GCC
/* retarget the C library printf function to the USART, in Eclipse GCC environment */
int __io_putchar(int ch)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE));
    return ch;
}
#else
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
//	usb_txfifo_write(&cdc_acm.regs,(uint8_t*)&ch,1,1);
//	cdc_acm_data_send(&cdc_acm);
    return ch;
}
#endif /* GD_ECLIPSE_GCC */
