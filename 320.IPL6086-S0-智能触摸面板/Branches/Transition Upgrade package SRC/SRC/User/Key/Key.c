/***************************Copyright BestFu 2016-03-09*************************
��  ����key.c
˵  ��������������
��  �룺Keil uVision V5.12.0.0
��  ����V1.2
��  д��Seven
��  �ڣ�2016-03-09
��  ��:	
		V1.1��
>> �̰���Ϊ�ͷ�ʱ��ִ�У�����ӳ�������Ӧ
		V1.2:
>> �����Ż����������߼�
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
KeyProp_t  gKeyCh[MAX_TOUCH_KEY];        				//���尴���Ķ�����Դ <gKeyCh[0]����>
KeyStatus_t gKeyStatus[MAX_TOUCH_KEY];					//���尴����״̬��Դ <gKeyStatus[0]����>
KeySta_t gKeyPrevState = {0,KEY_NONE};					//���尴�����߱����ֵ
u8 gPassword[4] = {0};									//������ʵ������洢����
u8 gPswFlag = 0u; 										//���ڼ�¼��������������ֵ

//Private Variable definition
static const u8  KeyMapTab[7] = {0,31,59,62,47,55,61};	//����ӳ��� ���16������,��λ��0����
static u8 InputPassword[4] = {0};						//�����û����������ֵ��������
static Queue_t  Queue;									//���尴�����ն���
static u8 PswBit = 0; 									//��¼�����������λ��

//Private function declaration
static void CheckUserPassWordInput(u8 keynum);
static void DeviceKeyBusyDispPicture(void);
static void DeviceKeyBusyInstruct(void);
static void DeviceKeyNoBusyInstruct(void);
static void DeviceKeyNoBusyDispPicture(void);

#if (COMPLEX_TIME_NUM_IMPORT_FILTER_EN > 0u)
ComplexTimeNumImportFilter_t ComplexImportFilter;		//�����˲����ṹ����
static unsigned int key_map[6] = {1,2,3,4,5,6};			//��Ч�İ���ֵ
static unsigned int key_cnt[6] = {1000,1000,1000,1000,1000,1000};
static void Key_FilterInit(void);
//static void DeviceKeyBusyInstruct(void);
#endif

/*******************************************************************************
**��    ���� Key_Init()
**��    �ܣ� ����ģ���ʼ��
**��    ���� void
**��    �أ� void
*******************************************************************************/
void Key_Init(void)
{
	WTC6106_Init();
    InitQueue(&Queue);
	Key_FilterInit();
}

/*******************************************************************************
**��    ���� void KeyTimer_Init(void)
**��    �ܣ� ������ʱ��ģ���ʼ��
**��    ���� void
**��    �أ� void
*******************************************************************************/
void KeyTimer_Init(void)
{
	u32 count = 0xffffffff;
	while((Get_TouchFlag())||(0 == count))
	{
		count--;
	} 
	TIMx_Init(TIM2 ,32, 6399);		//��ʼ���ж�Ƶ��5ms
}

