#ifndef __USB_PRINT_H
#define __USB_PRINT_H

#include "sys_bsp.h"

#define SEND_DATA 0x81
#define SEND_STATE 0x82
#define SEND_ASK 0x83

void send_param_data(void);


void cdc_send_data(const uint8_t *buf, uint32_t len);


#endif