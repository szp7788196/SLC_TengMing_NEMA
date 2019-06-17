#include "event.h"
#include "24cxx.h"
#include "task_sensor.h"

//记录并存储发生的事件
void RecordEventsECx(u8 ecx,u8 len,u8 *msg)
{
	u16 crc_cal = 0;
	u16 add_pos = 0;
	u8 buf[24];

	xSemaphoreTake(xMutex_EVENT_RECORD, portMAX_DELAY);

	add_pos = E_IMPORTEAT_ADD;

	memset(buf,0,24);

	buf[0] = ecx;
	buf[1] = len + 6;

	memcpy(buf + 2,CalendarClock,6);
	memcpy(buf + 8,msg,len);

	crc_cal = CRC16(buf,22,1);

	buf[22] = (u8)(crc_cal >> 8);
	buf[23] = (u8)(crc_cal & 0x00FF);

	EventRecordList.lable1[EventRecordList.ec1] = ecx;

	crc_cal = CRC16(EventRecordList.lable1,256,1);

	AT24CXX_WriteOneByte(EC1_LABLE_ADD + EventRecordList.ec1,EventRecordList.lable1[EventRecordList.ec1]);
	AT24CXX_WriteOneByte(EC1_LABLE_ADD + 256,(u8)(crc_cal >> 8));
	AT24CXX_WriteOneByte(EC1_LABLE_ADD + 257,(u8)(crc_cal & 0x00FF));

	WriteDataFromMemoryToEeprom(buf,add_pos + EventRecordList.ec1 * EVENT_LEN, EVENT_LEN);

	EventRecordList.ec1 ++;

	WriteDataFromMemoryToEeprom(&EventRecordList.ec1,EC1_ADD, 1);

	xSemaphoreGive(xMutex_EVENT_RECORD);
}

//检测正常开灯事件
void CheckEventsEC15(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static u8 cnt = 0;
	u8 buf[6];

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)
		{
			cnt ++;

			if(cnt >= 10)	//等待一段时间，计算电流和电压
			{
				cnt = 0;

				if(ctrl.control_type == 1 ||
				  (ctrl.control_type == 2 && ctrl.brightness == 0))			//上个状态为关灯
				{
					if(ctrl.control_type == 0 ||
					  (ctrl.control_type == 2 && ctrl.brightness >= 1))		//现在状态为开灯
					{
						memset(buf,0,22);

						if(InputCurrent <= SWITCH_ON_MIN_CURRENT)			//开灯异常
						{
							buf[0] = 0x00;		//开灯成功数
							buf[1] = 0x00;
							buf[2] = 0x01;		//开灯失败数
							buf[3] = 0x00;
							buf[4] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
							buf[5] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
						}
						else												//开灯正常
						{
							buf[0] = 0x01;		//开灯成功数
							buf[1] = 0x00;
							buf[2] = 0x00;		//开灯失败数
							buf[3] = 0x00;
							buf[4] = 0x00;		//失败灯号
							buf[5] = 0x00;
						}
					}

					RecordEventsECx(EVENT_ERC15,6,buf);
				}

				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
	}
}

//检测异常开灯事件
void CheckEventsEC16(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static u8 cnt = 0;
	u8 buf[6];

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)
		{
			cnt ++;

			if(cnt >= 10)	//等待一段时间，计算电流和电压
			{
				cnt = 0;

				if(ctrl.control_type == 1 ||
				  (ctrl.control_type == 2 && ctrl.brightness == 0))			//上个状态为开灯
				{
					if(ctrl.control_type == 1 ||
					  (ctrl.control_type == 2 && ctrl.brightness > 0))		//现在状态为关灯
					{
						memset(buf,0,22);

						if(InputCurrent > SWITCH_OFF_MAX_CURRENT)			//关灯异常
						{
							buf[0] = 0x00;		//开灯成功数
							buf[1] = 0x00;
							buf[2] = 0x01;		//开灯失败数
							buf[3] = 0x00;
							buf[4] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
							buf[5] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
						}
						else												//关灯正常
						{
							buf[0] = 0x01;		//开灯成功数
							buf[1] = 0x00;
							buf[2] = 0x00;		//开灯失败数
							buf[3] = 0x00;
							buf[4] = 0x00;		//失败灯号
							buf[5] = 0x00;
						}
					}

					RecordEventsECx(EVENT_ERC16,6,buf);
				}

				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
	}
}

