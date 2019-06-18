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
				CheckSwitchStatus(&CurrentControl);		//��ѯ��ǰ����Ӧ�ô��ڵ�״̬

				if(CurrentControl._switch == 1)			//ֻ���ڿ���Ϊ����״̬ʱ����ѯ����
				{
					if(CurrentControl._switch != ContrastControl._switch)
					{
						up_date_strategy_state = 1;
					}
					
					LookUpStrategyList(ControlStrategy,&CurrentControl,&up_date_strategy_state);	//��ѵ�����б�
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

		if(DeviceReset != 0x00 || ReConnectToServer == 0x01) //���յ�����������
		{
			if(DeviceReset == 0x01)
			{
				delay_ms(5000);
				
				DeviceReset = 0;

				__disable_fault_irq();							//����ָ��
				NVIC_SystemReset();
			}
			else if(DeviceReset == 0x02 || DeviceReset == 0x03)
			{
				RestoreFactorySettings(DeviceReset);
				
				DeviceReset = 0;
			}

			if(ReConnectToServer == 0x01)
			{
				delay_ms(5000);
				
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

	if(SwitchMode != 1)							//��������
	{
		CurrentControl._switch = 1;				//Ĭ�Ͽ���
	}
	else										//������
	{
		if(LampsSwitchProject.total_days >= 1 &&
		   LampsSwitchProject.total_days <= 366)	//���Ϊ��
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

						if(month_c >= 1 && month_c <= 12 && date_c >= 1 && date_c <= 31)		//�õ��Ϸ�����
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

								if(on_hour <= 23 && on_minute <= 59 && off_hour <= 23 && off_minute <= 59)		//�õ��Ϸ�ʱ��
								{
									on_gate = on_hour * 60 + on_minute;
									off_gate = off_hour * 60 + off_minute;
									now_gate = calendar.hour * 60 + calendar.min;

									if(on_gate != off_gate)
									{
										if(on_gate < off_gate)		//�ȿ��ƺ�ص�
										{
											if(on_gate <= now_gate && now_gate < off_gate)	//��ǰʱ����ڵ��ڿ���ʱ��,С�ڹص�ʱ��,�򿪵�
											{
												ctrl->_switch = 1;		//����
											}
											else	//��ǰʱ��С�ڿ���ʱ��,���ߴ��ڵ��ڹص�ʱ��,��ص�
											{
												ctrl->_switch = 0;		//�ص�
											}
										}
										else						//�ȹصƺ󿪵�
										{
											if(off_gate <= now_gate && now_gate < on_gate)	//��ǰʱ����ڵ��ڹص�ʱ��,С�ڿ���ʱ��,��ص�
											{
												ctrl->_switch = 0;		//�ص�
											}
											else	//��ǰʱ��С�ڹص�ʱ��,���ߴ��ڵ��ڿ���ʱ��,�򿪵�
											{
												ctrl->_switch = 1;		//����
											}
										}
									}
									else					//���ص�ʱ����ͬ
									{
										ctrl->_switch = 1;		//Ĭ������
									}
								}
								else	//�õ��Ƿ�ʱ��
								{
									ctrl->_switch = 1;		//Ĭ������
								}

								i = 0xFF00;		//����366����
							}
						}
						else	//�õ��Ƿ�����
						{
							ctrl->_switch = 1;	//Ĭ������
						}
					}
					else		//���ڳ�����Χ
					{
						ctrl->_switch = 1;		//Ĭ������
					}
				}
			}
			else								//����ͷ����
			{
				ctrl->_switch = 1;				//Ĭ������
			}
		}
		else									//���Ϊ��
		{
			ctrl->_switch = 1;					//Ĭ������
		}
	}
}

