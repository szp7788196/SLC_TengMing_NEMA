#include "event.h"
#include "24cxx.h"
#include "task_sensor.h"

//记录并存储发生的事件
void RecordEventsECx(u8 ecx,u8 len,u8 *msg)
{
	u16 crc_cal = 0;
	u16 add_pos = 0;
	u8 buf[24];

	if((EventEffective & ((long long)1 << ((long long)ecx - 1))) == 0x00)
	{
		return;
	}

	if(xSchedulerRunning == 1)
	{
		xSemaphoreTake(xMutex_EVENT_RECORD, portMAX_DELAY);
	}

	memset(buf,0,24);

	buf[0] = ecx;						//事件代号
	buf[1] = len + 6;					//事件长度

	memcpy(buf + 2,CalendarClock,6);	//发生时间
	memcpy(buf + 8,msg,len);			//具体事件内容

	crc_cal = GetCRC16(buf,22);			//计算校验

	buf[22] = (u8)(crc_cal >> 8);
	buf[23] = (u8)(crc_cal & 0x00FF);

	if((EventImportant & ((long long)1 << ((long long)ecx - 1))) == 0x00)			//一般事件
	{
		add_pos = E_NORMAL_ADD;

		EventRecordList.lable2[EventRecordList.ec2] = ecx;	//更新事件列表

		crc_cal = GetCRC16(EventRecordList.lable2,256);

		AT24CXX_WriteOneByte(EC2_LABLE_ADD + EventRecordList.ec2,EventRecordList.lable2[EventRecordList.ec2]);
		AT24CXX_WriteOneByte(EC2_LABLE_ADD + 256,(u8)(crc_cal >> 8));
		AT24CXX_WriteOneByte(EC2_LABLE_ADD + 257,(u8)(crc_cal & 0x00FF));

		WriteDataFromMemoryToEeprom(buf,add_pos + EventRecordList.ec2 * EVENT_LEN, EVENT_LEN);

		EventRecordList.ec2 ++;				//更新事件计数器

		WriteDataFromMemoryToEeprom(&EventRecordList.ec2,EC2_ADD, 1);

		EventRecordList.normal_event_flag ++;
	}
	else					//重要事件
	{
		add_pos = E_IMPORTEAT_ADD;

		EventRecordList.lable1[EventRecordList.ec1] = ecx;	//更新事件列表

		crc_cal = GetCRC16(EventRecordList.lable1,256);

		AT24CXX_WriteOneByte(EC1_LABLE_ADD + EventRecordList.ec1,EventRecordList.lable1[EventRecordList.ec1]);
		AT24CXX_WriteOneByte(EC1_LABLE_ADD + 256,(u8)(crc_cal >> 8));
		AT24CXX_WriteOneByte(EC1_LABLE_ADD + 257,(u8)(crc_cal & 0x00FF));

		WriteDataFromMemoryToEeprom(buf,add_pos + EventRecordList.ec1 * EVENT_LEN, EVENT_LEN);

		EventRecordList.ec1 ++;				//更新事件计数器

		WriteDataFromMemoryToEeprom(&EventRecordList.ec1,EC1_ADD, 1);

		EventRecordList.important_event_flag ++;
	}

	if(xSchedulerRunning == 1)
	{
		xSemaphoreGive(xMutex_EVENT_RECORD);
	}
}

//检测正常开灯事件
void CheckEventsEC15(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static time_t cnt = 0;
	static u8 state_change = 0;
	static u8 first = 1;
	u8 buf[6];

	if(first == 1)
	{
		first = 0;

		control.control_type = 1;
		control.brightness = 0;
	}

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)		//单灯状态有变化
		{
			if(state_change == 0)
			{
				cnt = GetSysTick1s();

				state_change = 1;
			}

			if(GetSysTick1s() - cnt >= EventDetectConf.turn_on_collect_delay * 60)	//等待一段时间，计算电流和电压
			{
				cnt = 0;
				state_change = 0;

				if(control.control_type == 1 ||
				  (control.control_type == 2 && control.brightness == 0))	//上个状态为关灯
				{
					if(ctrl.control_type == 0 ||
					  (ctrl.control_type == 2 && ctrl.brightness >= 1))		//现在状态为开灯
					{
						memset(buf,0,6);

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

						RecordEventsECx(EVENT_ERC15,6,buf);

					}
				}

				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
		else
		{
			cnt = 0;
			state_change = 0;
		}
	}
}

