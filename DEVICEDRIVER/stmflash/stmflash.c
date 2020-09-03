#include "stmflash.h"

u16 STMFLASH_BUF[1024];

u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}

void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

u8 STMFLASH_ReadByte(u32 faddr)
{
	return *(vu8*)faddr;
}

void STMFLASH_ReadBytes(u32 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	u16 i = 0;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadByte(ReadAddr);
		ReadAddr++;
	}
}

void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i=0;i<NumToWrite;i++)
	{
		FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
	    WriteAddr+=2;//地址增加2.
	}  
} 

u8 ReadDeviceBaseInfoFlash(void)
{
	u8 ret = 0;
	u16 i = 0;
	u16 crc16_cal = 0;
	u8 buf[DEV_BASIC_INFO_F_LEN];
	DeviceBaseInfo_S info;

	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	for(i = 0; i < DEV_BASIC_INFO_F_LEN / 2; i ++)
	{
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + DEV_BASIC_INFO_F_ADD + i * 2 + 1, &buf[i * 2 + 0],1);
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + DEV_BASIC_INFO_F_ADD + i * 2 + 0, &buf[i * 2 + 1],1);
	}
	
	FLASH_Lock();
	
	__set_PRIMASK(0);
	
	memcpy(&info,buf,DEV_BASIC_INFO_F_LEN);
	
	crc16_cal = GetCRC16(buf,DEV_BASIC_INFO_F_LEN - 2);
	
	if(crc16_cal == info.crc16)
	{
		memcpy(&DeviceBaseInfo,&info,DEV_BASIC_INFO_F_LEN);
		
		ret = 1;
	}
	
	return ret;
}


void WriteDeviceBaseInfoFlash(void)
{
	u8 i = 0;
	u8 buf[DEV_BASIC_INFO_F_LEN];
	
	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	STMFLASH_Read(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);
	
	FLASH_ErasePage(FIRMWARE_LAST_PAGE_ADD);
	
	DeviceBaseInfo.crc16 = GetCRC16((u8 *)&DeviceBaseInfo,DEV_BASIC_INFO_F_LEN - 2);

	memcpy(buf,&DeviceBaseInfo,DEV_BASIC_INFO_F_LEN);
	
	for(i = 0; i < DEV_BASIC_INFO_F_LEN / 2; i ++)
	{
		STMFLASH_BUF[DEV_BASIC_INFO_F_ADD / 2 + i] = ((((u16)buf[i * 2 + 0]) << 8) & 0xFF00) + (((u16)buf[i * 2 + 1]) & 0x00FF);
	}
	
	STMFLASH_Write_NoCheck(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);

	FLASH_Lock();
	
	__set_PRIMASK(0);
}

u8 ReadLampsRunModeFlash(void)
{
	u8 ret = 0;
	u16 i = 0;
	u16 crc16_cal = 0;
	u8 buf[LAMPS_NUM_MODE_F_LEN];
	LampsRunMode_S mode;
	
	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	for(i = 0; i < LAMPS_NUM_MODE_F_LEN / 2; i ++)
	{
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + LAMPS_NUM_MODE_F_ADD + i * 2 + 1, &buf[i * 2 + 0],1);
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + LAMPS_NUM_MODE_F_ADD + i * 2 + 0, &buf[i * 2 + 1],1);
	}
	
	FLASH_Lock();
	
	__set_PRIMASK(0);
	
	memcpy(&mode,buf,LAMPS_NUM_MODE_F_LEN);
	
	crc16_cal = GetCRC16(buf,LAMPS_NUM_MODE_F_LEN - 2);
	
	if(crc16_cal == mode.crc16)
	{
		memcpy(&LampsRunMode,&mode,LAMPS_NUM_MODE_F_LEN);
		
		ret = 1;
	}
	
	return ret;
}

void WriteLampsRunModeFlash(void)
{
	u8 i = 0;
	u8 buf[LAMPS_NUM_MODE_F_LEN];
	
	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	STMFLASH_Read(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);
	
	FLASH_ErasePage(FIRMWARE_LAST_PAGE_ADD);
	
	LampsRunMode.crc16 = GetCRC16((u8 *)&LampsRunMode,LAMPS_NUM_MODE_F_LEN - 2);

	memcpy(buf,&LampsRunMode,LAMPS_NUM_MODE_F_LEN);
	
	for(i = 0; i < LAMPS_NUM_MODE_F_LEN / 2; i ++)
	{
		STMFLASH_BUF[LAMPS_NUM_MODE_F_ADD / 2 + i] = ((((u16)buf[i * 2 + 0]) << 8) & 0xFF00) + (((u16)buf[i * 2 + 1]) & 0x00FF);
	}
	
	STMFLASH_Write_NoCheck(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);

	FLASH_Lock();
	
	__set_PRIMASK(0);
}

