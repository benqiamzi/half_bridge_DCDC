#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

#define Y_Base  8

#define X_Base_s   6
#define X_Base   8

#define Line0  0
#define Line1  16
#define Line2  32
#define Line3  48

#define Lines0  0
#define Lines1  8
#define Lines2  16
#define Lines3  24
#define Lines4  32
#define Lines5  40
#define Lines6  48
#define Lines7  56

/*中文字符字节宽度*/
#define OLED_CHN_CHAR_WIDTH			3		//UTF-8编码格式给3，GB2312编码格式给2

/*字模基本单元*/
typedef struct 
{
	char Index[OLED_CHN_CHAR_WIDTH + 1];	//汉字索引
	uint8_t Data[32];						//字模数据
} ChineseCell_t;

/*ASCII字模数据声明*/
extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F6x8[][6];

/*汉字字模数据声明*/
extern const ChineseCell_t OLED_CF16x16[];

/*图像数据声明*/
extern const uint8_t Diode[];
/*按照上面的格式，在这个位置加入新的图像数据声明*/
//...

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
