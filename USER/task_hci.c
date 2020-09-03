#include "common.h"
#include "task_hci.h"
#include "delay.h"
#include "usart.h"
#include "inventr.h"
#include "at_protocol.h"
#include "task_sensor.h"
#include "stmflash.h"

u8 ret = 0;
TaskHandle_t xHandleTaskHCI = NULL;
unsigned portBASE_TYPE HCI_Satck;
void vTaskHCI(void *pvParameters)
{
	u16 send_len1 = 0;
	
//	AT_CommandInit();
#ifdef DEBUG_LOG
	printf("READY\r\n");
#endif
	
//	WriteDeviceBaseInfoFlash();
//	ret = ReadDeviceBaseInfoFlash();

//	WriteLampsRunModeFlash();
//	ret = ReadLampsRunModeFlash();
//	
//	WriteAppointmentControlFlash();
//	ret = ReadAppointmentControlFlash();
//	
//	WriteEnergySavingModeFlash(0);
//	ReadEnergySavingModeFlash();

	
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

//�˻��������ݽ���
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
			
			StrToHex(buf, (char*)inbuf + 11, 6);
			StrToHex(buf + 6, (char*)inbuf + 7, 8);
			
			if(buf[6] == 0x00 && 
			   buf[7] == 0x29 && 
			   buf[8] == 0x20 && 
			   buf[9] <= 0x99 && 
			   buf[10] <= 0x99 && 
			   buf[11] <= 0x99 && 
			   buf[12] <= 0x99 && 
			   buf[13] == 0x20)
			{
				memcpy(DeviceBaseInfo.id,buf + 0,6);
				memcpy(DeviceBaseInfo.mail_add,buf + 6,8);
				memcpy(DeviceBaseInfo.longitude,buf + 14,5);
				memcpy(DeviceBaseInfo.latitude,buf + 19,5);
				
				WriteDataFromMemoryToEeprom(buf,
					                        DEV_BASIC_INFO_ADD,
					                        DEV_BASIC_INFO_LEN - 2);	//����EEPROM
				
				WriteDeviceBaseInfoFlash();
				
				memset(temp_buf,0,32);
				HexToStr((char*)temp_buf,DeviceBaseInfo.mail_add,7);
				sprintf((char*)outbuf,"{\"LTUAddr\":\"%s\",",temp_buf);
				
				strcat((char*)outbuf,"\"No\":\"");
				memset(temp_buf,0,32);
				HexToStr(temp_buf,DeviceBaseInfo.id,6);
				strcat((char*)outbuf,temp_buf);
				strcat((char*)outbuf,"\",");
				
				strcat((char*)outbuf,"\"Type\":\"20\",");
				strcat((char*)outbuf,"\"Provider\":\"LK\",");
				
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



































