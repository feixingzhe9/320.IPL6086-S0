/***************************Copyright BestFu 2016-03-09*************************
文  件：key.c
说  明：按键处理函数
编  译：Keil uVision V5.12.0.0
版  本：V1.2
编  写：Seven
日  期：2016-03-09
修  改:	
		V1.1：
>> 短按键为释放时才执行，并添加长按键响应
		V1.2:
>> 精简优化按键处理逻辑
*******************************************************************************/
#include "delay.h"
#include "LowPower.h"
#include "WTC6216.h"
#include "MsgPackage.h"
#include "ShakeMotor.h"
#include "OLED.h"
#include "ShowLED.h"
#include "TouchPadPic.h"
#include "Queue.h"
#include "sys.h"
#include "iwdg.h"
#include "EEPROM.h"
#include "ImportFilter.h"
#include "TIMx.h"
#include "Thread.h"
#include "usart.h"

//Public Variable definitions
KeyProp_t  gKeyCh[MAX_TOUCH_KEY];        				//定义按键的动作资源 <gKeyCh[0]保留>
KeyStatus_t gKeyStatus[MAX_TOUCH_KEY];					//定义按键的状态资源 <gKeyStatus[0]保留>
KeySta_t gKeyPrevState = {0,KEY_NONE};					//定义按键休眠保存键值
u8 gPassword[4] = {0};									//定义真实的密码存储数组
u8 gPswFlag = 0u; 										//用于记录密码存在与否属性值

//Private Variable definition
static const u8  KeyMapTab[7] = {0,31,59,62,47,55,61};	//按键映射表 最大16个按键,表位置0保留
static u8 InputPassword[4] = {0};						//定义用户输入的密码值缓冲数组
static Queue_t  Queue;									//定义按键接收队列
static u8 PswBit = 0; 									//记录密码已输入的位数

//Private function declaration
static void CheckUserPassWordInput(u8 keynum);
static void DeviceKeyBusyDispPicture(void);
static void DeviceKeyBusyInstruct(void);
static void DeviceKeyNoBusyInstruct(void);
static void DeviceKeyNoBusyDispPicture(void);

#if (COMPLEX_TIME_NUM_IMPORT_FILTER_EN > 0u)
ComplexTimeNumImportFilter_t ComplexImportFilter;		//定义滤波器结构函数
static unsigned int key_map[6] = {1,2,3,4,5,6};			//有效的按键值
static unsigned int key_cnt[6] = {1000,1000,1000,1000,1000,1000};
static void Key_FilterInit(void);
//static void DeviceKeyBusyInstruct(void);
#endif

/*******************************************************************************
**函    数： Key_Init()
**功    能： 按键模块初始化
**参    数： void
**返    回： void
*******************************************************************************/
void Key_Init(void)
{
	WTC6106_Init();
    InitQueue(&Queue);
	Key_FilterInit();
}

/*******************************************************************************
**函    数： void KeyTimer_Init(void)
**功    能： 按键定时器模块初始化
**参    数： void
**返    回： void
*******************************************************************************/
void KeyTimer_Init(void)
{
	u32 count = 0xffffffff;
	while((Get_TouchFlag())||(0 == count))
	{
		count--;
	} 
	TIMx_Init(TIM2 ,32, 6399);		//初始化中断频率5ms
}

#if (COMPLEX_TIME_NUM_IMPORT_FILTER_EN > 0u)
/*******************************************************************************
函 数 名:	static void Key_FilterInit(void)
功能说明:	按键滤波初始化
返    回:	无
*******************************************************************************/
static void Key_FilterInit(void)
{
	ComplexImportFilter.import_all_space_threshold	= 6000;	//连续操作至保护模式不可用时间间隔
	ComplexImportFilter.import_count_threshold		= 13;	//可以连续操作的次数
	ComplexImportFilter.import_space_threshold		= 500;	//两次有效的按键时间间隔
	ComplexImportFilter.import_map					= key_map;
	ComplexImportFilter.import_num					= 6;
	ComplexImportFilter.import_travel_cycle			= 5;
	ComplexImportFilter.import_unvalid_flag			= 0;
	ComplexImportFilter.import_unvalid_count		= 0;
	ComplexImportFilter.import_valid_keep_time		= 1500;
	ComplexImportFilter.import_valid_count			= 0;
	ComplexImportFilter.import_valid_space_ptr		= key_cnt;
}
#endif