#if (COMPLEX_TIME_NUM_IMPORT_FILTER_EN > 0u)
/*******************************************************************************
�� �� ��:	static void Key_FilterInit(void)
����˵��:	�����˲���ʼ��
��    ��:	��
*******************************************************************************/
static void Key_FilterInit(void)
{
	ComplexImportFilter.import_all_space_threshold	= 6000;	//��������������ģʽ������ʱ����
	ComplexImportFilter.import_count_threshold		= 13;	//�������������Ĵ���
	ComplexImportFilter.import_space_threshold		= 500;	//������Ч�İ���ʱ����
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
**��    ���� void KeyStateScan(void)
**��    �ܣ� ����״̬ɨ��
**��    ���� void
**��    �أ� void
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
**��    ���� Key_Scan()
**��    �ܣ� ״̬����ʽɨ������
**��    ���� *keyFlag    ������־    0-�ް���  1-�̰���    2-������ 
**��    �أ� ����ֵ
**˵    ���� �ú��� ÿ�� 5~20ms ����һ��
*******************************************************************************/
u8 Key_Scan(u8 *keyFlag,u8 keyAccuracy)
{
    u8 i;
    static u8 MidValidKey = NONE_KEY;       //��Ч�����м䱣��ֵ
	static u8  NewKey = NONE_KEY;
	static u8  OldKey = NONE_KEY;
	static u8  LstKey = NONE_KEY;
	static u16 LongKeyCount = 0;            //������������
	
    *keyFlag = 0;
    NewKey = WTC6106_ReadKey();
    //OLED_ShowNumber(0,1,NewKey,3,0);
    if(NewKey == OldKey)
    {
        if(NewKey==NONE_KEY)                                        //�ް��� �� �ͷŰ���    
        {   
            if((LstKey != NONE_KEY)&&(LongKeyCount<LONG_KEY_TIMES)) // �ͷŰ��� �� �ǳ�����
            {
                if(LongKeyCount > keyAccuracy)  					//���˳��̰��� �����ȿɵ�
                {
                    LstKey  = NONE_KEY;
                    LongKeyCount = 0;
                    *keyFlag =1;
                    return MidValidKey;                             //���ض̼��ͷ�
                }
            }
            else{                                                   
                LstKey = NONE_KEY;
                LongKeyCount = 0;
                return NONE_KEY;                    
            }
        }
        else if(NewKey==LstKey)                                     //��������
        {
            if(LongKeyCount++ == LONG_KEY_TIMES)
            {
                *keyFlag =2;
                return MidValidKey;                                 //���س���ȷ��
            }
        }
        else{                                                       //����
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
    else OldKey = NewKey;               //��������
    return NONE_KEY;
}

/*******************************************************************************
�� �� ��:	static void DeviceKeyBusyInstruct(void)
����˵��:	������æ������ʾ����
��    ��:	��
*******************************************************************************/
static void DeviceKeyBusyInstruct(void)
{
	Thread_Login(ONCEDELAY,0,10,DeviceKeyBusyDispPicture);
}

/*******************************************************************************
�� �� ��:	static void DeviceKeyBusyDispPicture(void)
����˵��:	���ڰ���������æ��ʾ
��    ��:	��
*******************************************************************************/
static void DeviceKeyBusyDispPicture(void)
{
	OLED_ClearScreen();
	OLED_ShowString(4,1,"ϵͳ��æ",0);
}

/*******************************************************************************
�� �� ��:	static void DeviceKeyNoBusyInstruct(void)
����˵��:	�����ɷ�æ����æ��ʾ����
��    ��:	��
*******************************************************************************/
static void DeviceKeyNoBusyInstruct(void)
{
	Thread_Login(ONCEDELAY,0,10,DeviceKeyNoBusyDispPicture);
}

/*******************************************************************************
�� �� ��:	static void DeviceKeyNoBusyDispPicture(void)
����˵��:	�����ɷ�æ����æ��ʾͼƬ��ʾ����
��    ��:	��
*******************************************************************************/
static void DeviceKeyNoBusyDispPicture(void)
{
	if(gPswFlag > 0u) 
	{
		OLED_ClearScreen();
		OLED_ShowString(1,0,"��������������",0);
		OLED_ShowString(4,1,"�������",0);
		OLED_ShowString(6,2,"----",0);
		PswBit = 0u;
	}
	else if(0u == gSleepFlag)
	{
		switch(gKeyPrevState.KeyType)
		{
			case KEY_SHORT:
			{
				if(SW_OFF == gKeyPrevState.KeyStatusVal[gKeyPrevState.KeyValue])   //״̬��Դ��
				{
					OLED_ShowHalfPicAt(4,PicTab[gKeyPrevState.KeyValue]);
				}
				else								//״̬��Դ��
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
**��    ���� Key_Handle()
**��    �ܣ� ��������
**��    ���� void 
**��    �أ� void
**˵    ���� �ú��� ÿ�� 5~20ms ����һ��
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
		if(gPswFlag > 0u)        //���뿪��
		{
			CheckUserPassWordInput(keyTemp.KeyValue);
		}
		else if(NONE_KEY != keyTemp.KeyValue)
		{
			/*����ϴ�״̬*/
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
					if(SW_OFF == gKeyStatus[keyTemp.KeyValue].Status)   //״̬��Դ��
					{
						gKeyStatus[keyTemp.KeyValue].Status = SW_ON;
						OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue]);
						PropEventFifo(1, (keyTemp.KeyValue + 6), SRCEVENT , SW_ON);
					}
					else								//״̬��Դ��
					{
						gKeyStatus[keyTemp.KeyValue].Status = SW_OFF;
						OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue + 32]);
						PropEventFifo(1, (keyTemp.KeyValue + 6), SRCEVENT , SW_OFF);
					}	
					gKeyPrevState.KeyStatusVal[keyTemp.KeyValue] = gKeyStatus[keyTemp.KeyValue].Status;
					gKeyCh[keyTemp.KeyValue].Status = KEY_SHORT; 
					PropEventFifo(1, keyTemp.KeyValue, SRCEVENT , KEY_SHORT);     //50ms          
					Upload();                                       //2.5s//�ϱ�
				}
				break;
				case KEY_LONG:
				{
					OLED_ShowHalfPicAt(4,PicTab[keyTemp.KeyValue + 16]);
					gKeyCh[keyTemp.KeyValue].Status = KEY_LONG;    
					PropEventFifo(1, keyTemp.KeyValue, SRCEVENT , KEY_LONG);        
					Upload();                                       //�ϱ�
				}
				break;
				default:break;
			}               
		}
    }
}

