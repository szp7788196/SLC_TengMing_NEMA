#include "task_sensor.h"
#include "delay.h"
#include "sht2x.h"
#include "bh1750.h"
#include "task_net.h"
#include "common.h"
#include "inventr.h"
#include "rtc.h"
#include "usart.h"
#include "att7059x.h"
#include "internal.h"
#include "24cxx.h"

float InputCurrent = 0;
float InputVoltage = 0;
float InputFreq = 0.0f;
float InputPowerP = 0.0f;
float InputPowerQ = 0.0f;
float InputPowerS = 0.0f;
float InputEnergyP = 0.0f;
float InputEnergyQ = 0.0f;
float InputEnergyS = 0.0f;
float PowerFactor = 0.0f;

TaskHandle_t xHandleTaskSENSOR = NULL;

SensorMsg_S *p_tSensorMsg = NULL;	//用于装在传感器数据的结构体变量
unsigned portBASE_TYPE SENSOR_Satck;
void vTaskSENSOR(void *pvParameters)
{
	u8 mark1 = 0;
	u8 mark2 = 0;
	u8 mark3 = 0;
	u8 mark4 = 0;

	p_tSensorMsg = (SensorMsg_S *)mymalloc(sizeof(SensorMsg_S));

	while(1)
	{
//		InventrOutPutCurrent = InventrGetOutPutCurrent();	//读取电源输出电流
//		delay_ms(500);
//		InventrOutPutVoltage = InventrGetOutPutVoltage();	//读取电源输出电压
//		delay_ms(300);
		InputCurrent 	= Att7059xGetCurrent1();
		delay_ms(300);
		InputVoltage 	= Att7059xGetVoltage();
		delay_ms(300);
//		InputFreq 		= Att7059xGetVoltageFreq();
//		delay_ms(300);
		InputPowerP 	= Att7059xGetChannel1PowerP();
		delay_ms(300);
//		InputPowerQ 	= Att7059xGetChannel1PowerQ();
//		delay_ms(300);
		InputPowerS 	= Att7059xGetChannel1PowerS();
		delay_ms(300);
//		InputEnergyP 	= Att7059xGetEnergyP();
//		delay_ms(300);
//		InputEnergyQ 	= Att7059xGetEnergyQ();
//		delay_ms(300);
//		InputEnergyS 	= Att7059xGetEnergyS();
//		delay_ms(300);

		DeviceElectricPara.initial_brightness = LampsRunMode.run_mode[0].initial_brightness;
		DeviceElectricPara.switch_state = CurrentControl.control_type;
		DeviceElectricPara.brightness = CurrentControl.brightness;
		DeviceElectricPara.run_state = 0;

		InputPowerP = abs(InputPowerP);
			
		if(InputPowerP > 0 && InputPowerS > 0)
		{
			PowerFactor = (InputPowerP / InputPowerS) * 1000.0f;
		}
		
#ifdef SHOW_VERSION
		if(CurrentControl.control_type == 1 ||
	      (CurrentControl.control_type == 2 && CurrentControl.brightness == 0))		//关灯
		{
			InputPowerP = 0;
			InputPowerS = 0;
			PowerFactor = 0;
		}
#endif

		//电压
		DeviceElectricPara.volatge[0] = (((((u16)(InputVoltage * 10)) / 10) % 10) << 4) + ((((u16)(InputVoltage * 10)) % 10) & 0x0F);
		DeviceElectricPara.volatge[1] = ((((u16)(InputVoltage * 10)) / 1000) << 4) + (((((u16)(InputVoltage * 10)) / 100) % 10) & 0x0F);

		//电流
		if(InputCurrent < 0)
		{
			mark1 = 1;

			InputCurrent = abs(InputCurrent);
		}

		DeviceElectricPara.current[0] = (((u16)(InputCurrent / 10) % 10) << 4) + (((u16)InputCurrent % 10) & 0x0F);
		DeviceElectricPara.current[1] = (((u16)(InputCurrent / 1000) % 10) << 4) + (((u16)(InputCurrent / 100) % 10) & 0x0F);
		DeviceElectricPara.current[2] = (((u16)(InputCurrent * 1000)/ 100000) << 4) + ((((u16)(InputCurrent) / 10000) % 10) & 0x0F);

		if(mark1 == 0)
		{
			DeviceElectricPara.current[2] &= ~0x80;
		}
		else
		{
			DeviceElectricPara.current[2] |= 0x80;
		}

		//有功功率
		if(InputPowerP < 1)
		{
			InputPowerP = 0;
		}

		if(InputPowerP > 0 && InputPowerP < 1000)
		{
			DeviceElectricPara.active_power[0] = (((((u16)InputPowerP) / 10) % 10) << 4) + ((((u16)InputPowerP) % 10) & 0x0F);
			DeviceElectricPara.active_power[1] = ((((u16)InputPowerP) / 100) & 0x0F);
			DeviceElectricPara.active_power[1] |= 0x80;
		}
		else
		{
			DeviceElectricPara.active_power[0] = (((((u16)InputPowerP / 10) / 10) % 10) << 4) + ((((u16)InputPowerP / 10) % 10) & 0x0F);
			DeviceElectricPara.active_power[1] = ((((u16)InputPowerP / 10) / 100) & 0x0F);
			DeviceElectricPara.active_power[1] |= 0x60;
		}

		//功率因数
		DeviceElectricPara.pf[0] = (((((u16)PowerFactor) / 10) % 10) << 4) + ((((u16)PowerFactor) % 10) & 0x0F);
		DeviceElectricPara.pf[1] = ((((u16)PowerFactor) / 1000) << 4) + (((((u16)PowerFactor) / 100) % 10) & 0x0F);

		//漏电流
		DeviceElectricPara.leakage_current[0] = 0;
		DeviceElectricPara.leakage_current[1] = 0;
		DeviceElectricPara.leakage_current[2] = 0;

		//光照阈值
		DeviceElectricPara.Illuminance[0] = 0;
		DeviceElectricPara.Illuminance[1] = 0;

		//时间戳
		memcpy(DeviceElectricPara.time_stamp,CalendarClock,6);

		//当前通信质量
		NB_ModulePara.csq = BcxxCsq;

		//频带
		NB_ModulePara.band = BcxxBand;

		//PCI
		if(BcxxPci < 0)
		{
			mark2 = 1;

			BcxxPci = abs(BcxxPci);
		}

		if(BcxxPci >= 1 && BcxxPci < 1000)
		{
			NB_ModulePara.pci[0] = (((((u16)BcxxPci) / 10) % 10) << 4) + ((((u16)BcxxPci) % 10) & 0x0F);
			NB_ModulePara.pci[1] = ((((u16)BcxxPci) / 100) & 0x0F);
			NB_ModulePara.pci[1] |= 0x80;
		}
		else if(BcxxPci >= 1000 && BcxxPci < 10000)
		{
			NB_ModulePara.pci[0] = (((((u16)BcxxPci / 10) / 10) % 10) << 4) + ((((u16)BcxxPci / 10) % 10) & 0x0F);
			NB_ModulePara.pci[1] = ((((u16)BcxxPci / 10) / 100) & 0x0F);
			NB_ModulePara.pci[1] |= 0x60;
		}
		else if(BcxxPci >= 10000 && BcxxPci <= 32767)
		{
			NB_ModulePara.pci[0] = (((((u16)BcxxPci / 100) / 10) % 10) << 4) + ((((u16)BcxxPci / 100) % 10) & 0x0F);
			NB_ModulePara.pci[1] = ((((u16)BcxxPci / 100) / 100) & 0x0F);
			NB_ModulePara.pci[1] |= 0x20;
		}

		if(mark2 == 0)
		{
			NB_ModulePara.pci[1] &= ~0x10;
		}
		else
		{
			NB_ModulePara.pci[1] |= 0x10;
		}

		//RSRP
		if(BcxxRsrp < 0)
		{
			mark3 = 1;

			BcxxRsrp = abs(BcxxRsrp);
		}

		if(BcxxRsrp >= 1 && BcxxRsrp < 1000)
		{
			NB_ModulePara.rsrp[0] = (((((u16)BcxxRsrp) / 10) % 10) << 4) + ((((u16)BcxxRsrp) % 10) & 0x0F);
			NB_ModulePara.rsrp[1] = ((((u16)BcxxRsrp) / 100) & 0x0F);
			NB_ModulePara.rsrp[1] |= 0x80;
		}
		else if(BcxxRsrp >= 1000 && BcxxRsrp < 10000)
		{
			NB_ModulePara.rsrp[0] = (((((u16)BcxxRsrp / 10) / 10) % 10) << 4) + ((((u16)BcxxRsrp / 10) % 10) & 0x0F);
			NB_ModulePara.rsrp[1] = ((((u16)BcxxRsrp / 10) / 100) & 0x0F);
			NB_ModulePara.rsrp[1] |= 0x60;
		}
		else if(BcxxRsrp >= 10000 && BcxxRsrp <= 32767)
		{
			NB_ModulePara.rsrp[0] = (((((u16)BcxxRsrp / 100) / 10) % 10) << 4) + ((((u16)BcxxRsrp / 100) % 10) & 0x0F);
			NB_ModulePara.rsrp[1] = ((((u16)BcxxRsrp / 100) / 100) & 0x0F);
			NB_ModulePara.rsrp[1] |= 0x20;
		}

		if(mark3 == 0)
		{
			NB_ModulePara.rsrp[1] &= ~0x10;
		}
		else
		{
			NB_ModulePara.rsrp[1] |= 0x10;
		}

		//RSRP
		if(BcxxSnr < 0)
		{
			mark4 = 1;

			BcxxSnr = abs(BcxxSnr);
		}

		if(BcxxSnr >= 1 && BcxxSnr < 1000)
		{
			NB_ModulePara.snr[0] = (((((u16)BcxxSnr) / 10) % 10) << 4) + ((((u16)BcxxSnr) % 10) & 0x0F);
			NB_ModulePara.snr[1] = ((((u16)BcxxSnr) / 100) & 0x0F);
			NB_ModulePara.snr[1] |= 0x80;
		}
		else if(BcxxRsrp >= 1000 && BcxxRsrp < 10000)
		{
			NB_ModulePara.snr[0] = (((((u16)BcxxSnr / 10) / 10) % 10) << 4) + ((((u16)BcxxSnr / 10) % 10) & 0x0F);
			NB_ModulePara.snr[1] = ((((u16)BcxxSnr / 10) / 100) & 0x0F);
			NB_ModulePara.snr[1] |= 0x60;
		}
		else if(BcxxSnr >= 10000 && BcxxSnr <= 32767)
		{
			NB_ModulePara.snr[0] = (((((u16)BcxxSnr / 100) / 10) % 10) << 4) + ((((u16)BcxxSnr / 100) % 10) & 0x0F);
			NB_ModulePara.snr[1] = ((((u16)BcxxSnr / 100) / 100) & 0x0F);
			NB_ModulePara.snr[1] |= 0x20;
		}

		if(mark4 == 0)
		{
			NB_ModulePara.snr[1] &= ~0x10;
		}
		else
		{
			NB_ModulePara.snr[1] |= 0x10;
		}
		
#ifdef TENGMING
		//cell id
		memcpy(NB_ModulePara.cell_id,&BcxxCellID,4);
		
		//earfcn
		if(BcxxEARFCN >= 1 && BcxxEARFCN < 1000)
		{
			NB_ModulePara.earfcn[0] = (((((u16)BcxxEARFCN) / 10) % 10) << 4) + ((((u16)BcxxEARFCN) % 10) & 0x0F);
			NB_ModulePara.earfcn[1] = ((((u16)BcxxEARFCN) / 100) & 0x0F);
			NB_ModulePara.earfcn[1] |= 0x80;
		}
		else if(BcxxEARFCN >= 1000 && BcxxEARFCN < 10000)
		{
			NB_ModulePara.earfcn[0] = (((((u16)BcxxEARFCN / 10) / 10) % 10) << 4) + ((((u16)BcxxEARFCN / 10) % 10) & 0x0F);
			NB_ModulePara.earfcn[1] = ((((u16)BcxxEARFCN / 10) / 100) & 0x0F);
			NB_ModulePara.earfcn[1] |= 0x60;
		}
		else if(BcxxEARFCN >= 10000 && BcxxEARFCN <= 32767)
		{
			NB_ModulePara.earfcn[0] = (((((u16)BcxxEARFCN / 100) / 10) % 10) << 4) + ((((u16)BcxxEARFCN / 100) % 10) & 0x0F);
			NB_ModulePara.earfcn[1] = ((((u16)BcxxEARFCN / 100) / 100) & 0x0F);
			NB_ModulePara.earfcn[1] |= 0x20;
		}
		
		NB_ModulePara.earfcn[1] &= ~0x10;
#endif
		
		delay_ms(1000);

//		SENSOR_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}


















































