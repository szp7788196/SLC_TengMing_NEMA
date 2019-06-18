#include "task_main.h"
#include "common.h"
#include "delay.h"
#include "usart.h"
#include "inventr.h"
#include "pwm.h"


TaskHandle_t xHandleTaskMAIN = NULL;

RemoteControl_S ContrastControl;
unsigned portBASE_TYPE MAIN_Satck;

void vTaskMAIN(void *pvParameters)
{
	time_t times_sec = 0;
	u8 up_date_strategy_state = 0;

	SetLightLevel(CurrentControl);

	while(1)
	{
		if(GetSysTick1s() - times_sec >= 1)
		{
			times_sec = GetSysTick1s();

			if(GetTimeOK != 0)
			{
				CheckSwitchStatus(&CurrentControl);		//查询当前开关应该处于的状态

				if(CurrentControl._switch == 1)			//只有在开关为开的状态时才轮询策略
				{
					if(CurrentControl._switch != ContrastControl._switch)
					{
						up_date_strategy_state = 1;
					}
					
					LookUpStrategyList(ControlStrategy,&CurrentControl,&up_date_strategy_state);	//轮训策略列表
				}
			}
		}

		if(ContrastControl._switch != CurrentControl._switch ||
		   ContrastControl.control_type != CurrentControl.control_type ||
		   ContrastControl.brightness != CurrentControl.brightness ||
		   ContrastControl.interface != CurrentControl.interface)
		{
			ContrastControl._switch = CurrentControl._switch;
			ContrastControl.control_type = CurrentControl.control_type;
			ContrastControl.brightness = CurrentControl.brightness;
			ContrastControl.interface = CurrentControl.interface;

			SetLightLevel(CurrentControl);
		}

		if(DeviceReset == 0x01 || ReConnectToServer == 0x01) //接收到重启的命令
		{
			delay_ms(5000);

			if(DeviceReset == 0x01)
			{
				DeviceReset = 0;

				__disable_fault_irq();							//重启指令
				NVIC_SystemReset();
			}

			if(ReConnectToServer == 0x01)
			{
				ReConnectToServer = 0x81;
			}
		}

		delay_ms(100);
//		MAIN_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}

void CheckSwitchStatus(RemoteControl_S *ctrl)
{
	u16 i = 0;
	s16 sum = 0;
	u8 month = 0;
	u8 date = 0;
	u8 month_c = 0;
	u8 date_c = 0;
	u8 on_hour = 0;
	u8 on_minute = 0;
	u8 off_hour = 0;
	u8 off_minute = 0;

	u16 on_gate = 0;
	u16 off_gate = 0;
	u16 now_gate = 0;

	if(SwitchMode != 1)							//非年表控制
	{
		CurrentControl._switch = 1;				//默认开灯
	}
	else										//年表控制
	{
		if(LampsSwitchProject.total_days >= 1 &&
		   LampsSwitchProject.total_days <= 366)	//年表不为空
		{
			month = LampsSwitchProject.start_month;
			date = LampsSwitchProject.start_date;

			if(month >= 1 && month <= 12 && date >= 1 && date <= 31)
			{
				for(i = 0; i < LampsSwitchProject.total_days; i ++)
				{
					sum = get_day_num(month,date) + i;

					if(sum <= 366)
					{
						get_date_from_days(sum, &month_c, &date_c);

						if(month_c >= 1 && month_c <= 12 && date_c >= 1 && date_c <= 31)		//得到合法日期
						{
							if(month_c == calendar.w_month && date_c == calendar.w_date)
							{
								on_hour = (LampsSwitchProject.switch_time[i].on_time[1] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].on_time[1] & 0x0F);
								on_minute = (LampsSwitchProject.switch_time[i].on_time[0] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].on_time[0] & 0x0F);
								off_hour = (LampsSwitchProject.switch_time[i].off_time[1] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].off_time[1] & 0x0F);
								off_minute = (LampsSwitchProject.switch_time[i].off_time[0] >> 4) * 10 +
									   (LampsSwitchProject.switch_time[i].off_time[0] & 0x0F);

								if(on_hour <= 23 && on_minute <= 59 && off_hour <= 23 && off_minute <= 59)		//得到合法时间
								{
									on_gate = on_hour * 60 + on_minute;
									off_gate = off_hour * 60 + off_minute;
									now_gate = calendar.hour * 60 + calendar.min;

									if(on_gate != off_gate)
									{
										if(on_gate < off_gate)		//先开灯后关灯
										{
											if(on_gate <= now_gate && now_gate < off_gate)	//当前时间大于等于开灯时间,小于关灯时间,则开灯
											{
												ctrl->_switch = 1;		//开灯
											}
											else	//当前时间小于开灯时间,或者大于等于关灯时间,则关灯
											{
												ctrl->_switch = 0;		//关灯
											}
										}
										else						//先关灯后开灯
										{
											if(off_gate <= now_gate && now_gate < on_gate)	//当前时间大于等于关灯时间,小于开灯时间,则关灯
											{
												ctrl->_switch = 0;		//关灯
											}
											else	//当前时间小于关灯时间,或者大于等于开灯时间,则开灯
											{
												ctrl->_switch = 1;		//开灯
											}
										}
									}
									else					//开关灯时间相同
									{
										ctrl->_switch = 1;		//默认亮灯
									}
								}
								else	//得到非法时间
								{
									ctrl->_switch = 1;		//默认亮灯
								}

								i = 0xFF00;		//大于366即可
							}
						}
						else	//得到非法日期
						{
							ctrl->_switch = 1;	//默认亮灯
						}
					}
					else		//日期超出范围
					{
						ctrl->_switch = 1;		//默认亮灯
					}
				}
			}
			else								//年表表头错误
			{
				ctrl->_switch = 1;				//默认亮灯
			}
		}
		else									//年表为空
		{
			ctrl->_switch = 1;					//默认亮灯
		}
	}
}