/*******************************************************************************
**��    ���� static void CheckUserPassWordInput(u8 keynum)
**��    �ܣ� �û����봦����
**��    ���� keynum���û�����İ���ֵ 
**��    �أ� void
**˵    ���� �ú��� ÿ�� 5~20ms ����һ��
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
			OLED_ShowString(2,0,"����������",0);
			OLED_ShowString(6,2,"----",0);
		}
		break;
		case 1:
		{
			if(keynum <= 6)      //123456������Ч
			{
				StandbyCountReset();
				if(gWukeUpForPassWord > 0u)
				{
					OLED_ShowString(1,0,"��������������",0);
					OLED_ShowString(4,1,"�������",0);
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
//                      OLED_ShowString(4,1,"��ӭʹ��",0);
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
						OLED_ShowString(1,0,"��������������",0);
						OLED_ShowString(4,1,"�������",0);
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
**��    ��:  Key_FirstScan()
**��    ��:  ��������ɨ��
**��    ��:  void
**��    ��:  �״���Ч����ֵ 
*******************************************************************************/
u8 Key_FirstScan(void)
{
    u8 i;
    u8 first_key = NONE_KEY;
    u8 temp_key  = NONE_KEY;
    first_key = WTC6106_ReadKey();
    delay_us(10);                         /**> ʱ�������������󴥷� **/
	
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
**��    ��:  void Save_CurrentKeyState(void)
**��    ��:  ���浱ǰ�İ���״̬
**��    ��:  ��
**��    ��:  void
********************************************************************************/
void Save_CurrentKeyState(void)
{
    WriteDataToEEPROM(ADDR_KEY_CURRENT_STATE,sizeof(KeySta_t),(u8*)&gKeyPrevState);
}

/*******************************************************************************
**��    ��:  static void Get_CurrentKeyState(void)
**��    ��:  ��ȡ�浱ǰ�İ���״̬
**��    ��:  ��
**��    ��:  void
********************************************************************************/
void Get_CurrentKeyState(pKeySta_t pKeyState)
{
    ReadDataFromEEPROM(ADDR_KEY_CURRENT_STATE,sizeof(KeySta_t),(u8*)pKeyState);
}
/***************************Copyright BestFu **********************************/