/*******************************************************************************
**函    数： void KeyStateScan(void)
**功    能： 按键状态扫描
**参    数： void
**返    回： void
*******************************************************************************/
void KeyStateScan(void)
{
	KeyCurStatus_t keyTemp;
	
	keyTemp.KeyValue=Key_Scan((u8*)&keyTemp.KeyType , 0);
#if (COMPLEX_TIME_NUM_IMPORT_FILTER_EN > 0u)
	if(UNVALID_IMPORT_VALUE != ComplexTimeNumImportFilter(\
							&ComplexImportFilter,\
							(unsigned int)keyTemp.KeyValue,\
							DeviceKeyBusyInstruct,\
							DeviceKeyNoBusyInstruct))
#endif
	{	
		if((NONE_KEY != keyTemp.KeyValue))                 
		{                
			CPU_SR_ALLOC();
			cpu_sr = CPU_SR_Save();
			Enqueue(&Queue,keyTemp);
			CPU_SR_Restore(cpu_sr);
			ShowLED_FlashLED(keyTemp.KeyValue);
		}
	}
}

/*******************************************************************************
**函    数： Key_Scan()
**功    能： 状态机方式扫描摁键
**参    数： *keyFlag    按键标志    0-无按键  1-短按键    2-长按键 
**返    回： 摁键值
**说    明： 该函数 每隔 5~20ms 调用一次
*******************************************************************************/
u8 Key_Scan(u8 *keyFlag,u8 keyAccuracy)
{
    u8 i;
    static u8 MidValidKey = NONE_KEY;       //有效按键中间保存值
	static u8  NewKey = NONE_KEY;
	static u8  OldKey = NONE_KEY;
	static u8  LstKey = NONE_KEY;
	static u16 LongKeyCount = 0;            //长按键计数器
	
    *keyFlag = 0;
    NewKey = WTC6106_ReadKey();
    //OLED_ShowNumber(0,1,NewKey,3,0);
    if(NewKey == OldKey)
    {
        if(NewKey==NONE_KEY)                                        //无按键 或 释放按键    
        {   
            if((LstKey != NONE_KEY)&&(LongKeyCount<LONG_KEY_TIMES)) // 释放按键 且 非长按键
            {
                if(LongKeyCount > keyAccuracy)  					//过滤超短按键 灵敏度可调
                {
                    LstKey  = NONE_KEY;
                    LongKeyCount = 0;
                    *keyFlag =1;
                    return MidValidKey;                             //返回短键释放
                }
            }
            else{                                                   
                LstKey = NONE_KEY;
                LongKeyCount = 0;
                return NONE_KEY;                    
            }
        }
        else if(NewKey==LstKey)                                     //连续按键
        {
            if(LongKeyCount++ == LONG_KEY_TIMES)
            {
                *keyFlag =2;
                return MidValidKey;                                 //返回长键确认
            }
        }
        else{                                                       //单键
            LstKey = NewKey;
            for(i=1 ; i<=MAX_KEY_NUM ; i++)
            {
                if( KeyMapTab[i] == NewKey )  
                {
                    MidValidKey = i;
                    break;
                }
            }
            if( i > MAX_KEY_NUM)              MidValidKey = NONE_KEY;
            return NONE_KEY ;         
        }
    }
    else OldKey = NewKey;               //抖动处理
    return NONE_KEY;
}

/*******************************************************************************
函 数 名:	static void DeviceKeyBusyInstruct(void)
功能说明:	按键繁忙操作提示函数
返    回:	无
*******************************************************************************/
static void DeviceKeyBusyInstruct(void)
{
	Thread_Login(ONCEDELAY,0,10,DeviceKeyBusyDispPicture);
}