//检测异常开灯事件
void CheckEventsEC16(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static time_t cnt = 0;
	static u8 state_change = 0;
	static u8 first = 1;
	u8 buf[6];

	if(first == 1)
	{
		first = 0;

		control.control_type = 1;
		control.brightness = 0;
	}

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)		//单灯状态有变化
		{
			if(state_change == 0)
			{
				cnt = GetSysTick1s();

				state_change = 1;
			}

			if(GetSysTick1s() - cnt >= EventDetectConf.turn_off_collect_delay * 60)	//等待一段时间，计算电流和电压
			{
				cnt = 0;
				state_change = 0;

				if(control.control_type == 0 ||
				  (control.control_type == 2 && control.brightness >= 1))	//上个状态为开灯
				{
					if(ctrl.control_type == 1 ||
					  (ctrl.control_type == 2 && ctrl.brightness > 0))		//现在状态为关灯
					{
						memset(buf,0,6);

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

						RecordEventsECx(EVENT_ERC16,6,buf);
					}
				}

				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
		else
		{
			cnt = 0;
			state_change = 0;
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
			if(InputCurrent > SWITCH_OFF_MAX_CURRENT)				//异常开灯
			{
				if(occur == 0)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
			}
			else
			{
				if(occur == 1)
				{
					if(cnt > -100)
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

		if(cnt >= 100)
		{
			cnt = 0;
			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x01;														//记录类型
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;														//事件开始

			RecordEventsECx(EVENT_ERC17,4,buf);
		}
		else if(cnt <= -100)
		{
			cnt = 0;
			occur = 0;

			memset(buf,0,4);

			buf[0] = 0x00;														//记录类型
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;														//事件结束

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
			if(InputCurrent <= SWITCH_ON_MIN_CURRENT)		//异常开灯
			{
				if(occur == 0)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
			}
			else
			{
				if(occur == 1)
				{
					if(cnt > -100)
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

		if(cnt >= 100)
		{
			cnt = 0;
			occur = 1;

			memset(buf,0,4);

			buf[0] = 0x01;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;

			RecordEventsECx(EVENT_ERC18,4,buf);
		}
		else if(cnt <= -100)
		{
			cnt = 0;
			occur = 0;

			memset(buf,0,4);

			buf[0] = 0x00;
			buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
			buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
			buf[3] = 0x00;

			RecordEventsECx(EVENT_ERC18,4,buf);
		}
	}
}

//检测单灯电流过大记录
void CheckEventsEC19(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static time_t time_cnt = 0;
	static u8 occur = 0;
	static s8 cnt = 0;
	static u8 first = 1;
	u8 buf[7];

	if(first == 1)
	{
		first = 0;

		control.control_type = 1;
		control.brightness = 0;
	}

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)		//单灯状态有变化
		{
			time_cnt = GetSysTick1s();
			occur = 0;
			cnt = 0;

			control.control_type = ctrl.control_type;
			control.brightness = ctrl.brightness;
			control.interface = ctrl.interface;
		}

		if(GetSysTick1s() - time_cnt >= EventDetectConf.current_detect_delay * 60)	//等待一段时间，计算电流和电压
		{
			if(occur == 0)
			{
				if((InputCurrent - CurrentControl.current) / CurrentControl.current >=
				  (float)EventDetectConf.over_current_ratio / 100.0f)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > 0)
					{
						cnt --;
					}
				}

				if(cnt >= 100)
				{
					cnt = 0;
					occur = 1;

					buf[0] = 0x01;														//记录类型
					buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
					buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
					buf[3] = DeviceElectricPara.current[0];								//发生时电流
					buf[4] = DeviceElectricPara.current[1];
					buf[5] = DeviceElectricPara.current[2];
					buf[6] = control.control_type;										//发生时控制状态

					RecordEventsECx(EVENT_ERC19,7,buf);
				}
			}
			else if(occur == 1)
			{
				if((InputCurrent - CurrentControl.current) / CurrentControl.current <=
				  (float)EventDetectConf.over_current_recovery_ratio / 100.0f)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > 0)
					{
						cnt --;
					}
				}

				if(cnt >= 100)
				{
					cnt = 0;
					occur = 0;

					buf[0] = 0x00;
					buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
					buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
					buf[3] = DeviceElectricPara.current[0];
					buf[4] = DeviceElectricPara.current[1];
					buf[5] = DeviceElectricPara.current[2];
					buf[6] = control.control_type;

					RecordEventsECx(EVENT_ERC19,7,buf);
				}
			}
		}
	}
}

