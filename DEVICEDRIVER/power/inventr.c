#include "inventr.h"
#include "common.h"
#include "usart.h"
#include "pwm.h"
#include "dac.h"
#include "cd4051b.h"

u8 InventrBusy = 0;
u8 InventrDisable = 0;

float InventrInPutCurrent = 0.0f;
float InventrInPutVoltage = 0.0f;
float InventrOutPutCurrent = 0.0f;
float InventrOutPutVoltage = 0.0f;

void RELAY_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RELAY_OFF;
}

//设置恒功率最大电流
u8 InventrSetMaxPowerCurrent(u8 percent)
{
	u8 ret = 0;
	u8 i = 0;
	u8 send_buf[8];

	if(InventrDisable == 0)
	{
		if(xSchedulerRunning == 1)
		{
			xSemaphoreTake(xMutex_INVENTR, portMAX_DELAY);
		}

		if(InventrBusy == 0)
		{
			InventrBusy = 1;

			memset(send_buf,0,8);

			send_buf[0] = 0x3A;
			send_buf[1] = 0x31;
			send_buf[2] = 0x00;
			send_buf[3] = 0x01;
			send_buf[4] = percent;
			send_buf[6] = 0x0D;
			send_buf[7] = 0x0A;

			for(i = 1; i <= 4; i ++)
			{
				send_buf[5] += send_buf[i];
			}

			UsartSendString(USART2,send_buf,8);

			i = 10;

			while(i --)
			{
				delay_ms(100);

				if(Usart2RecvEnd == 0xAA)
				{
					Usart2RecvEnd = 0;

					if(MyStrstr(Usart2RxBuf, send_buf, Usart2FrameLen, 8) != 0xFFFF)
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);
					}
					else
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);

						i = 1;
						ret = 1;
					}
				}
			}

			InventrBusy = 0;
		}

		if(xSchedulerRunning == 1)
		{
			xSemaphoreGive(xMutex_INVENTR);
		}
	}

	return ret;
}

//设置亮度级别，范围0~200
u8 InventrSetLightLevel(u8 level)
{
	u8 ret = 0;
	u8 i = 0;
	static u8 re_try_cnt = 0;
	u8 send_buf[8];

	if(InventrDisable == 0)
	{
		if(xSchedulerRunning == 1)
		{
			xSemaphoreTake(xMutex_INVENTR, portMAX_DELAY);
		}

		if(InventrBusy == 0)
		{
			InventrBusy = 1;

			memset(send_buf,0,8);

			send_buf[0] = 0x3A;
			send_buf[1] = 0x3C;
			send_buf[2] = 0x00;
			send_buf[3] = 0x01;
			send_buf[4] = level;
			send_buf[6] = 0x0D;
			send_buf[7] = 0x0A;

			for(i = 1; i <= 4; i ++)
			{
				send_buf[5] += send_buf[i];
			}

			RE_SEND:
			UsartSendString(USART2,send_buf,8);

			i = 10;

			while(i --)
			{
				delay_ms(100);

				if(Usart2RecvEnd == 0xAA)
				{
					Usart2RecvEnd = 0;

					if(MyStrstr(Usart2RxBuf, send_buf, Usart2FrameLen, 8) != 0xFFFF)
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);
					}
					else
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);

						i = 1;
						ret = 1;
					}
				}
			}

			if(ret == 0)
			{
				re_try_cnt ++;

				if(re_try_cnt < 10)
				{
					goto RE_SEND;
				}
			}

			InventrBusy = 0;
		}

		if(xSchedulerRunning == 1)
		{
			xSemaphoreGive(xMutex_INVENTR);
		}
	}

	return ret;
}