u8 ReadAppointmentControlFlash(void)
{
	u8 ret = 0;
	u16 i = 0;
	u16 crc16_cal = 0;
	u8 buf[APPOIN_CONTENT_F_LEN];
	AppointmentControl_S control;
	
	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	for(i = 0; i < APPOIN_CONTENT_F_LEN / 2; i ++)
	{
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + APPOIN_CONTENT_F_ADD + i * 2 + 1, &buf[i * 2 + 0],1);
		STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + APPOIN_CONTENT_F_ADD + i * 2 + 0, &buf[i * 2 + 1],1);
	}
	
	FLASH_Lock();
	
	__set_PRIMASK(0);
	
	memcpy(&control,buf,APPOIN_CONTENT_F_LEN);
	
	crc16_cal = GetCRC16(buf,APPOIN_CONTENT_F_LEN - 2);
	
	if(crc16_cal == control.crc16)
	{
		memcpy(&AppointmentControl,&control,APPOIN_CONTENT_F_LEN);
		
		ret = 1;
	}
	
	return ret;
}

void WriteAppointmentControlFlash(void)
{
	u8 i = 0;
	u8 buf[APPOIN_CONTENT_F_LEN];
	
	__set_PRIMASK(1);
	
	FLASH_Unlock();
	
	STMFLASH_Read(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);
	
	FLASH_ErasePage(FIRMWARE_LAST_PAGE_ADD);
	
	AppointmentControl.crc16 = GetCRC16((u8 *)&AppointmentControl,APPOIN_CONTENT_F_LEN - 2);

	memcpy(buf,&AppointmentControl,APPOIN_CONTENT_F_LEN);
	
	for(i = 0; i < APPOIN_CONTENT_F_LEN / 2; i ++)
	{
		STMFLASH_BUF[APPOIN_CONTENT_F_ADD / 2 + i] = ((((u16)buf[i * 2 + 0]) << 8) & 0xFF00) + (((u16)buf[i * 2 + 1]) & 0x00FF);
	}
	
	STMFLASH_Write_NoCheck(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);

	FLASH_Lock();
	
	__set_PRIMASK(0);
}

u8 ReadEnergySavingModeFlash(void)
{
	u8 ret = 0;
	u16 i = 0;
	u8 j = 0;
	u16 crc16_cal = 0;
	u8 buf[E_SAVE_MODE_F_LEN];
	
	EnergySavingModeNumFlash = 0;
	
	__set_PRIMASK(1);
	
	for(j = 0; j < MAX_ENERGY_SAVING_MODE_NUM; j ++)
	{
		EnergySavingMode_S mode;
		
		FLASH_Unlock();
	
		for(i = 0; i < E_SAVE_MODE_F_LEN / 2; i ++)
		{
			STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + E_SAVE_MODE_F_ADD + E_SAVE_MODE_F_LEN * j + i * 2 + 1, &buf[i * 2 + 0],1);
			STMFLASH_ReadBytes(FIRMWARE_LAST_PAGE_ADD + E_SAVE_MODE_F_ADD + E_SAVE_MODE_F_LEN * j + i * 2 + 0, &buf[i * 2 + 1],1);
		}
		
		FLASH_Lock();
		
		memcpy(&mode,buf,E_SAVE_MODE_F_LEN);
		
		crc16_cal = GetCRC16(buf,E_SAVE_MODE_F_LEN - 2);
		
		if(crc16_cal == mode.crc16)
		{
			memcpy(&EnergySavingMode[j],&mode,E_SAVE_MODE_F_LEN);
			
			EnergySavingModeNumFlash ++;
			
			ret = 1;
		}
	}
	
	__set_PRIMASK(0);
	
	return ret;
}

void WriteEnergySavingModeFlash(u8 index)
{
	u16 i = 0;
	u8 buf[E_SAVE_MODE_F_LEN];
	
	if(index < MAX_ENERGY_SAVING_MODE_NUM)
	{
		__set_PRIMASK(1);
		
		FLASH_Unlock();
	
		STMFLASH_Read(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);
			
		FLASH_ErasePage(FIRMWARE_LAST_PAGE_ADD);
		
		EnergySavingMode[index].crc16 = GetCRC16((u8 *)&EnergySavingMode[index],E_SAVE_MODE_F_LEN - 2);

		memcpy(buf,&EnergySavingMode[index],E_SAVE_MODE_F_LEN);
		
		for(i = 0; i < E_SAVE_MODE_F_LEN / 2; i ++)
		{
			STMFLASH_BUF[(E_SAVE_MODE_F_ADD / 2) + ((E_SAVE_MODE_F_LEN / 2) * index) + i] = ((((u16)buf[i * 2 + 0]) << 8) & 0xFF00) + (((u16)buf[i * 2 + 1]) & 0x00FF);
		}
		
		STMFLASH_Write_NoCheck(FIRMWARE_LAST_PAGE_ADD,STMFLASH_BUF,1024);
		
		FLASH_Lock();
		
		__set_PRIMASK(0);
	}
}




































































