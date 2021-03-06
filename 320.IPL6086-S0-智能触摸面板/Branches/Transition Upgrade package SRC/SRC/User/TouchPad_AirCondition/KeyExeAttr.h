/***************************Copyright BestFu ***********************************
**  文    件:   KeyExeAttr.h
**  功    能：  <<属性层>>摁键属性层接口
**  编    译：  Keil uVision5 V5.10
**  版    本：  V1.0.0
**  编    写：  Seven
**  创建日期：  2014/08/21
**  修改日期：  2014/08/21
**  说    明：
**  V1.0
    >> 摁键状态 : 0==无摁键   1==短摁键   2==长按键
*******************************************************************************/
#ifndef _KEY_EXE_ATTR_H_
#define _KEY_EXE_ATTR_H_

#include "BF_type.h"

extern const AttrExe_st KeyAttrTab[];

extern void  KeyInit(u8 unitID);
extern void  KeyEepromInit(u8 unitID);
extern void  KeyChecking_Attr(void);
extern MsgResult_t Get_TouchKey_Attr(UnitPara_t *pData, u8 *rlen, u8 *rpara);

MsgResult_t Set_Password_Attr(UnitPara_t *pData);
MsgResult_t Get_Password_Attr(UnitPara_t *pData, u8 *rLen, u8 *rData);
MsgResult_t Set_FlashCpy_Attr(UnitPara_t *pData);
MsgResult_t Get_TouchKeyStatus_Attr(UnitPara_t *pData, u8 *rLen, u8 *rData);
MsgResult_t Set_TouchKey_Attr(UnitPara_t *pData);

u8 Key_UnitAttrNum(void);
void KeyLowPowerHandle(void);
#endif

/***************************Copyright BestFu **********************************/
