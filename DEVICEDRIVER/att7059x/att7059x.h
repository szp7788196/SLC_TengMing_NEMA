#ifndef __ATT7059X_H
#define __ATT7059X_H

#include "sys.h"


#define ACK_OK		0x54
#define ACK_FAIL	0x63



#define POWER_RATIO						0.01777947713f		//����ת��ϵ��
#define ELECTRIC_ENERGY_METER_CONSTANT	3200.0f				//���ܱ���
#define CURRENT_RATIO					0.01144472f			//����ת��ϵ��
#define VOLTAGE_RATIO					0.000184083851f		//��ѹת��ϵ��




u8 CalAtt7059CheckSum(u8 *buf, u8 len);
u8 Att7059xWriteOperate(u8 add, u16 data);
u8 Att7059xReadOperate(u8 add, u32 *data);

s32 Att7059xGetCurrent1ADCValue(void);
s32 Att7059xGetVoltageADCValue(void);
float Att7059xGetCurrent1(void);
float Att7059xGetVoltage(void);
float Att7059xGetVoltageFreq(void);
float Att7059xGetChannel1PowerP(void);
float Att7059xGetChannel1PowerQ(void);
float Att7059xGetChannel1PowerS(void);
float Att7059xGetEnergyP(void);
float Att7059xGetEnergyQ(void);
float Att7059xGetEnergyS(void);
s32 Att7059xGetMaxVoltageADCValue(void);


































#endif
