#include "protection.h"

/*
** ===================================================================
**     Funtion Name :void VoutSwOVP(uint8_t flag,int32_t ovpValue)
**     Description :
**     输出过压保护，预先判定BUCK模式运行还是BOOST模式来决定用X端还是Y端的电压作为输出过压保护的变量
**     当电源检测输出电压大于一定阈值，且在一定时间内保持该条件，则判定输出为过压；										
**     过压容易发生器件超压击穿风险，不建议电源自动恢复重启
**     Parameters  : 
**     flag：运行模式 BUCK模式或者BOOST模式；
**     ovpValu：过压保护值数字量
**     Returns     : none
** ===================================================================
*/
CCMRAM uint8_t VoutSwOVP(uint8_t run_flag,int32_t ovpValue, float realValue)
{
	static  int32_t  Cnt=0;//过压判据保持时间计数器
	//static 	int32_t	 vo=0;//输出电压变量

	if (realValue > ovpValue)//当输出电压大于*V；
	{
		Cnt++;//过压条件保持计时
		
		if(Cnt > 102)//过压条件保持*秒，则认为过压发生
		{
			Cnt = 0;//过压判据保持时间计数器清零
			
			//---------------------处理代码
			//HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TD1|HRTIM_OUTPUT_TD2);//禁止PWM输出
			
			

//			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TD1|HRTIM_OUTPUT_TD2);//禁止PWM输出
//			DF.PWMENFlag=0;//关闭PWM
//			setRegBits(DF.ErrFlag,F_SW_VOUT_OVP);//DF.ErrFlag中对应故障类型的标志位置1
//			DF.SMFlag  =Err;//主状态机跳转至故障状态
			
			
			
			return 2;
		}
		return 1;
	}
	else//过压条件不满足，
		Cnt  = 0;//计数器一直复位清0
	return 0;
}

/*
** ===================================================================
**     Funtion Name :CCMRAM void VinSwOVP(uint8_t flag,int32_t ovpValue)
**     Description :
**     输入过压保护
**     预先判定BUCK模式运行还是BOOST模式来决定用X端还是Y端的电压作为输入欠压保护的变量
**     当电源检测输入电压大于一定阈值，且在一定时间内保持该条件，则判定输入过压；
**     当发生输入过压后，电源关闭输出，输入过压标志位置位；
**     当发生输入过压后，电源检测输入电压小于一定阈值，且在一定时间内保持该条件，则电源恢复正常运行；
**     这里过压保护的阈值，和恢复的电压阈值有一定电压差，避免供电电源的负载效应电压下降的特性
**     Parameters  : none
**     flag：运行模式 BUCK模式或者BOOST模式；
**     ovpValue：过压保护值数字量
**     Returns     : none
** ===================================================================
*/
CCMRAM uint8_t VinSwOVP(uint8_t run_flag,int32_t ovpValue, int32_t vinValue)
{
	static  int32_t  Cnt=0;//输入过压判据保持时间计数器
//	static  int32_t	 RSCnt=0;//输入过压恢复判据保持时间计数器
//	static 	int32_t	 vi=0;//输入电压变量
	
	if ((vinValue > ovpValue) && (run_flag == 1 ))//当输入电压大于*V；排除电源初始化时还没启动采样
	{
		Cnt++;//输入过压条件保持计时
		if(Cnt > 200)//输入过压保持*秒，则认为过输入过压
		{
			Cnt = 0;//输入过压判据保持时间计数器清零
			
			//处理代码
			
			
			
			return 2;
		}
		return 1;
	}
	else//输入过压条件不满足，
		Cnt  = 0;//计数器一直复位清0		

	return 0;	
}