//读取输出电流
float InventrGetOutPutCurrent(void)
{
	static float current = 0.0f;
	u8 i = 0;
	u8 sum_cal = 0;
	u8 sum_recv = 0;
	u8 send_buf[8];
	u8 revc_buf[10];
//	u16 adc_val = 0;

	if(InventrDisable == 0)
	{
		if(xSchedulerRunning == 1)
		{
			xSemaphoreTake(xMutex_INVENTR, portMAX_DELAY);
		}

		if(InventrBusy == 0)
		{
			InventrBusy = 1;

			memset(send_buf,0,8);

			send_buf[0] = 0x3A;
			send_buf[1] = 0x3A;
			send_buf[2] = 0x00;
			send_buf[3] = 0x01;
			send_buf[4] = 0x02;
			send_buf[6] = 0x0D;
			send_buf[7] = 0x0A;

			for(i = 1; i <= 4; i ++)
			{
				send_buf[5] += send_buf[i];
			}

			UsartSendString(USART2,send_buf,8);

			i = 10;

			while(i --)
			{
				delay_ms(100);

				if(Usart2RecvEnd == 0xAA)
				{
					Usart2RecvEnd = 0;

					if(MyStrstr(Usart2RxBuf, send_buf, Usart2FrameLen, 8) != 0xFFFF)
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);
					}
					else
					{
						memset(revc_buf,0,10);

						if(Usart2FrameLen <= 10)
						{
							memcpy(revc_buf,Usart2RxBuf,Usart2FrameLen);


							for(i = 1; i < Usart2FrameLen - 3; i ++)
							{
								sum_cal += revc_buf[i];
							}

							sum_recv = revc_buf[Usart2FrameLen - 3];

							if(sum_cal == sum_recv)
							{
								current = (float)((((u16)revc_buf[4]) << 8) + (u16)revc_buf[5]);			//新的通讯协议，读出来的值无需转换，直接是电流值ma

	//							if(adc_val > INVENTR_MAX_CURRENT_ADC_VAL)
	//							{
	//								adc_val = INVENTR_MAX_CURRENT_ADC_VAL;
	//							}
	//
	//							current = (float)INVENTR_MAX_CURRENT_MA * ((float)adc_val / (float)INVENTR_MAX_CURRENT_ADC_VAL);
							}
						}

						memset(Usart2RxBuf,0,Usart2FrameLen);

						i = 1;
					}
				}
			}

			InventrBusy = 0;
		}

		if(xSchedulerRunning == 1)
		{
			xSemaphoreGive(xMutex_INVENTR);
		}
	}

	return current;
}

//读取输出电压
float InventrGetOutPutVoltage(void)
{
	static float voltage = 0.0f;
	u8 i = 0;
	u8 sum_cal = 0;
	u8 sum_recv = 0;
	u8 send_buf[8];
	u8 revc_buf[10];
//	u16 adc_val = 0;

	if(InventrDisable == 0)
	{
		if(xSchedulerRunning == 1)
		{
			xSemaphoreTake(xMutex_INVENTR, portMAX_DELAY);
		}

		if(InventrBusy == 0)
		{
			InventrBusy = 1;

			memset(send_buf,0,8);

			send_buf[0] = 0x3A;
			send_buf[1] = 0x3A;
			send_buf[2] = 0x01;
			send_buf[3] = 0x01;
			send_buf[4] = 0x02;
			send_buf[6] = 0x0D;
			send_buf[7] = 0x0A;

			for(i = 1; i <= 4; i ++)
			{
				send_buf[5] += send_buf[i];
			}

			UsartSendString(USART2,send_buf,8);

			i = 10;

			while(i --)
			{
				delay_ms(100);

				if(Usart2RecvEnd == 0xAA)
				{
					Usart2RecvEnd = 0;

					if(MyStrstr(Usart2RxBuf, send_buf, Usart2FrameLen, 8) != 0xFFFF)
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);
					}
					else
					{
						memset(revc_buf,0,10);

						if(Usart2FrameLen <= 10)
						{
							memcpy(revc_buf,Usart2RxBuf,Usart2FrameLen);


							for(i = 1; i < Usart2FrameLen - 3; i ++)
							{
								sum_cal += revc_buf[i];
							}

							sum_recv = revc_buf[Usart2FrameLen - 3];

							if(sum_cal == sum_recv)
							{
								voltage = (float)((((u16)revc_buf[4]) << 8) + (u16)revc_buf[5]);				//新的通讯协议，读出来的值无需转换，直接是电压值V

	//							if(adc_val > INVENTR_MAX_VOLTAGE_ADC_VAL)
	//							{
	//								adc_val = INVENTR_MAX_VOLTAGE_ADC_VAL;
	//							}
	//
	//							voltage = (float)INVENTR_MAX_VOLTAGE_V * ((float)adc_val / (float)INVENTR_MAX_VOLTAGE_ADC_VAL);
							}
						}

						memset(Usart2RxBuf,0,Usart2FrameLen);

						i = 1;
					}
				}
			}

			InventrBusy = 0;
		}

		if(xSchedulerRunning == 1)
		{
			xSemaphoreGive(xMutex_INVENTR);
		}
	}

	return voltage;
}

