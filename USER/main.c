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

//u16 i = 0;
//u8 j = 0;
//u8 eepbuf[256];
RCC_ClocksTypeDef RCC_Clocks;

//u8 test_buf[87] = {
//0x68,0x4e,0x00,0x4e,0x00,0x68,0x00,0x29,0x20,0x01,
//0x00,0x00,0x00,0x20,0x40,0x01,0x06,0x00,0x04,0xe0,
//0x00,0x00,0x20,0x05,0x01,0x4e,0x6f,0x72,0x6d,0x61,
//0x6c,0x20,0x4d,0x6f,0x64,0x65,0x00,0x00,0x00,0x00,
//0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x01,0x02,
//0x7f,0x00,0x00,0x50,0x15,0x00,0x00,0x00,0x01,0x01,
//0x00,0x00,0x00,0x51,0x15,0x00,0x00,0x00,0x26,0x55,
//0x15,0x29,0x88,0x19,0x40,0xb8,0x16
//};

//u8 jjjj[100];

int main(void)
{
	SCB->VTOR = FLASH_BASE | 0x06000; 	/* Vector Table Relocation in Internal FLASH. */
	IWDG_Init(IWDG_Prescaler_128,1600);	//128分频 312.5HZ 625为2秒

	RCC_GetClocksFreq(&RCC_Clocks);		//查看各个总线的时钟频率
	__set_PRIMASK(1);					//关闭全局中断

	NVIC_Configuration();
	RELAY_Init();
	delay_init(72);
	RTC_Init();
	AT24CXX_Init();
	DAC1_Init();
	LED_Init();
	CD4051B_Init();
	TIM2_Init(99,7199);
	TIM5_Init(2000,36 - 1);
	USART1_Init(115200);
	USART3_Init(4800);
	USART2_Init(9600);
	UART4_Init(9600);

	__set_PRIMASK(0);	//开启全局中断

//	NetDataFrameHandle(test_buf,87,jjjj);

//	AT24CXX_WriteOneByte(EC1_ADD,0);

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

//	FrameWareState.state = FIRMWARE_FREE;
//	WriteFrameWareStateToEeprom();

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

























