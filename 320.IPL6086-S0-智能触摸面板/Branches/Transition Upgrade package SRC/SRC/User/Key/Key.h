/***************************Copyright BestFu 2016-03-09*************************
文  件：key.h
说  明：按键处理函数头文件
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
#ifndef _KEY_H_
#define _KEY_H_

#include "BF_type.h"

//Macro declaration
#define MAX_TOUCH_KEY				(6+1)        	//通道0 保留
#define MAX_KEY_NUM					(MAX_TOUCH_KEY)	//定义面板支持的最大按键数
#define NONE_KEY					(0xff)			//定义无键的键值
#define LONG_KEY_TIMES				(250)			//长按键计数值 250 * 5ms = 1.25s
#define ADDR_KEY_CURRENT_STATE		(0x4008)		//保存按键状态的EEPROM地址

//Data structure definition
typedef enum        //动作资源枚举类型定义
{
    KEY_NONE  = 0,
    KEY_SHORT = 1,
    KEY_LONG  = 2
}KeyState_e;

typedef enum		//状态资源枚举类型定义
{
	SW_OFF = 0,
	SW_ON  = 1
}KeySWStatus_e;

typedef struct
{
    KeyState_e  Status;
}KeyProp_t;

typedef struct 
{
	KeySWStatus_e Status;
}KeyStatus_t;

typedef struct	//用于休眠时,记录当前各个按键的状态
{
    u8    KeyValue;
    KeyState_e    KeyType; 
	KeySWStatus_e    KeyStatusVal[MAX_TOUCH_KEY];//按键的状态值
}KeySta_t,*pKeySta_t;

//variable declaration
extern KeyProp_t gKeyCh[MAX_TOUCH_KEY];          //按键属性状态保存
extern KeyStatus_t gKeyStatus[MAX_TOUCH_KEY];
extern u8 gPassword[4];
extern u8 gPswFlag;
extern KeySta_t gKeyPrevState;

//fuction declaration
void Key_Init(void);                     //按键初始化
u8 Key_Scan(u8 *keyFlag,u8 keyAccuracy); //正常按键扫描
void Key_Handle(void);                   //普通按键处理
u8   Key_FirstScan(void);                //首次按键扫描
void Save_CurrentKeyState(void);
void Get_CurrentKeyState(pKeySta_t pKeyState);
u8 UserFlashDataCpy(u32 des_addr,u32 src_addr, u32 len);
void KeyStateScan(void);
void KeyTimer_Init(void);

#endif

/***************************Copyright BestFu **********************************/
