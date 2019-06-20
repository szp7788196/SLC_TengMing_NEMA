#include "common.h"
#include "task_hci.h"
#include "delay.h"
#include "usart.h"
#include "inventr.h"
#include "at_protocol.h"
#include "task_sensor.h"


TaskHandle_t xHandleTaskHCI = NULL;
unsigned portBASE_TYPE HCI_Satck;
void vTaskHCI(void *pvParameters)
{
	u16 send_len1 = 0;
	
//	AT_CommandInit();
#ifdef DEBUG_LOG
	printf("READY\r\n");
#endif
	
	while(1)
	{
		if(Usart1RecvEnd == 0xAA)
		{
			Usart1RecvEnd = 0;
			
//			send_len1 = AT_CommandDataAnalysis(Usart1RxBuf,Usart1FrameLen,Usart1TxBuf,HoldReg);
			
			if(Usart1FrameLen == 4)
			{
				if(Usart1RxBuf[0] >= 0x30 && Usart1RxBuf[0] <= 0x39 &&
				   Usart1RxBuf[1] >= 0x30 && Usart1RxBuf[1] <= 0x39 &&
				   Usart1RxBuf[2] >= 0x30 && Usart1RxBuf[2] <= 0x39 &&
				   Usart1RxBuf[3] >= 0x30 && Usart1RxBuf[3] <= 0x39)
				{
					InputCurrent = (float)((Usart1RxBuf[0] - 0x30) * 1000 + 
								   (Usart1RxBuf[1] - 0x30) * 100 + 
					               (Usart1RxBuf[2] - 0x30) * 10 + 
					               (Usart1RxBuf[3] - 0x30));
				}
			}
			
			memset(Usart1RxBuf,0,Usart1FrameLen);
		}
		
		if(send_len1 != 0)
		{
			printf("%s",Usart1TxBuf);
			
			memset(Usart1TxBuf,0,send_len1);
			
			send_len1 = 0;
		}
		
		delay_ms(100);
		
		HCI_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}






