//读取机种信息
u8 InventrGetDeviceInfo(void)
{
	u8 ret = 0;
	u8 i = 0;
	u8 send_buf[8];

	if(InventrDisable == 0)
	{
		if(xSchedulerRunning == 1)
		{
			xSemaphoreTake(xMutex_INVENTR, portMAX_DELAY);
		}

		if(InventrBusy == 0)
		{
			InventrBusy = 1;

			memset(send_buf,0,8);

			send_buf[0] = 0x3A;
			send_buf[1] = 0x35;
			send_buf[2] = 0x0B;
			send_buf[3] = 0x01;
			send_buf[4] = 0x05;
			send_buf[6] = 0x0D;
			send_buf[7] = 0x0A;

			for(i = 1; i <= 4; i ++)
			{
				send_buf[5] += send_buf[i];
			}

			UsartSendString(USART2,send_buf,8);

			i = 10;

			while(i --)
			{
				delay_ms(100);

				if(Usart2RecvEnd == 0xAA)
				{
					Usart2RecvEnd = 0;

					if(MyStrstr(Usart2RxBuf, send_buf, Usart2FrameLen, 8) != 0xFFFF)
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);
					}
					else
					{
						memset(Usart2RxBuf,0,Usart2FrameLen);

						i = 1;
						ret = 1;
					}
				}
			}

			InventrBusy = 0;
		}

		if(xSchedulerRunning == 1)
		{
			xSemaphoreGive(xMutex_INVENTR);
		}
	}

	return ret;
}


void SetLightLevel(RemoteControl_S control)
{
	u8 brightness = 0;

	if(control.interface > INTFC_DALI ||
	  (control.control_type == 2 && control.brightness > 100))
	{
		return;
	}

	if(control._switch == 0 ||
	   control.control_type == 1 ||
	  (control.control_type == 2 && control.brightness == 0))
	{
		delay_ms(100);
		
		RELAY_OFF;
		
		return;
	}
	else
	{
		RELAY_ON;

		if(control.interface == INTFC_DIGIT)
		{
			delay_ms(100);
		}
	}

#ifndef INTFC_FIXED
	cd4051b_set_channel(control.interface);
	delay_ms(10);
#endif

	if(control.control_type == 0 || control.control_type == 2)
	{
		if(control.control_type == 0)
		{
			brightness = 100;
		}
		else
		{
			brightness = control.brightness;
		}

		switch(control.interface)
		{
			case INTFC_0_10V:
				Dac1SetOutPutVoltage(((float)brightness / 100.0f) * (float)DAC_MAX_VOLATGE);
			break;

			case INTFC_PWM:
				PWMSetLightLevel(brightness * 2 * 10);
			break;

			case INTFC_DIGIT:
				InventrSetLightLevel(brightness * 2);
			break;

			default:

			break;
		}
	}
}