//轮训策略列表
u8 LookUpStrategyList(pControlStrategy strategy_head,RemoteControl_S *ctrl,u8 *update)
{
	u8 ret = 0;

	static u8 date = 0;
	
	static time_t start_time = 0;		//开始执行时间
	static time_t total_time = 0;		//执行结束时间
	time_t current_time = 0;			//当前时间
	time_t start_time_next = 0;			//下一个策略的开始执行时间
	static u8 y_m_d_effective = 0;		//年月日有效标志 bit2 = 1年有效 bit1 = 1月有效 bit0 = 1日有效
	static u8 y_m_d_matched = 0;		//年月日匹配成功标志 bit2 = 1年 bit1 = 1月 bit0 = 1日
	static u8 y_m_d_effective_next = 0;	//年月日有效标志 bit2 = 1年有效 bit1 = 1月有效 bit0 = 1日有效
	static u8 y_m_d_matched_next = 0;	//年月日匹配成功标志 bit2 = 1年 bit1 = 1月 bit0 = 1日

	pControlStrategy tmp_strategy = NULL;

	if(NeedUpdateStrategyList == 1)
	{
		NeedUpdateStrategyList = 0;

		ret = UpdateControlStrategyList();	//更新策略列表

		if(ret == 0)						//更新策略列表失败
		{
//			ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度

			return 0;
		}
		else								//策略列表更新成功
		{
			start_time = 0;
			total_time = 0;
			current_time = 0;
			start_time_next = 0;
			y_m_d_effective = 0;
			y_m_d_matched = 0;
			y_m_d_effective_next = 0;
			y_m_d_matched_next = 0;
			
			date = calendar.w_date;
		}
	}

	xSemaphoreTake(xMutex_STRATEGY, portMAX_DELAY);
	
	if(date != calendar.w_date)		//过了一天或重新设置了时间
	{
		date = calendar.w_date;
		
		StrategyListStateReset(strategy_head,1);	//复位绝对时间策略状态
	}
	
	if(*update == 1)				//开关状态有变化,由关变为开
	{
		*update = 0;
		
		StrategyListStateReset(strategy_head,0);	//复位所有策略状态
	}

	if(strategy_head != NULL && ControlStrategy->next != NULL)	//策略列表不为空
	{
		for(tmp_strategy = strategy_head->next; tmp_strategy != NULL; tmp_strategy = tmp_strategy->next)
		{
			switch(tmp_strategy->mode)
			{
				case 0:		//相对时间方式
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//等待执行状态
							start_time = GetSysTick1s();			//获取开始执行时间 秒
							total_time = tmp_strategy->time_re;		//获取执行结束时间 秒

							ctrl->lock = 0;		//解锁远程控制

							tmp_strategy->state = EXECUTING;		//下个状态将切换为 正在执行状态

							if(tmp_strategy->state == EXECUTING)	//此判断无实际作用,置位消除编译器警告
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//正在执行状态
							current_time = GetSysTick1s();		//获取当前时间

							if(current_time - start_time <= total_time)
							{
								if(ctrl->lock == 0)				//远程控制已解锁
								{
									ctrl->control_type = tmp_strategy->type;
									ctrl->brightness = tmp_strategy->brightness;
								}

								if(current_time - start_time >= total_time)	//到了结束时间
								{
									tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
								}

								ret = 1;

								goto GET_OUT;
							}
						break;

						case EXECUTED:			//已过期状态

						break;

						default:				//未知状态
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
						break;
					}
				break;

				case 1:		//绝对时间方式
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//等待执行状态
							start_time = tmp_strategy->hour * 3600 +
						                 tmp_strategy->minute * 60 +
						                 tmp_strategy->second;

							if(tmp_strategy->year != 0)		//判断策略中年是否有效
							{
								y_m_d_effective |= 0x04;
							}

							if(tmp_strategy->month != 0)	//判断策略中月是否有效
							{
								y_m_d_effective |= 0x02;
							}

							if(tmp_strategy->date != 0)		//判断策略中日是否有效
							{
								y_m_d_effective |= 0x01;
							}

							y_m_d_matched = 0x07;			//初始化年月日匹配成功标志

							if(tmp_strategy->next != NULL)	//下个策略有效
							{
								if(tmp_strategy->next->mode == 1)		//下个策略也是绝对时间方式
								{
									if(tmp_strategy->next->year != 0)	//判断策略中年是否有效
									{
										y_m_d_effective_next |= 0x04;
									}

									if(tmp_strategy->next->month != 0)	//判断策略中月是否有效
									{
										y_m_d_effective_next |= 0x02;
									}

									if(tmp_strategy->next->date != 0)	//判断策略中日是否有效
									{
										y_m_d_effective_next |= 0x01;
									}

									y_m_d_matched_next = 0x07;			//初始化年月日匹配成功标志
								}
							}

							if(ctrl->lock == 1)				//判断是否需要解开远程控制所
							{
								if(y_m_d_effective & 0x04 != (u32)0x00000000 &&
								   tmp_strategy->year != calendar.w_year - 2000)
								{
									y_m_d_matched &= ~(0x04);
								}

								if(y_m_d_effective & 0x02 != (u32)0x00000000 &&
								   tmp_strategy->month != calendar.w_month)
								{
									y_m_d_matched &= ~(0x02);
								}

								if(y_m_d_effective & 0x01 != (u32)0x00000000 &&
								   tmp_strategy->date != calendar.w_date)
								{
									y_m_d_matched &= ~(0x01);
								}

								if(y_m_d_matched == 0x07)	//年月日全部匹配成功
								{
									current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

									if(tmp_strategy->next != NULL)				//下个策略有效
									{
										if(tmp_strategy->next->mode == 1)		//下个策略也是绝对时间方式
										{
											if(y_m_d_effective_next & 0x04 != (u32)0x00000000 &&
											   tmp_strategy->next->year != calendar.w_year - 2000)
											{
												y_m_d_matched_next &= ~(0x04);
											}

											if(y_m_d_effective_next & 0x02 != (u32)0x00000000 &&
											   tmp_strategy->next->month != calendar.w_month)
											{
												y_m_d_matched_next &= ~(0x02);
											}

											if(y_m_d_effective_next & 0x01 != (u32)0x00000000 &&
											   tmp_strategy->next->date != calendar.w_date)
											{
												y_m_d_matched_next &= ~(0x01);
											}

											if(y_m_d_matched_next == 0x07)		//下个策略的年月日全部匹配成功
											{
												start_time_next = tmp_strategy->next->hour * 3600 +
															      tmp_strategy->next->minute * 60 +
															      tmp_strategy->next->second;

												if(start_time <= current_time && current_time <= start_time_next)	//当前时间在当前策略和下个策略的时间段区间内
												{
													ctrl->lock = 0;				//解锁远程控制
												}
											}
											else	//下个策略的你那月日没有全部匹配成功
											{
												if(start_time <= current_time)	//当前时间大于等于起始执行时间
												{
													ctrl->lock = 0;				//解锁远程控制
												}
											}
										}
										else	//下个策略是相对时间方式
										{
											if(start_time <= current_time)		//当前时间大于等于起始执行时间
											{
												ctrl->lock = 0;					//解锁远程控制
											}
										}
									}
									else	//当前策略是最后一个策略,没有下个策略
									{
										if(start_time <= current_time)			//当前时间大于等于起始执行时间
										{
											ctrl->lock = 0;						//解锁远程控制
										}
									}
								}
							}

							tmp_strategy->state = EXECUTING;	//下个状态将切换为 正在执行状态

							if(tmp_strategy->state == EXECUTING)	//此判断无实际作用,置位消除编译器警告
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//正在执行状态
							if(y_m_d_effective & 0x04 != (u32)0x00000000 &&
							   tmp_strategy->year != calendar.w_year - 2000)
							{
								y_m_d_matched &= ~(0x04);
							}

							if(y_m_d_effective & 0x02 != (u32)0x00000000 &&
							   tmp_strategy->month != calendar.w_month)
							{
								y_m_d_matched &= ~(0x02);
							}

							if(y_m_d_effective & 0x01 != (u32)0x00000000 &&
							   tmp_strategy->date != calendar.w_date)
							{
								y_m_d_matched &= ~(0x01);
							}

							if(y_m_d_matched == 0x07)	//年月日全部匹配成功
							{
								current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

								if(tmp_strategy->next != NULL)			//下个策略有效
								{
									if(tmp_strategy->next->mode == 1)	//下个策略也是绝对时间方式
									{
										if(y_m_d_effective_next & 0x04 != (u32)0x00000000 &&
										   tmp_strategy->next->year != calendar.w_year - 2000)
										{
											y_m_d_matched_next &= ~(0x04);
										}

										if(y_m_d_effective_next & 0x02 != (u32)0x00000000 &&
										   tmp_strategy->next->month != calendar.w_month)
										{
											y_m_d_matched_next &= ~(0x02);
										}

										if(y_m_d_effective_next & 0x01 != (u32)0x00000000 &&
										   tmp_strategy->next->date != calendar.w_date)
										{
											y_m_d_matched_next &= ~(0x01);
										}

										if(y_m_d_matched_next == 0x07)	//下个策略的年月日全部匹配成功
										{
											start_time_next = tmp_strategy->next->hour * 3600 +
						                                      tmp_strategy->next->minute * 60 +
						                                      tmp_strategy->next->second;

											if(start_time <= current_time && current_time <= start_time_next)	//当前时间在当前策略和下个策略的时间段区间内
											{
												if(start_time == current_time)
												{
													if(ctrl->lock == 1)				//判断是否需要解开远程控制所
													{
														ctrl->lock = 0;
													}
												}

												if(ctrl->lock == 0)		//远程控制已解锁
												{
													ctrl->control_type = tmp_strategy->type;
													ctrl->brightness = tmp_strategy->brightness;
												}

												if(current_time >= start_time_next)		//当前策略即将执行结束
												{
													tmp_strategy->state = EXECUTED;		//下个状态将切换为 已过期状态
												}

												ret = 1;

												goto GET_OUT;
											}
											else if(current_time > start_time_next)		//下个策略已过期
											{
												tmp_strategy->state = EXECUTED;			//当前策略已过期
											}
											else if(current_time < start_time)
											{
												goto GET_OUT;							//等待当前策略生效
											}
										}
										else	//下个策略的年月日没有全部匹配成功
										{
											if(start_time <= current_time)	//当前时间大于等于起始执行时间
											{
												if(start_time == current_time)
												{
													if(ctrl->lock == 1)				//判断是否需要解开远程控制所
													{
														ctrl->lock = 0;
													}
												}

												if(ctrl->lock == 0)			//远程控制已解锁
												{
													ctrl->control_type = tmp_strategy->type;
													ctrl->brightness = tmp_strategy->brightness;
												}

												ret = 1;

												goto GET_OUT;
											}
										}
									}
									else	//下个策略是相对时间方式
									{
										if(start_time <= current_time)	//当前时间大于等于起始执行时间
										{
											if(start_time == current_time)
											{
												if(ctrl->lock == 1)				//判断是否需要解开远程控制所
												{
													ctrl->lock = 0;
												}
											}

											if(ctrl->lock == 0)			//远程控制已解锁
											{
												ctrl->control_type = tmp_strategy->type;
												ctrl->brightness = tmp_strategy->brightness;
											}

											ret = 1;

											goto GET_OUT;
										}
									}
								}
								else	//当前策略是最后一个策略,没有下个策略
								{
									if(start_time <= current_time)	//当前时间大于等于起始执行时间
									{
										if(start_time == current_time)
										{
											if(ctrl->lock == 1)				//判断是否需要解开远程控制所
											{
												ctrl->lock = 0;
											}
										}

										if(ctrl->lock == 0)			//远程控制已解锁
										{
											ctrl->control_type = tmp_strategy->type;
											ctrl->brightness = tmp_strategy->brightness;
										}

										ret = 1;

										goto GET_OUT;
									}
								}
							}
						break;

						case EXECUTED:			//已过期状态

						break;

						default:				//未知状态
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
						break;
					}
				break;

				default:	//未知方式
//					ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
				break;
			}
		}
	}
	else	//策略列表无效 为空
	{
//		ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//设置当前亮度为当前运行模式的初始亮度
	}

	GET_OUT:
	xSemaphoreGive(xMutex_STRATEGY);

	return ret;
}































