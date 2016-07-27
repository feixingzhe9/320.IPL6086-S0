/***************************Copyright BestFu 2016-03-09*************************
��  ����key.h
˵  ��������������ͷ�ļ�
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
#ifndef _KEY_H_
#define _KEY_H_

#include "BF_type.h"

//Macro declaration
#define MAX_TOUCH_KEY				(6+1)        	//ͨ��0 ����
#define MAX_KEY_NUM					(MAX_TOUCH_KEY)	//�������֧�ֵ���󰴼���
#define NONE_KEY					(0xff)			//�����޼��ļ�ֵ
#define LONG_KEY_TIMES				(250)			//����������ֵ 250 * 5ms = 1.25s
#define ADDR_KEY_CURRENT_STATE		(0x4008)		//���水��״̬��EEPROM��ַ

//Data structure definition
typedef enum        //������Դö�����Ͷ���
{
    KEY_NONE  = 0,
    KEY_SHORT = 1,
    KEY_LONG  = 2
}KeyState_e;

typedef enum		//״̬��Դö�����Ͷ���
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

typedef struct	//��������ʱ,��¼��ǰ����������״̬
{
    u8    KeyValue;
    KeyState_e    KeyType; 
	KeySWStatus_e    KeyStatusVal[MAX_TOUCH_KEY];//������״ֵ̬
}KeySta_t,*pKeySta_t;

//variable declaration
extern KeyProp_t gKeyCh[MAX_TOUCH_KEY];          //��������״̬����
extern KeyStatus_t gKeyStatus[MAX_TOUCH_KEY];
extern u8 gPassword[4];
extern u8 gPswFlag;
extern KeySta_t gKeyPrevState;

//fuction declaration
void Key_Init(void);                     //������ʼ��
u8 Key_Scan(u8 *keyFlag,u8 keyAccuracy); //��������ɨ��
void Key_Handle(void);                   //��ͨ��������
u8   Key_FirstScan(void);                //�״ΰ���ɨ��
void Save_CurrentKeyState(void);
void Get_CurrentKeyState(pKeySta_t pKeyState);
u8 UserFlashDataCpy(u32 des_addr,u32 src_addr, u32 len);
void KeyStateScan(void);
void KeyTimer_Init(void);

#endif

/***************************Copyright BestFu **********************************/
