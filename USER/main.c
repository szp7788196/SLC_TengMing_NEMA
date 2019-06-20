#include "common.h"
#include "rtos_task.h"
#include "24cxx.h"
#include "led.h"
#include "rtc.h"
#include "usart.h"
#include "uart4.h"
#include "pwm.h"
#include "dac.h"
#include "att7059x.h"
#include "inventr.h"
#include "dali.h"
#include "cd4051b.h"

u16 i = 0;
u8 j = 0;
u8 eepbuf[256];
RCC_ClocksTypeDef RCC_Clocks;

int main(void)
{
	SCB->VTOR = FLASH_BASE | 0x06000; 	/* Vector Table Relocation in Internal FLASH. */
//	IWDG_Init(IWDG_Prescaler_128,1600);	//128分频 312.5HZ 625为2秒

	RCC_GetClocksFreq(&RCC_Clocks);		//查看各个总线的时钟频率
	__set_PRIMASK(1);	//关闭全局中断

	NVIC_Configuration();
	delay_init(72);
	RTC_Init();
	AT24CXX_Init();
	DAC1_Init();
	LED_Init();
	RELAY_Init();
	CD4051B_Init();
	TIM2_Init(99,7199);
	TIM5_Int_Init(2000,36 - 1);
	USART1_Init(115200);
	USART3_Init(4800);
	USART2_Init(9600);
	UART4_Init(9600);

	__set_PRIMASK(0);	//开启全局中断

	AT24CXX_WriteOneByte(EC1_ADD,0);

//	for(i = 0; i < 256; i ++)
//	{
//		AT24CXX_WriteOneByte(i,i);
//	}
//	for(i = 0; i < 256; i ++)
//	{
//		eepbuf[i] = AT24CXX_ReadOneByte(i);
//	}
//	AT24CXX_WriteOneByte(UU_ID_ADD,255);

//	AT24CXX_WriteLenByte(E_FW_UPDATE_STATE_ADD + E_FW_UPDATE_STATE_LEN - 2,0xFFFF,2);	//恢复OTA状态信息

	mem_init();

	IWDG_Feed();				//喂看门狗

	ReadParametersFromEEPROM();	//读取所有的运行参数

	AppObjCreate();				//创建消息队列、互斥量
	AppTaskCreate();			//创建任务

	vTaskStartScheduler();		//启动调度，开始执行任务

	while(1)
	{
		delay_ms(100);
	}
}

