//检测单灯电流过小记录
void CheckEventsEC20(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	static time_t time_cnt = 0;
	static u8 occur = 0;
	static s8 cnt = 0;
	static u8 first = 1;
	u8 buf[7];

	if(first == 1)
	{
		first = 0;

		control.control_type = 1;
		control.brightness = 0;
	}

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)
		{
			time_cnt = GetSysTick1s();
			occur = 0;
			cnt = 0;

			control.control_type = ctrl.control_type;
			control.brightness = ctrl.brightness;
			control.interface = ctrl.interface;
		}

		if(GetSysTick1s() - time_cnt >= EventDetectConf.current_detect_delay * 60)	//等待一段时间，计算电流和电压
		{
			if(occur == 0)
			{
				if((CurrentControl.current - InputCurrent) / CurrentControl.current >=
				  (float)EventDetectConf.low_current_ratio / 100.0f)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > 0)
					{
						cnt --;
					}
				}

				if(cnt >= 100)
				{
					cnt = 0;
					occur = 1;

					buf[0] = 0x01;
					buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
					buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
					buf[3] = DeviceElectricPara.current[0];
					buf[4] = DeviceElectricPara.current[1];
					buf[5] = DeviceElectricPara.current[2];
					buf[6] = control.control_type;

					RecordEventsECx(EVENT_ERC20,7,buf);
				}
			}
			else if(occur == 1)
			{
				if((CurrentControl.current - InputCurrent) / CurrentControl.current <=
				  (float)EventDetectConf.low_current_recovery_ratio / 100.0f)
				{
					if(cnt < 100)
					{
						cnt ++;
					}
				}
				else
				{
					if(cnt > 0)
					{
						cnt --;
					}
				}

				if(cnt >= 100)
				{
					cnt = 0;
					occur = 0;

					buf[0] = 0x00;
					buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
					buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
					buf[3] = DeviceElectricPara.current[0];
					buf[4] = DeviceElectricPara.current[1];
					buf[5] = DeviceElectricPara.current[2];
					buf[6] = control.control_type;

					RecordEventsECx(EVENT_ERC20,7,buf);
				}
			}
		}
	}
}

//校时结果事件记录
void CheckEventsEC28(u8 *cal1,u8 *cal2)
{
	u8 buf[13];

	memset(buf,0,13);

	buf[0] = 0x00;

	memcpy(&buf[1],cal1,6);		//校时前时间
	memcpy(&buf[7],cal2,6);		//校时后时间

	RecordEventsECx(EVENT_ERC28,13,buf);
}

//升级结果事件记录
void CheckEventsEC51(u8 result,u8 *version)
{
	u8 buf[9];

	memset(buf,0,9);

	buf[0] = result;			//记录类型 成功或失败

	memcpy(&buf[1],version,8);	//软件版本号

	RecordEventsECx(EVENT_ERC51,9,buf);
}

//单灯状态变化记录
void CheckEventsEC52(RemoteControl_S ctrl)
{
	static RemoteControl_S control;
	u8 buf[14];
	static u8 cnt = 0;
	static u8 first = 1;

	if(first == 1)
	{
		first = 0;

		control.control_type = 1;
		control.brightness = 0;
	}

	if(LampsParameters.num != 0)		//有已经配置过的灯
	{
		if(control.control_type != ctrl.control_type ||
		   control.brightness != ctrl.brightness)
		{
			cnt ++;

			if(cnt >= 100)	//等待一段时间，计算电流和电压
			{
				cnt = 0;

				memset(buf,0,14);

				buf[0] = 0x00;														//记录类型
				buf[1] = (u8)(LampsParameters.parameters[0].lamps_id & 0x00FF);		//灯具序号
				buf[2] = (u8)(LampsParameters.parameters[0].lamps_id >> 8);
				buf[3] = ctrl.control_type;											//控制类型
				buf[4] = ctrl.brightness;											//亮度

				memcpy(&buf[5],DeviceElectricPara.volatge,2);						//当前电压
				memcpy(&buf[7],DeviceElectricPara.current,3);						//当前电流
				memcpy(&buf[10],DeviceElectricPara.active_power,2);					//当前功率
				memcpy(&buf[12],DeviceElectricPara.pf,2);							//当前功率因数

				RecordEventsECx(EVENT_ERC52,14,buf);

				control.control_type = ctrl.control_type;
				control.brightness = ctrl.brightness;
				control.interface = ctrl.interface;
			}
		}
		else
		{
			cnt = 0;
		}
	}
}















































