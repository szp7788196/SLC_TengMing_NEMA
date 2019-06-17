#ifndef __COMMON_H
#define __COMMON_H

#include "stm32f10x.h"
#include "string.h"
#include "sys.h"
#include "delay.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <ctype.h>
#include "malloc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"

#include <time.h>

#include "task_sensor.h"
#include "rtc.h"
#include "platform.h"

#include "net_protocol.h"

/*---------------------------------------------------------------------------*/
/* Type Definition Macros                                                    */
/*---------------------------------------------------------------------------*/
#ifndef __WORDSIZE
  /* Assume 32 */
  #define __WORDSIZE 32
#endif

    typedef unsigned char   uint8;
    typedef char            int8;
    typedef unsigned short  uint16;
    typedef short           int16;
    typedef unsigned int    uint32;
    typedef int             int32;

#ifdef WIN32
    typedef int socklen_t;
#endif

    typedef unsigned long long int  uint64;
    typedef long long int           int64;


#define SOFT_WARE_VRESION			101			//软件版本号

#define DEVICE_TYPE					'G'			//设备类型
#define DEBUG_LOG								//是否打印调试信息
#define NEW_BOARD

#define INTFC_FIXED					INTFC_0_10V	//调光模式固定为0~10V


#define INTFC_0_10V					0			//0~10V调光
#define INTFC_PWM					1			//PWM调光
#define INTFC_DIGIT					2			//数字调光
#define INTFC_DALI					3			//DALI调光

#define INIT_LIGHT_LEVEL			LightLevelPercent

#define EVENT_ERC15					15
#define EVENT_ERC16					16
#define EVENT_ERC17					17
#define EVENT_ERC18					18
#define EVENT_ERC19					19
#define EVENT_ERC20					20
#define EVENT_ERC21					21
#define EVENT_ERC22					22
#define EVENT_ERC23					23
#define EVENT_ERC28					28
#define EVENT_ERC36					36
#define EVENT_ERC37					37
#define EVENT_ERC51					51
#define EVENT_ERC52					52

#define SWITCH_ON_MIN_CURRENT		10.0f
#define SWITCH_OFF_MAX_CURRENT		20.0f

#define WAIT_EXECUTE				0		//等待执行
#define EXECUTING					1		//正在执行
#define EXECUTED					2		//已经执行完,已过期


//EEPROM存储数据地址列表
#define	UP_COMM_PARA_ADD				1		//上行通讯口通讯参数
#define	UP_COMM_PARA_LEN				10

#define SERVER_INFO_ADD					11		//主站信息
#define SERVER_INFO_LEN					158

#define ER_TIME_CONF_ADD				169		//事件记录时间参数配置
#define ER_TIME_CONF_LEN				7

#define ER_THRE_CONF_ADD				176		//事件记录时间参数配置
#define ER_THRE_CONF_LEN				18

#define ER_OTHER_CONF_ADD				194		//事件记录配置信息
#define ER_OTHER_CONF_LEN				26

#define DEV_BASIC_INFO_ADD				220		//设备基本信息
#define DEV_BASIC_INFO_LEN				26

#define SWITCH_MODE_ADD					246		//开关模式选择
#define SWITCH_MODE_LEN					3

#define RSA_ENCRY_PARA_ADD				256		//RSA加密参数
#define RSA_ENCRY_PARA_LEN				259

#define SWITCH_DATE_DAYS_ADD			515		//开关灯起始日期和天数
#define SWITCH_DATE_DAYS_LEN			6
#define SWITCH_TIME_ADD					521		//具体开关灯时间 共366组
#define SWITCH_TIME_LEN					6

#define LAMPS_NUM_PARA_ADD				2717	//灯具个数(灯具参数)
#define LAMPS_NUM_PARA_LEN				4
#define LAMPS_PARA_ADD					2721	//灯具参数	共10组
#define LAMPS_PARA_LEN					14

#define LAMPS_NUM_MODE_ADD				2862	//灯具个数(运行模式)
#define LAMPS_NUM_MODE_LEN				3
#define LAMPS_MODE_ADD					2865	//灯具运行模式 共10组
#define LAMPS_MODE_LEN					6