/*******************************************************************************
函 数 名:	static void DeviceKeyBusyDispPicture(void)
功能说明:	用于按键操作繁忙提示
返    回:	无
*******************************************************************************/
static void DeviceKeyBusyDispPicture(void)
{
	OLED_ClearScreen();
	OLED_ShowString(4,1,"系统繁忙",0);
}

/*******************************************************************************
函 数 名:	static void DeviceKeyNoBusyInstruct(void)
功能说明:	按键由繁忙到不忙提示函数
返    回:	无
*******************************************************************************/
static void DeviceKeyNoBusyInstruct(void)
{
	Thread_Login(ONCEDELAY,0,10,DeviceKeyNoBusyDispPicture);
}

/*******************************************************************************
函 数 名:	static void DeviceKeyNoBusyDispPicture(void)
功能说明:	按键由繁忙到不忙提示图片显示函数
返    回:	无
*******************************************************************************/
static void DeviceKeyNoBusyDispPicture(void)
{
	if(gPswFlag > 0u) 
	{
		OLED_ClearScreen();
		OLED_ShowString(1,0,"请重新输入密码",0);
		OLED_ShowString(4,1,"密码错误",0);
		OLED_ShowString(6,2,"----",0);
		PswBit = 0u;
	}
	else if(0u == gSleepFlag)
	{
		switch(gKeyPrevState.KeyType)
		{
			case KEY_SHORT:
			{
				if(SW_OFF == gKeyPrevState.KeyStatusVal[gKeyPrevState.KeyValue])   //状态资源关
				{
					OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue]);
				}
				else								//状态资源开
				{
					OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue + 32]);
				}	                               
			}
			break;
			case KEY_LONG:
			{
				OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue + 16]); 
			}
			break;
			default:break;
		} 
	}
}

/*******************************************************************************
**函    数： Key_Handle()
**功    能： 摁键处理
**参    数： void 
**返    回： void
**说    明： 该函数 每隔 5~20ms 调用一次
*******************************************************************************/
void Key_Handle(void)
{
    u8 i;	
    KeyCurStatus_t keyTemp;
    u8 result = 0x00;
	
    CPU_SR_ALLOC();
    cpu_sr = CPU_SR_Save();
    result = Dequeue(&Queue,&keyTemp);
    CPU_SR_Restore(cpu_sr);		

    if(result > 0u)
    {
		DEBUG("key = %d\r\n",keyTemp.KeyValue);
		if(gPswFlag > 0u)        //密码开启
		{
			CheckUserPassWordInput(keyTemp.KeyValue);
		}
		else if(NONE_KEY != keyTemp.KeyValue)
		{
			/*清除上次状态*/
			for(i = 0;i< MAX_TOUCH_KEY;i ++)
			{
				gKeyCh[i].Status = KEY_NONE; 
			} 
			gSleepFlag = 0;               
			gKeyPrevState.KeyType = keyTemp.KeyType;
			gKeyPrevState.KeyValue= keyTemp.KeyValue;

			StandbyCountReset();
			
			switch(keyTemp.KeyType)
			{
				case KEY_SHORT:
				{
					if(SW_OFF == gKeyStatus[keyTemp.KeyValue].Status)   //状态资源关
					{
						gKeyStatus[keyTemp.KeyValue].Status = SW_ON;
						OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue]);
						PropEventFifo(1, (keyTemp.KeyValue + 6), SRCEVENT , SW_ON);
					}
					else								//状态资源开
					{
						gKeyStatus[keyTemp.KeyValue].Status = SW_OFF;
						OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue + 32]);
						PropEventFifo(1, (keyTemp.KeyValue + 6), SRCEVENT , SW_OFF);
					}	
					gKeyPrevState.KeyStatusVal[keyTemp.KeyValue] = gKeyStatus[keyTemp.KeyValue].Status;
					gKeyCh[keyTemp.KeyValue].Status = KEY_SHORT; 
					PropEventFifo(1, keyTemp.KeyValue, SRCEVENT , KEY_SHORT);     //50ms          
					Upload();                                       //2.5s//上报
				}
				break;
				case KEY_LONG:
				{
					OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue + 16]);
					gKeyCh[keyTemp.KeyValue].Status = KEY_LONG;    
					PropEventFifo(1, keyTemp.KeyValue, SRCEVENT , KEY_LONG);        
					Upload();                                       //上报
				}
				break;
				default:break;
			}               
		}
    }
}

