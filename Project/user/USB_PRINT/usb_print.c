#include "usb_print.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"
#include <string.h>

usb_core_driver cdc_acm;
uint8_t send_data[20]={
	0xAA,0x55
};


void cdc_send_data(const uint8_t *buf, uint32_t len)
{
    usb_cdc_handler *cdc = (usb_cdc_handler *)cdc_acm.dev.class_data[CDC_COM_INTERFACE];

    if ((cdc != NULL) && (len > 0) && (len <= USB_CDC_RX_LEN)) {
        if (cdc_acm_check_ready(&cdc_acm) == 0) {
            memcpy(cdc->data, buf, len);
            cdc->receive_length = len;
            cdc_acm_data_send(&cdc_acm);
        }
    }
}

void cdc_receive_data(void)
{

    
}

void send_param_data(void)
{
    memset()
    uint8_t i = 2;
    send_data[i] = 1+12+4;
    send_data[i+1] = SEND_DATA;

    uint16_t tmp = my_adc_sample.Vx_f32*1000;
    send_data[i+2] = tmp &0x00ff;
    send_data[i+3] = tmp >>8;

    tmp = my_adc_sample.Vy_f32*1000;
    send_data[i+4] = tmp &0x00ff;
    send_data[i+5] = tmp >>8;

    tmp = my_adc_sample.Ix_f32*1000;
    send_data[i+6] = tmp &0x00ff;
    send_data[i+7] = tmp >>8;

    tmp = my_adc_sample.Iy_f32*1000;
    send_data[i+8] = tmp &0x00ff;
    send_data[i+9] = tmp >>8;

    tmp = my_adc_sample.iL_f32*1000;
    send_data[i+10] = tmp &0x00ff;
    send_data[i+11] = tmp >>8;

    tmp = pwm_value.Q1Duty;
    send_data[i+12] = tmp &0x00ff;
    send_data[i+13] = tmp >>8;
    
    uint32_t arr[4];
    uint8_t j = 0;

    for(j = 0;j<4;j++)
    {
        arr[j] = send_data[j*4]<<24|send_data[j*4+1]<<16|send_data[j*4+2]<<8|send_data[j*4+3];
    }

    crc_data_register_reset();

    uint32_t crc_value = crc_block_data_calculate(arr, 4);

    send_data[i+14] = crc_value&0x000000ff;
    send_data[i+15] = (crc_value&(0x000000ff<<2))>>8;
    send_data[i+16] = (crc_value&(0x000000ff<<4))>>16;
    send_data[i+17] = (crc_value&(0x000000ff<<6))>>24;

    cdc_send_data(send_data, 20);

}

void send_state_data(void)
{
    memset()
    uint8_t i = 2;
    send_data[i] = 1+12+4;
    send_data[i+1] = SEND_DATA;

    uint16_t tmp = my_adc_sample.Vx_f32*1000;
    send_data[i+2] = tmp &0x00ff;
    send_data[i+3] = tmp >>8;


}
