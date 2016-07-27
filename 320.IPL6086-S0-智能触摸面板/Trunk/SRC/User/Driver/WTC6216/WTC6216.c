/***************************Copyright BestFu ***********************************
**  文    件：  WTC6216.c
**  功    能：  <<驱动层>> WTC6216 十六通道电容触摸芯片
**  编    译：  Keil uVision5 V5.10
**  版    本：  V1.0
**  编    写：  Seven
**  创建日期：  2014.08.20
**  修改日期：  2014.08.20
**  说    明：  数据接口: INT_Flag线 + 四线BCD码
*******************************************************************************/
#include "WTC6216.h"
#include "LowPower.h"

/*******************************************************************************
**函    数:  WTC6216_Init()
**功    能:  WTC6216初始化
**参    数:  null
**返    回:  null
*******************************************************************************/
void WTC6106_Init(void)
{
    GPIOx_Cfg(PORT_WTC_TouchFlag, PIN_WTC_TouchFlag, IN_DOWN);
//    GPIOx_Cfg(PORT_WTC_DATA1    , DATA1|DATA2|DATA3, IN_FLOATING);
//    GPIOx_Cfg(PORT_WTC_DATA2    , DATA4|DATA5|DATA6, IN_FLOATING);
    GPIOx_Cfg(PORT_WTC_DATA1    , DATA1|DATA2|DATA3, IN_UP);
    GPIOx_Cfg(PORT_WTC_DATA2    , DATA4|DATA5|DATA6, IN_UP);
}

/*******************************************************************************
**函    数:  WTC6216_Read()
**功    能:  WTC6216读取摁键值
**参    数:  null
**返    回:  摁键值
             无摁键返回 0xff
*******************************************************************************/
u8 WTC6106_ReadKey(void)   
{
    if(0 != Get_TouchFlag())
    {
        return Get_TouchKey1()|Get_TouchKey2();
    }
    return 0xff;
}

/********************************* END FILE ***********************************/