/*******************************************************************************
**函    数： static void CheckUserPassWordInput(u8 keynum)
**功    能： 用户密码处理函数
**参    数： keynum：用户输入的按键值 
**返    回： void
**说    明： 该函数 每隔 5~20ms 调用一次
*******************************************************************************/
static void CheckUserPassWordInput(u8 keynum)
{
	static u8 FirstPassWord = 0u;  
	
	switch(FirstPassWord)
	{
		case 0:
		{
			FirstPassWord = 1;
			OLED_ClearScreen();
			OLED_ShowString(2,0,"请输入密码",0);
			OLED_ShowString(6,2,"----",0);
		}
		break;
		case 1:
		{
			if(keynum <= 6)      //123456按键有效
			{
				StandbyCountReset();
				if(gWukeUpForPassWord > 0u)
				{
					OLED_ShowString(1,0,"请重新输入密码",0);
					OLED_ShowString(4,1,"密码错误",0);
					OLED_ShowString(6,2,"----",0);
					gWukeUpForPassWord = 0;
					PswBit = 0;
				}
				OLED_ShowChar(6+PswBit,2,keynum + '0',0);
				InputPassword[PswBit] = keynum;
				if(PswBit++ >= 3)
				{
					PswBit=0;
					if((gPassword[0] == InputPassword[0])&& \
					   (gPassword[1] == InputPassword[1])&& \
					   (gPassword[2] == InputPassword[2])&& \
					   (gPassword[3] == InputPassword[3]) )
					{
						gPswFlag = 0;
						OLED_ClearScreen();
//                      OLED_ShowString(4,1,"欢迎使用",0);
						gSleepFlag = 0; 
						if(KEY_SHORT == gKeyPrevState.KeyType)
						{
							OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue]);
						}
						else 
						{
							OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue + 16]);
						}
					}
					else 
					{
						OLED_ShowString(1,0,"请重新输入密码",0);
						OLED_ShowString(4,1,"密码错误",0);
						OLED_ShowString(6,2,"----",0);
					}
				}
			}
		}
		break;
		default:break;
	}
}

/*******************************************************************************
**函    数:  Key_FirstScan()
**功    能:  按键唤醒扫描
**参    数:  void
**返    回:  首次有效按键值 
*******************************************************************************/
u8 Key_FirstScan(void)
{
    u8 i;
    u8 first_key = NONE_KEY;
    u8 temp_key  = NONE_KEY;
    first_key = WTC6106_ReadKey();
    delay_us(10);                         /**> 时间过长，会造成误触发 **/
	
    if(first_key == WTC6106_ReadKey())
    {
        for(i=1 ; i<=MAX_KEY_NUM ; i++)
        {
            if( KeyMapTab[i] == first_key )  
            {
                temp_key = i;
                break;
            }
        }
        if( i > MAX_KEY_NUM)              
			temp_key = NONE_KEY;           
    }
    return temp_key;
}

/*******************************************************************************
**函    数:  void Save_CurrentKeyState(void)
**功    能:  保存当前的按键状态
**参    数:  无
**返    回:  void
********************************************************************************/
void Save_CurrentKeyState(void)
{
    WriteDataToEEPROM(ADDR_KEY_CURRENT_STATE,sizeof(KeySta_t),(u8*)&gKeyPrevState);
}

/*******************************************************************************
**函    数:  static void Get_CurrentKeyState(void)
**功    能:  获取存当前的按键状态
**参    数:  无
**返    回:  void
********************************************************************************/
void Get_CurrentKeyState(pKeySta_t pKeyState)
{
    ReadDataFromEEPROM(ADDR_KEY_CURRENT_STATE,sizeof(KeySta_t),(u8*)pKeyState);
}
/***************************Copyright BestFu **********************************/