/*
** ===================================================================
**     Funtion Name :VinSwUVP(uint8_t flag,int32_t uvpValue)
**     Description :
**     输入欠压保护
**     预先判定BUCK模式运行还是BOOST模式来决定用X端还是Y端的电压作为输入欠压保护的变量
**     当电源检测输入电压小于一定阈值，且在一定时间内保持该条件，则判定输入欠压；										
**     当发生输入欠压后，电源关闭输出，输入欠压标志位置位；								
**     当发生输入欠压后，电源检测输入电压大于一定阈值，且在一定时间内保持该条件，则电源恢复正常运行；									
**     这里欠压保护的阈值，和恢复的电压阈值有一定电压差，避免供电电源的负载效应电压下降的特性
**     Parameters  : none
**     flag：运行模式 BUCK模式或者BOOST模式；
**     uvpValue：欠压保护值数字量
**     Returns     : none
** ===================================================================
*/
CCMRAM uint8_t VinSwUVP(uint8_t run_flag,int32_t uvpValue, int32_t vinValue)
{
	static  int32_t  Cnt=0;//输入欠压判据保持时间计数器
	//static  int32_t	 RSCnt=0;//输入欠压恢复判据保持时间计数器
	//static 	int32_t	 vi=0;//输入电压变量	
	
	if ((vinValue < uvpValue) && (run_flag == 1 ))//当输入电压小于*V；排除电源初始化时还没启动采样
	{
		Cnt++;//输入欠压条件保持计时
		if(Cnt > 200)//输入欠压保持*秒，则认为过输入欠压
		{
			Cnt = 0;//输入欠压判据保持时间计数器清零
			
			
			
			//HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TD1|HRTIM_OUTPUT_TD2);//禁止PWM输出
			
			return 2;
		}
		return 1;
	}
	else//输入欠压条件不满足，
		Cnt  = 0;//计数器一直复位清0		
	
	return 0;


}

/*
** ===================================================================
**     Funtion Name :CCMRAM void SwOCP(uint8_t flag,int32_t ocpValue)
**     Description :
**     输出过流保护
**     预先判定BUCK模式运行还是BOOST模式来决定用X端还是Y端的电流作为输出过流保护的电流变量
**     当电源检测输出电流大于一定阈值，且在一定时间内保持该条件，则判定输出为过流；
**     当发生输出过流后，电源关闭输出，过流标志位置位；
**     电源关机后等待*秒后重新启动；
**     Parameters  : 
**     flag：运行模式 BUCK模式或者BOOST模式；
**     ocpValue：过流保护值数字量
**     Returns     : none
** ===================================================================
*/
CCMRAM uint8_t SwOCP(uint8_t run_flag,int32_t ocpValue, int32_t outValue)
{
	static  int32_t  Cnt=0;//过流判据保持时间计数器
	//static  int32_t  RSCnt=0;//过流故障恢复等待的时间计数器
	//static 	int32_t	 io=0;//输出电流变量

	if((outValue> ocpValue)&&(run_flag  == 1))//当输出电流大于*A；只有在电源运行过程中才检测输出是否过流
	{
		Cnt++;//过流条件保持计时
		if(Cnt>102)//过流条件保持*秒，则认为过流发生
		{
			Cnt = 0;//过流判据保持时间计数器清零
			
			
			return 2;
		}
		return 1;
	}
	else//过流条件不满足，
		Cnt  = 0;//计数器一直复位清0

	return 0;
}


/*
** ===================================================================
**     Funtion Name :void ShortOff(uint8_t flag,int32_t vValue,int32_t iValue)
**     Description :
**     输出短路保护
**     预先判定BUCK模式运行还是BOOST模式来决定用X端还是Y端的电流作为输出过流保护的电流变量
**     当电源检测输出电压小于一定阈值，电流大于一定阈值，则判定输出为短路；
**     当发生输出短路后，电源关闭输出，短路标志位置位；
**     电源关机后等待*秒后重新启动；
**     Parameters  : none
**     flag：运行模式 BUCK模式或者BOOST模式；
**     vValue：短路电压判据
**     iValue：短路电流判据
**     Returns     : none
** ===================================================================
*/
CCMRAM uint8_t ShortOff(uint8_t run_flag,int32_t vValue,int32_t iValue, float realVoltValue,float realCurrValue)
{
	//static int32_t RSCnt = 0;//短路故障恢复等待的时间计数器
	//static int32_t io=0,vo=0;//输出电压电流变量

/***需要注意的是，boost电路由于上管MOS Q1体二极管的处在，boost不具备短路保护功能***/
/***需要注意的是，boost电路由于上管MOS Q1体二极管的处在，boost不具备短路保护功能***/
/***需要注意的是，boost电路由于上管MOS Q1体二极管的处在，boost不具备短路保护功能***/
	if((realCurrValue> iValue)&&(realVoltValue <vValue))//当输出电压过小，电流过大时，则判定为输出短路
	{
		
		//HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TD1|HRTIM_OUTPUT_TD2);//禁止PWM输出
		
		return 2;
	}

	return 0;
}
