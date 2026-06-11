#ifndef __RINGBUF_H_
#define __RINGBUF_H_

#include "stm32f3xx_hal.h" 
#include "main.h"
#include "usart.h"




#define u8 uint8_t

#define usize getRingBuffLenght()
#define code_c() initRingBuff()
#define udelete(x) deleteRingBuff(x)
#define u(x) read1BFromRingBuff(x)

#define STR_LENGTH 4
#define RINGBUFF_LEN	100     //定义最大接收字节数 20

typedef struct
{
    uint16_t Head;
    uint16_t Tail;
    uint16_t Lenght;
    uint8_t  Ring_data[RINGBUFF_LEN];
}RingBuff_t;

extern __IO uint8_t  usart_dma_tx_over; //串口发送完成标志位

/**
	打印到屏幕串口
*/
int myprintf(const char *format, ...);
void RingBuff_Init(void);
void writeRingBuff(uint8_t data);
void deleteRingBuff(uint16_t size);
uint16_t getRingBuffLenght(void);
uint8_t read1BFromRingBuff(uint16_t position);


extern RingBuff_t ringBuff;	//创建一个ringBuff的缓冲区
extern uint8_t RxBuff[1];

//extern volatile uint8_t  usart_dma_tx_over;

#endif
