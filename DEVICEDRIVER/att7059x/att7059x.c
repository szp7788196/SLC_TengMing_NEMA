#include "att7059x.h"
#include "usart.h"
#include "delay.h"
#include "string.h"

//����У����
u8 CalAtt7059CheckSum(u8 *buf, u8 len)
{
	u8 i = 0;
	u8 check_sum = 0;

	for(i = 0; i < len; i ++)
	{
		check_sum += buf[i];
	}

	check_sum = ~check_sum;

	return check_sum;
}

//��7059д������
u8 Att7059xWriteOperate(u8 add, u16 data)
{
	u8 ret = 0;
	u8 i = 0;
	u8 send_len = 0;
	u8 send_buf[6];

	send_buf[send_len] = 0x6A;
	send_len ++;
	send_buf[send_len] = 0x80 | add;
	send_len ++;
	send_buf[send_len] = (u8)(data >> 8);
	send_len ++;
	send_buf[send_len] = (u8)(data & 0x00FF);
	send_len ++;
	send_buf[send_len] = 0;
	send_len ++;

	send_buf[send_len] = CalAtt7059CheckSum(send_buf, send_len);

	UsartSendString(USART3,send_buf, send_len);

	i = 100;
	while(i --)
	{
		delay_ms(10);

		if(Usart3RecvEnd == 0xAA)
		{
			Usart3RecvEnd = 0;

			if(Usart3FrameLen == 1)
			{
				Usart3FrameLen = 0;

				if(Usart3RxBuf[0] == ACK_OK)
				{
					i = 0;
					ret = 1;
				}
			}
		}
	}

	return ret;
}

//��7059�ж�ȡ����
u8 Att7059xReadOperate(u8 add, u32 *data)
{
	u8 ret = 0;
	u8 i = 0;
	u8 send_len = 0;
	u8 send_buf[6];
	u8 check_sum = 0;

	*data = 0;

	send_buf[send_len] = 0x6A;
	send_len ++;
	send_buf[send_len] = 0x00 | add;
	send_len ++;

	UsartSendString(USART3,send_buf, send_len);

	i = 100;
	while(i --)
	{
		delay_ms(10);

		if(Usart3RecvEnd == 0xAA)
		{
			Usart3RecvEnd = 0;

			if(Usart3FrameLen == 4)
			{
				memcpy(&send_buf[2],Usart3RxBuf,Usart3FrameLen);

				check_sum = CalAtt7059CheckSum(send_buf, 5);

				if(check_sum == Usart3RxBuf[Usart3FrameLen - 1])
				{
					*data = (((u32)Usart3RxBuf[0]) << 16) + \
							(((u32)Usart3RxBuf[1]) << 8) + \
							(u32)Usart3RxBuf[2];

					i = 0;
					ret = 1;
				}

				Usart3FrameLen = 0;
			}
		}
	}

	return ret;
}

//��ȡ����ͨ��1��ADC��������
s32 Att7059xGetCurrent1ADCValue(void)
{
	static s32 adc_value = 0;
	s32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x00, (u32 *)&value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		if(value & (1 << 23))
		{
			value = value - 0x00FFFFFF;
		}
	}

	return adc_value;
}

//��ȡ��ѹͨ����ADC��������
s32 Att7059xGetVoltageADCValue(void)
{
	static s32 adc_value = 0;
	s32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x02, (u32 *)&value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		if(value & (1 << 23))
		{
			value = value - 0x00FFFFFF;
		}
	}

	return adc_value;
}

//��ȡ����ͨ��1����Чֵ
float Att7059xGetCurrent1(void)
{
	static u32 current = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x06, &value);

	if(ret == 1)
	{
		current = (float)value * CURRENT_RATIO;
	}

	return current;
}

//��ȡ��ѹͨ������Чֵ
float Att7059xGetVoltage(void)
{
	static u32 voltage = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x08, &value);

	if(ret == 1)
	{
		voltage = (float)value * VOLTAGE_RATIO;
	}

	return voltage;
}

//��ȡ��ѹͨ����Ƶ��
float Att7059xGetVoltageFreq(void)
{
	static float freq_value = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x09, &value);

	if(ret == 1)
	{
		freq_value = 1000000.0f / 2.0f / (float)value;
	}

	return freq_value;
}

//��ȡͨ��1���й�����
float Att7059xGetChannel1PowerP(void)
{
	static float power_p = 0;
	s32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0A, (u32 *)&value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		if(value & (1 << 23))
		{
			value = value - 0x00FFFFFF;
		}

		power_p = (float)value * POWER_RATIO;
	}

	return power_p;
}

//��ȡͨ��1���޹�����
float Att7059xGetChannel1PowerQ(void)
{
	static float power_q = 0;
	s32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0B, (u32 *)&value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		if(value & (1 << 23))
		{
			value = value - 0x00FFFFFF;
		}

		power_q = (float)value * POWER_RATIO;
	}

	return power_q;
}

//��ȡͨ��1�����ڹ���
float Att7059xGetChannel1PowerS(void)
{
	static float power_s = 0;
	s32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0C, (u32 *)&value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		if(value & (1 << 23))
		{
			value = value - 0x00FFFFFF;
		}

		power_s = (float)value * POWER_RATIO;
	}

	return power_s;
}

//��ȡ�й����� ��λǧ��
float Att7059xGetEnergyP(void)
{
	static float energy_p = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0D, &value);

	if(ret == 1)
	{
		energy_p = (float)value / ELECTRIC_ENERGY_METER_CONSTANT;
	}

	return energy_p;
}

//��ȡ�޹����� ��λǧ��
float Att7059xGetEnergyQ(void)
{
	static float energy_q = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0E, &value);

	if(ret == 1)
	{
		energy_q = (float)value / ELECTRIC_ENERGY_METER_CONSTANT;
	}

	return energy_q;
}

//��ȡ�������� ��λǧ��
float Att7059xGetEnergyS(void)
{
	static float energy_s = 0;
	u32 value = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x0F, &value);

	if(ret == 1)
	{
		energy_s = (float)value / ELECTRIC_ENERGY_METER_CONSTANT;
	}

	return energy_s;
}

//��ȡ��ѹͨ����ADC����������
s32 Att7059xGetMaxVoltageADCValue(void)
{
	static s32 adc_value = 0;
	u32 value = 0;
	u8 sign = 0;
	u8 ret = 0;

	ret = Att7059xReadOperate(0x12, &value);

	if(ret == 1)								//������ת��ΪԴ��
	{
		value |= 0xFF000000;					//��8λǿ��Ϊ1

		if(value & (1 << 21))
		{
			sign = 1;							//����
		}
		else
		{
			sign = 0;							//����
		}

		value |= 0x00E00000;					//bit21~23ǿ��Ϊ1

		value = ~value;

		if(sign)
		{
			value |= ((u32)1 << 31);			//����
		}
		else
		{
			value &= ~((u32)1 << 31);			//����
		}

		adc_value = (s32)value;
	}

	return adc_value;
}
















































