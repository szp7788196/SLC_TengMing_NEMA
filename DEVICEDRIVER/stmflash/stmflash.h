#ifndef __STMFLASH_H__
#define __STMFLASH_H__

#include "sys.h"
#include "common.h"

#define DEV_BASIC_INFO_F_ADD				0
#define DEV_BASIC_INFO_F_LEN				26

#define LAMPS_NUM_MODE_F_ADD				26
#define LAMPS_NUM_MODE_F_LEN				24

#define APPOIN_CONTENT_F_ADD				50
#define APPOIN_CONTENT_F_LEN				36

#define E_SAVE_MODE_F_ADD					86
#define E_SAVE_MODE_F_LEN					336



u16 STMFLASH_ReadHalfWord(u32 faddr);
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);
u8 STMFLASH_ReadByte(u32 faddr);
void STMFLASH_ReadBytes(u32 ReadAddr,u8 *pBuffer,u16 NumToRead);
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);

u8 ReadDeviceBaseInfoFlash(void);
void WriteDeviceBaseInfoFlash(void);
u8 ReadLampsRunModeFlash(void);
void WriteLampsRunModeFlash(void);
u8 ReadAppointmentControlFlash(void);
void WriteAppointmentControlFlash(void);
u8 ReadEnergySavingModeFlash(void);
void WriteEnergySavingModeFlash(u8 j);




























#endif
