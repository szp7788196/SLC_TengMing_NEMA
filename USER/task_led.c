#include "task_led.h"
#include "led.h"
#include "delay.h"
#include "task_net.h"
#include "internal.h"


TaskHandle_t xHandleTaskLED = NULL;
unsigned portBASE_TYPE LED_Satck;
u8 mem_used = 0;
void vTaskLED(void *pvParameters)
{
	u32 cnt = 0;
	u8 led_state = 0;
	
	while(1)
	{
		if(cnt % 50 == 0)					//ÿ��0.5��ι���Ź�
		{
			IWDG_Feed();
		}
		
		if(dev->state == STATE_REGISTERED)		//����״̬��ÿ3�����һ��
		{
			if(cnt % 300 == 0)
			{
				led_state = 1;
			}
			else
			{
				led_state = 0;
			}
		}
		else if(dev->state == STATE_REG_UPDATE_PENDING)		//���������У�ÿ3�����һ��
		{
			if(cnt % 100 == 0)
			{
				led_state = 1;
			}
			else
			{
				led_state = 0;
			}
		}
		else								//����״̬��������˸������600ms
		{
			if(cnt % 30 == 0)
			{
				led_state = ~led_state;
			}
		}
		
		if(led_state)
		{
			RUN_LED = 0;
		}
		else
		{
			RUN_LED = 1;
		}
		
		cnt = (cnt + 1) & 0xFFFFFFFF;
		
		mem_used = mem_perused();
		
		delay_ms(10);
		
//		LED_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}






