#define E_SAVE_MODE_NUM_ADD				2925	//节能模式个数
#define E_SAVE_MODE_NUM_LEN				3
#define E_SAVE_MODE_LEN					396		//每个节能模式的长度
#define E_SAVE_MODE_LABLE_ADD			2928	//节能模式标签 共10组
#define E_SAVE_MODE_LABLE_LEN			36
#define E_SAVE_MODE_CONTENT_ADD			2964	//节能模式内容 每个节能模式组包含30组节能模式内容
#define E_SAVE_MODE_CONTENT_LEN			12

#define APPOIN_LABLE_ADD				6868	//预约控制标签
#define APPOIN_LABLE_LEN				16
#define APPOIN_CONTENT_ADD				6884	//预约控制内容 共10组
//#define APPOIN_CONTENT_LEN				6
#define APPOIN_CONTENT_LEN				5

#define ILLUM_THRE_ADD					6944	//光照度阈值
#define ILLUM_THRE_LEN					6

#define REMOTE_CTL_ADD					6950	//遥控命令
#define REMOTE_CTL_LEN					6

#define FACTORY_CODE_ADD				6956	//厂商代号
#define FACTORY_CODE_LEN				6

#define FACTORY_DEV_ID_ADD				6962	//厂商设备编号
#define FACTORY_DEV_ID_LEN				10

#define SW_VERSION_ADD					6972	//终端软件版本号
#define SW_VERSION_LEN					10

#define SW_RE_DATE_ADD					6982	//终端软件发布日期
#define SW_RE_DATE_LEN					5

#define PROTOCOL_VERSION_ADD			6987	//终端通信协议版本号
#define PROTOCOL_VERSION_LEN			10

#define HW_VERSION_ADD					6997	//终端硬件版本号
#define HW_VERSION_LEN					10

#define DEV_MODEL_ADD					7007	//终端型号
#define DEV_MODEL_LEN					10

#define HW_RE_DATE_ADD					7017	//终端硬件发布日期
#define HW_RE_DATE_LEN					10

#define SUP_LAMPS_NUM_ADD				7022	//支持灯数
#define SUP_LAMPS_NUM_LEN				3

#define EC1_ADD							7025	//重要事件计数器EC1
#define EC1_LEN							3

#define EC2_ADD							7028	//一般事件计数器EC2
#define EC2_LEN							3

#define EC1_LABLE_ADD					7031	//重要事件标签数组
#define EC1_LABLE_LEN					258

#define EC2_LABLE_ADD					7289	//重要事件标签数组
#define EC2_LABLE_LEN					258

#define E_IMPORTANT_FLAG_ADD			7547	//重要事件标志
#define E_IMPORTANT_FLAG_LEN			3

#define FTP_SERVER_INFO_ADD				7550	//FTP服务器信息
#define FTP_SERVER_INFO_LEN				214

#define FTP_FW_INFO_ADD					7764	//FTP固件信息
#define FTP_FW_INFO_LEN					44

#define E_IMPORTEAT_ADD					8001	//重要事件记录SOE
#define E_NORMAL_ADD					14145	//重要事件记录SOE
#define EVENT_LEN						24



#define ControlStrategy_S struct ControlStrategy
typedef struct ControlStrategy *pControlStrategy;
struct ControlStrategy
{
	u8 mode;				//操作方式
	u8 type;				//控制类型
	u8 brightness;			//亮度

	u8 year;				//绝对时间年
	u8 month;
	u8 date;
	u8 hour;
	u8 minute;
	u8 second;
	
	time_t time_re;			//相对时间 单位min

	u8 state;				//策略状态 等待执行 正在执行 已过期
	
	pControlStrategy prev;
	pControlStrategy next;
};


static const uint32_t crc32tab[] = 
{
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
	0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
	0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
	0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
	0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
	0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
	0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
	0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
	0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
	0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
	0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
	0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
	0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
	0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
	0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
	0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
	0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
	0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
	0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
	0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
	0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
	0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
	0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
	0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
	0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
	0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
	0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL 
};

