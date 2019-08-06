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
			
			memset(Usart1TxBuf,0,Usart1TxLen);
			
			send_len1 = HCI_DataAnalysis(Usart1RxBuf,Usart1FrameLen,Usart1TxBuf);
			
			memset(Usart1RxBuf,0,Usart1FrameLen);
		}
		
		if(send_len1 != 0)
		{
			printf("%s",Usart1TxBuf);
			
			send_len1 = 0;
		}
		
		delay_ms(100);
		
		HCI_Satck = uxTaskGetStackHighWaterMark(NULL);
	}
}

//人机交互数据解析
u16 HCI_DataAnalysis(u8 *inbuf,u16 inbuf_len,u8 *outbuf)
{
	u16 ret = 0;
	u8 buf[DEV_BASIC_INFO_LEN];
	char temp_buf[32];
	
	if(inbuf_len == 25)
	{
		if(MyStrstr(inbuf, "AT+ADD=", inbuf_len, 7) != 0xFFFF)
		{
			memset(buf,0,DEV_BASIC_INFO_LEN);
			
			StrToHex(buf, (char*)inbuf + 7, 8);
			StrToHex(buf + 8, (char*)inbuf + 11, 6);
			
			if(buf[0] == 0x00 && 
			   buf[1] == 0x29 && 
			   buf[2] == 0x20 && 
			   buf[3] <= 0x99 && 
			   buf[4] <= 0x99 && 
			   buf[5] <= 0x99 && 
			   buf[6] <= 0x99 && 
			   buf[7] == 0x20)
			{
				memcpy(DeviceBaseInfo.id,buf + 0,6);					//更新设备基本信息
				memcpy(DeviceBaseInfo.mail_add,buf + 6,8);
				memcpy(DeviceBaseInfo.longitude,buf + 14,5);
				memcpy(DeviceBaseInfo.latitude,buf + 19,5);
				
				WriteDataFromMemoryToEeprom(buf,
					                        DEV_BASIC_INFO_ADD,
					                        DEV_BASIC_INFO_LEN - 2);	//存入EEPROM
				
				memset(temp_buf,0,32);
				HexToStr((char*)temp_buf,&buf[0],7);
				sprintf((char*)outbuf,"{\"LTUAddr\":\"%s\",",temp_buf);
				
				strcat((char*)outbuf,"\"No\":\"");
				memset(temp_buf,0,32);
				HexToStr(temp_buf,&buf[8],6);
				strcat((char*)outbuf,temp_buf);
				strcat((char*)outbuf,"\",");
				
				strcat((char*)outbuf,"\"Type\":\"20\",");
				strcat((char*)outbuf,"\"Provider\":\"NN\",");
				
				memset(temp_buf,0,32);
				memcpy(temp_buf,DeviceInfo.iccid,20);
				strcat((char*)outbuf,"\"ICCID\":\"");
				strcat((char*)outbuf,temp_buf);
				strcat((char*)outbuf,"\",");
				
				memset(temp_buf,0,32);
				memcpy(temp_buf,DeviceInfo.imei,15);
				strcat((char*)outbuf,"\"IMEI\":\"");
				strcat((char*)outbuf,temp_buf);
				strcat((char*)outbuf,"\",");
				
				memset(temp_buf,0,32);
				memcpy(temp_buf,DeviceInfo.imsi,15);
				strcat((char*)outbuf,"\"IMSI\":\"");
				strcat((char*)outbuf,temp_buf);
				strcat((char*)outbuf,"\"}");
				
				ret = strlen((char *)outbuf);
			}
		}
	}
	
	return ret;
}



