//��ѵ�����б�
u8 LookUpStrategyList(pControlStrategy strategy_head,RemoteControl_S *ctrl,u8 *update)
{
	u8 ret = 0;

	static u8 date = 0;
	
	static time_t start_time = 0;		//��ʼִ��ʱ��
	static time_t total_time = 0;		//ִ�н���ʱ��
	time_t current_time = 0;			//��ǰʱ��
	time_t start_time_next = 0;			//��һ�����ԵĿ�ʼִ��ʱ��
	static u8 y_m_d_effective = 0;		//��������Ч��־ bit2 = 1����Ч bit1 = 1����Ч bit0 = 1����Ч
	static u8 y_m_d_matched = 0;		//������ƥ��ɹ���־ bit2 = 1�� bit1 = 1�� bit0 = 1��
	static u8 y_m_d_effective_next = 0;	//��������Ч��־ bit2 = 1����Ч bit1 = 1����Ч bit0 = 1����Ч
	static u8 y_m_d_matched_next = 0;	//������ƥ��ɹ���־ bit2 = 1�� bit1 = 1�� bit0 = 1��
	
	static u8 appointment_control_valid = 0;	//ԤԼ������Ч��־
	static u8 appointment_control_valid_c = 0;	//ԤԼ������Ч��־ �Ƚ�
	static u8 appointment_control_valid_r = 0;	//ԤԼ������Ч��־ ˢ��

	pControlStrategy tmp_strategy = NULL;
	
	appointment_control_valid = CheckAppointmentControlValid();		//�鿴ԤԼ�����Ƿ���Ч
	
	if(appointment_control_valid_c != appointment_control_valid)	//ԤԼ����״̬�б仯
	{
		appointment_control_valid_c = appointment_control_valid;
		
		appointment_control_valid_r = 1;
	}

	if(NeedUpdateStrategyList == 1 || 
	   appointment_control_valid_r == 1)	//��Ҫ���²���
	{	
		NeedUpdateStrategyList = 0;
		appointment_control_valid_r = 0;

		ret = UpdateControlStrategyList(appointment_control_valid);	//���²����б�

		if(ret == 0)						//���²����б�ʧ��
		{
//			ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//���õ�ǰ����Ϊ��ǰ����ģʽ�ĳ�ʼ����

			return 0;
		}
		else								//�����б���³ɹ�
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
	
	if(date != calendar.w_date)		//����һ�������������ʱ��
	{
		date = calendar.w_date;
		
		StrategyListStateReset(strategy_head,1);	//��λ����ʱ�����״̬
	}
	
	if(*update == 1)				//����״̬�б仯,�ɹر�Ϊ��
	{
		*update = 0;
		
		StrategyListStateReset(strategy_head,0);	//��λ���в���״̬
	}

	if(strategy_head != NULL && ControlStrategy->next != NULL)	//�����б�Ϊ��
	{
		for(tmp_strategy = strategy_head->next; tmp_strategy != NULL; tmp_strategy = tmp_strategy->next)
		{
			switch(tmp_strategy->mode)
			{
				case 0:		//���ʱ�䷽ʽ
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//�ȴ�ִ��״̬
							start_time = GetSysTick1s();			//��ȡ��ʼִ��ʱ�� ��
							total_time = tmp_strategy->time_re;		//��ȡִ�н���ʱ�� ��

							ctrl->lock = 0;		//����Զ�̿���

							tmp_strategy->state = EXECUTING;		//�¸�״̬���л�Ϊ ����ִ��״̬

							if(tmp_strategy->state == EXECUTING)	//���ж���ʵ������,��λ��������������
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//����ִ��״̬
							current_time = GetSysTick1s();		//��ȡ��ǰʱ��

							if(current_time - start_time <= total_time)
							{
								if(ctrl->lock == 0)				//Զ�̿����ѽ���
								{
									ctrl->control_type = tmp_strategy->type;
									ctrl->brightness = tmp_strategy->brightness;
								}

								if(current_time - start_time >= total_time)	//���˽���ʱ��
								{
									tmp_strategy->state = EXECUTED;		//�¸�״̬���л�Ϊ �ѹ���״̬
								}

								ret = 1;

								goto GET_OUT;
							}
						break;

						case EXECUTED:			//�ѹ���״̬

						break;

						default:				//δ֪״̬
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//���õ�ǰ����Ϊ��ǰ����ģʽ�ĳ�ʼ����
						break;
					}
				break;

				case 1:		//����ʱ�䷽ʽ
					switch(tmp_strategy->state)
					{
						case WAIT_EXECUTE:		//�ȴ�ִ��״̬
							start_time = tmp_strategy->hour * 3600 +
						                 tmp_strategy->minute * 60 +
						                 tmp_strategy->second;

							if(tmp_strategy->year != 0)		//�жϲ��������Ƿ���Ч
							{
								y_m_d_effective |= 0x04;
							}

							if(tmp_strategy->month != 0)	//�жϲ��������Ƿ���Ч
							{
								y_m_d_effective |= 0x02;
							}

							if(tmp_strategy->date != 0)		//�жϲ��������Ƿ���Ч
							{
								y_m_d_effective |= 0x01;
							}

							y_m_d_matched = 0x07;			//��ʼ��������ƥ��ɹ���־

							if(tmp_strategy->next != NULL)	//�¸�������Ч
							{
								if(tmp_strategy->next->mode == 1)		//�¸�����Ҳ�Ǿ���ʱ�䷽ʽ
								{
									if(tmp_strategy->next->year != 0)	//�жϲ��������Ƿ���Ч
									{
										y_m_d_effective_next |= 0x04;
									}

									if(tmp_strategy->next->month != 0)	//�жϲ��������Ƿ���Ч
									{
										y_m_d_effective_next |= 0x02;
									}

									if(tmp_strategy->next->date != 0)	//�жϲ��������Ƿ���Ч
									{
										y_m_d_effective_next |= 0x01;
									}

									y_m_d_matched_next = 0x07;			//��ʼ��������ƥ��ɹ���־
								}
							}

							if(ctrl->lock == 1)				//�ж��Ƿ���Ҫ�⿪Զ�̿�����
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

								if(y_m_d_matched == 0x07)	//������ȫ��ƥ��ɹ�
								{
									current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

									if(tmp_strategy->next != NULL)				//�¸�������Ч
									{
										if(tmp_strategy->next->mode == 1)		//�¸�����Ҳ�Ǿ���ʱ�䷽ʽ
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

											if(y_m_d_matched_next == 0x07)		//�¸����Ե�������ȫ��ƥ��ɹ�
											{
												start_time_next = tmp_strategy->next->hour * 3600 +
															      tmp_strategy->next->minute * 60 +
															      tmp_strategy->next->second;

												if(start_time <= current_time && current_time <= start_time_next)	//��ǰʱ���ڵ�ǰ���Ժ��¸����Ե�ʱ���������
												{
													ctrl->lock = 0;				//����Զ�̿���
												}
											}
											else	//�¸����Ե���������û��ȫ��ƥ��ɹ�
											{
												if(start_time <= current_time)	//��ǰʱ����ڵ�����ʼִ��ʱ��
												{
													ctrl->lock = 0;				//����Զ�̿���
												}
											}
										}
										else	//�¸����������ʱ�䷽ʽ
										{
											if(start_time <= current_time)		//��ǰʱ����ڵ�����ʼִ��ʱ��
											{
												ctrl->lock = 0;					//����Զ�̿���
											}
										}
									}
									else	//��ǰ���������һ������,û���¸�����
									{
										if(start_time <= current_time)			//��ǰʱ����ڵ�����ʼִ��ʱ��
										{
											ctrl->lock = 0;						//����Զ�̿���
										}
									}
								}
							}

							tmp_strategy->state = EXECUTING;	//�¸�״̬���л�Ϊ ����ִ��״̬

							if(tmp_strategy->state == EXECUTING)	//���ж���ʵ������,��λ��������������
							{
								goto GET_OUT;
							}
						break;

						case EXECUTING:			//����ִ��״̬
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

							if(y_m_d_matched == 0x07)	//������ȫ��ƥ��ɹ�
							{
								current_time = calendar.hour * 3600 + calendar.min * 60 + calendar.sec;

								if(tmp_strategy->next != NULL)			//�¸�������Ч
								{
									if(tmp_strategy->next->mode == 1)	//�¸�����Ҳ�Ǿ���ʱ�䷽ʽ
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

										if(y_m_d_matched_next == 0x07)	//�¸����Ե�������ȫ��ƥ��ɹ�
										{
											start_time_next = tmp_strategy->next->hour * 3600 +
						                                      tmp_strategy->next->minute * 60 +
						                                      tmp_strategy->next->second;

											if(start_time <= current_time && current_time <= start_time_next)	//��ǰʱ���ڵ�ǰ���Ժ��¸����Ե�ʱ���������
											{
												if(start_time == current_time)
												{
													if(ctrl->lock == 1)				//�ж��Ƿ���Ҫ�⿪Զ�̿�����
													{
														ctrl->lock = 0;
													}
												}

												if(ctrl->lock == 0)		//Զ�̿����ѽ���
												{
													ctrl->control_type = tmp_strategy->type;
													ctrl->brightness = tmp_strategy->brightness;
												}

												if(current_time >= start_time_next)		//��ǰ���Լ���ִ�н���
												{
													tmp_strategy->state = EXECUTED;		//�¸�״̬���л�Ϊ �ѹ���״̬
												}

												ret = 1;

												goto GET_OUT;
											}
											else if(current_time > start_time_next)		//�¸������ѹ���
											{
												tmp_strategy->state = EXECUTED;			//��ǰ�����ѹ���
											}
											else if(current_time < start_time)
											{
												goto GET_OUT;							//�ȴ���ǰ������Ч
											}
										}
										else	//�¸����Ե�������û��ȫ��ƥ��ɹ�
										{
											if(start_time <= current_time)	//��ǰʱ����ڵ�����ʼִ��ʱ��
											{
												if(start_time == current_time)
												{
													if(ctrl->lock == 1)				//�ж��Ƿ���Ҫ�⿪Զ�̿�����
													{
														ctrl->lock = 0;
													}
												}

												if(ctrl->lock == 0)			//Զ�̿����ѽ���
												{
													ctrl->control_type = tmp_strategy->type;
													ctrl->brightness = tmp_strategy->brightness;
												}

												ret = 1;

												goto GET_OUT;
											}
										}
									}
									else	//�¸����������ʱ�䷽ʽ
									{
										if(start_time <= current_time)	//��ǰʱ����ڵ�����ʼִ��ʱ��
										{
											if(start_time == current_time)
											{
												if(ctrl->lock == 1)				//�ж��Ƿ���Ҫ�⿪Զ�̿�����
												{
													ctrl->lock = 0;
												}
											}

											if(ctrl->lock == 0)			//Զ�̿����ѽ���
											{
												ctrl->control_type = tmp_strategy->type;
												ctrl->brightness = tmp_strategy->brightness;
											}

											ret = 1;

											goto GET_OUT;
										}
									}
								}
								else	//��ǰ���������һ������,û���¸�����
								{
									if(start_time <= current_time)	//��ǰʱ����ڵ�����ʼִ��ʱ��
									{
										if(start_time == current_time)
										{
											if(ctrl->lock == 1)				//�ж��Ƿ���Ҫ�⿪Զ�̿�����
											{
												ctrl->lock = 0;
											}
										}

										if(ctrl->lock == 0)			//Զ�̿����ѽ���
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

						case EXECUTED:			//�ѹ���״̬

						break;

						default:				//δ֪״̬
//							ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//���õ�ǰ����Ϊ��ǰ����ģʽ�ĳ�ʼ����
						break;
					}
				break;

				default:	//δ֪��ʽ
//					ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//���õ�ǰ����Ϊ��ǰ����ģʽ�ĳ�ʼ����
				break;
			}
		}
	}
	else	//�����б���Ч Ϊ��
	{
//		ctrl->brightness = LampsRunMode.run_mode[0].initial_brightness;		//���õ�ǰ����Ϊ��ǰ����ģʽ�ĳ�ʼ����
	}

	GET_OUT:
	xSemaphoreGive(xMutex_STRATEGY);

	return ret;
}