static u8 auchCRCHi[] = 
{
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40
    } ;
    /* CRC低位字节值表*/
static u8 auchCRCLo[] = 
{
    0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,
    0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,
    0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
    0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,
    0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,
    0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
    0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,
    0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
    0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
    0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,
    0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,
    0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
    0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,
    0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,
    0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
    0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
    0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,
    0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
    0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,
    0x70,0xB0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
    0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
    0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,
    0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,
    0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
    0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
    0x43,0x83,0x41,0x81,0x80,0x40
};

//extern u8 HoldReg[HOLD_REG_LEN];			//保持寄存器 临时使用

extern SemaphoreHandle_t  xMutex_IIC1;			//IIC1的互斥量
extern SemaphoreHandle_t  xMutex_INVENTR;		//英飞特电源的互斥量
extern SemaphoreHandle_t  xMutex_AT_COMMAND;	//AT指令的互斥量
extern SemaphoreHandle_t  xMutex_STRATEGY;		//AT指令的互斥量
extern SemaphoreHandle_t  xMutex_EVENT_RECORD;	//事件记录的互斥量

extern QueueHandle_t xQueue_sensor;				//用于存储传感器的数据

/***************************系统心跳相关*****************************/
extern u32 SysTick1ms;					//1ms滴答时钟
extern u32 SysTick10ms;					//10ms滴答时钟
extern u32 SysTick100ms;				//10ms滴答时钟
extern time_t SysTick1s;				//1s滴答时钟

extern u8 GetTimeOK;						//成功获时间标志
extern pControlStrategy ControlStrategy;	//控制策略
extern u8 NeedUpdateStrategyList;			//更新策略列表标志

/***************************复位参数*****************************/
extern u8 DeviceReset;					//1:设备重启 2:除网络参数外，均恢复到出厂设置 3:恢复出厂设置
extern u8 ReConnectToServer;			//重新连接服务器

/********************终端上行通信口通信参数**********************/
extern UpCommPortParameters_S UpCommPortPara;

/*************************主站IP和端口***************************/
extern ServerInfo_S ServerInfo;

/**********************终端事件记录配置参数***********************/
extern EventRecordConf_S EventRecordConf;

/************************设备基本信息数据*************************/
extern DeviceBaseInfo_S DeviceBaseInfo;

/**********************终端事件检测配置参数***********************/
extern EventDetectConf_S EventDetectConf;

/**************************RSA加密参数****************************/
extern RSA_PublicKey_S RSA_PublicKey;

/*********************控制器开关灯时间数据************************/
extern LampsSwitchProject_S LampsSwitchProject;

/*************************灯具基本参数****************************/
extern LampsParameters_S LampsParameters;

/***********************单灯运行模式数据**************************/
extern LampsRunMode_S LampsRunMode;

/*********************单灯节能运行模式数据************************/
extern EnergySavingMode_S EnergySavingMode[MAX_ENERGY_SAVING_MODE_NUM];

/***********************单灯预约控制数据**************************/
extern AppointmentControl_S AppointmentControl;

/*********************单灯照度开关阈值数据************************/
extern IlluminanceThreshold_S IlluminanceThreshold;

/*********************单灯开关模式选择数据************************/
extern u8 SwitchMode;	//开关模式 1:年表控制 2:经纬度控制 3:光照度控制 4:常亮(常开)

/***********************单灯遥控操作数据*************************/
extern RemoteControl_S RemoteControl;
extern RemoteControl_S CurrentControl;

/*************************对时命令数据***************************/
extern SyncTime_S SyncTime;

/*************************终端信息数据***************************/
extern DeviceInfo_S DeviceInfo;

/**************************单灯电参数****************************/
extern DeviceElectricPara_S DeviceElectricPara;

/**************************NB模块参数****************************/
extern NB_ModulePara_S NB_ModulePara;

/**************************终端日历时钟****************************/
extern u8 CalendarClock[6];

/**************************事件计数器****************************/
extern EventRecordList_S EventRecordList;

/***********************FTP服务器配置参数************************/
extern FTP_ServerInfo_S FTP_ServerInfo;

