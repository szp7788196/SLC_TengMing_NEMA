#ifndef __INVENTR_H
#define __INVENTR_H	 
#include "sys.h"
#include "common.h"


#define RELAY_ON	GPIO_SetBits(GPIOB,GPIO_Pin_9);
#define RELAY_OFF	GPIO_ResetBits(GPIOB,GPIO_Pin_9);


extern u8 InventrBusy;
extern u8 InventrDisable;

extern float InventrInPutCurrent;
extern float InventrInPutVoltage;
extern float InventrOutPutCurrent;
extern float InventrOutPutVoltage;


void RELAY_Init(void);

u8 InventrSetMaxPowerCurrent(u8 percent); 
u8 InventrSetLightLevel(u8 level);
float InventrGetOutPutCurrent(void);
float InventrGetOutPutVoltage(void);
u8 InventrGetDeviceInfo(void);

void SetLightLevel(RemoteControl_S control);





































#endif