//单灯异常开灯
void CheckEventsEC17(RemoteControl_S ctrl)
{
	static s8 cnt = 0;
	static u8 occur = 0;
	u8 buf[4];

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(ctrl.control_type == 1 ||
		  (ctrl.control_type == 2 && ctrl.brightness == 0))			//上个状态为关灯
		{
			if(occur == 0)
			{
				if(InputCurrent > SWITCH_OFF_MAX_CURRENT)				//异常开灯
				{
					if(cnt < 10)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > -10)
					{
						cnt --;
					}
				}
			}
		}
		else
		{
			cnt = 0;

			occur = 0;
		}

		if(cnt >= 10)
		{
			cnt = 0;

			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x00;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;
		}
		else if(cnt <= -10)
		{
			cnt = 0;

			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x00;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;
		}

		if(occur == 1)
		{
			RecordEventsECx(EVENT_ERC17,4,buf);
		}
	}
}

//单灯异常关灯
void CheckEventsEC18(RemoteControl_S ctrl)
{
	static s8 cnt = 0;
	static u8 occur = 0;
	u8 buf[4];

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(ctrl.control_type == 0 ||
		  (ctrl.control_type == 2 && ctrl.brightness >= 1))		//现在状态为开灯
		{
			if(occur == 0)
			{
				if(InputCurrent <= SWITCH_ON_MIN_CURRENT)		//异常开灯
				{
					if(cnt < 10)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > -10)
					{
						cnt --;
					}
				}
			}
		}
		else
		{
			cnt = 0;

			occur = 0;
		}

		if(cnt >= 10)
		{
			cnt = 0;

			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x00;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;
		}
		else if(cnt <= -10)
		{
			cnt = 0;

			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x00;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;
		}

		if(occur == 1)
		{
			RecordEventsECx(EVENT_ERC18,4,buf);
		}
	}
}

//校时结果事件记录
void CheckEventsEC28(u8 *cal1,u8 *cal2)
{
	u8 buf[13];
	
	memset(buf,0,13);
	
	buf[0] = 0x00;
	
	memcpy(&buf[1],cal1,6);
	memcpy(&buf[7],cal2,6);
	
	RecordEventsECx(EVENT_ERC28,13,buf);
}

//升级结果事件记录
void CheckEventsEC51(u8 result,u8 *version)
{
	u8 buf[9];
	
	memset(buf,0,9);
	
	buf[0] = result;
	
	memcpy(&buf[1],version,8);
	
	RecordEventsECx(EVENT_ERC51,9,buf);
}

//单灯状态变化记录
void CheckEventsEC52(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	u8 buf[14];
	u8 cnt = 0;
	
	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)
		{
			cnt ++;

			if(cnt >= 10)	//等待一段时间，计算电流和电压
			{
				cnt = 0;

				memset(buf,0,14);
				
				buf[0] = 0x00;
				buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//失败灯号
				buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
				buf[3] = ctrl.control_type;
				buf[4] = ctrl.brightness;
				
				memcpy(&buf[5],DeviceElectricPara.volatge,2);
				memcpy(&buf[7],DeviceElectricPara.current,3);
				memcpy(&buf[10],DeviceElectricPara.active_power,2);
				memcpy(&buf[12],DeviceElectricPara.pf,2);
				
				RecordEventsECx(EVENT_ERC52,14,buf);
				
				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
	}
}















