/***************************固件参数*****************************/
extern FTP_FrameWareInfo_S FTP_FrameWareInfo;


u16 MyStrstr(u8 *str1, u8 *str2, u16 str1_len, u16 str2_len);
u8 GetDatBit(u32 dat);
u32 GetADV(u8 len);
void IntToString(u8 *DString,u32 Dint,u8 zero_num);
u32 StringToInt(u8 *String);
void HexToStr(char *pbDest, u8 *pbSrc, u16 len);
void StrToHex(u8 *pbDest, char *pbSrc, u16 len);
u16 get_day_num(u8 m,u8 d);
void get_date_from_days(u16 days, u8 *m, u8 *d);
s16 get_dates_diff(u8 m1,u8 d1,u8 m2,u8 d2);
unsigned short find_str(unsigned char *s_str, unsigned char *p_str, unsigned short count, unsigned short *seek);
int search_str(unsigned char *source, unsigned char *target);
unsigned short get_str1(unsigned char *source, unsigned char *begin, unsigned short count1, unsigned char *end, unsigned short count2, unsigned char *out);
unsigned short get_str2(unsigned char *source, unsigned char *begin, unsigned short count, unsigned short length, unsigned char *out);
unsigned short get_str3(unsigned char *source, unsigned char *out, unsigned short length);
u32 CRC32( const u8 *buf, u32 size);
u16 CRC16(u8 *puchMsgg,u16 usDataLen,u8 mode);
u16 GetCRC16(u8 *data,u16 len);
u8 CalCheckSum(u8 *buf, u16 len);

void SysTick1msAdder(void);
u32 GetSysTick1ms(void);
void SysTick10msAdder(void);
u32 GetSysTick10ms(void);
void SysTick100msAdder(void);
u32 GetSysTick100ms(void);
void SetSysTick1s(time_t sec);
time_t GetSysTick1s(void);
u8 GetRTC_State(void);


u8 ReadDataFromEepromToMemory(u8 *buf,u16 s_add, u16 len);
void WriteDataFromMemoryToEeprom(u8 *inbuf,u16 s_add, u16 len);
u8 GetMemoryForString(u8 **str, u8 type, u32 id, u16 add, u16 size, u8 *hold_reg);
u8 GetMemoryForSpecifyPointer(u8 **str,u16 size, u8 *memory);
u8 GetIpAdderssFromMemory(u8 **ip,u8 *memory);
u8 GetPortFromMemory(u8 **port,u8 *memory);
u8 CopyStrToPointer(u8 **pointer, u8 *str, u8 len);




u8 ReadUpCommPortPara(void);
u8 ReadServerInfo(void);
u8 ReadEventRecordConf(void);
u8 ReadDeviceBaseInfo(void);
u8 ReadEventDetectTimeConf(void);
u8 ReadEventDetectThreConf(void);
u8 ReadRSA_PublicKey(void);
u8 ReadLampsSwitchProject(void);
u8 ReadLampsParameters(void);
u8 ReadLampsRunMode(void);
u8 ReadEnergySavingMode(void);
u8 ReadAppointmentControl(void);
u8 ReadIlluminanceThreshold(void);
u8 ReadSwitchMode(void);
void ResetRemoteCurrentControl(void);
u8 ReadFTP_ServerInfo(void);
u8 ReadFTP_FrameWareInfo(void);
u8 ReadFactoryCode(void);
u8 ReadFactoryDeviceId(void);
u8 ReadSoftWareVer(void);
u8 ReadSoftWareReleaseDate(void);
u8 ReadProtocolVer(void);
u8 ReadHardWareVer(void);
u8 ReadDeviceModel(void);
u8 ReadHardWareReleaseDate(void);
u8 ReadLamosNumSupport(void);
u8 ReadEventRecordList(void);
u8 LookUpMatchedEnergySivingMode(void);
void StrategyListFree(pControlStrategy head);
void StrategyListStateReset(pControlStrategy head);
u8 UpdateControlStrategyList(void);



void ReadParametersFromEEPROM(void);



void RemoveAllStrategy(void);































#endif
