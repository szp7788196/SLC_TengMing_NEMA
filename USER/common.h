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


#define SOFT_WARE_VRESION				101			//软件版本号

#define FIRMWARE_FREE					0			//无需固件升级
#define FIRMWARE_DOWNLOADING			1			//固件正在下载中
#define FIRMWARE_DOWNLOAD_WAIT			2			//等待服务器下发固件
#define FIRMWARE_DOWNLOADED				3			//固件下载完成
#define FIRMWARE_DOWNLOAD_FAILED		4			//下载失败
#define FIRMWARE_UPDATING				5			//正在升级
#define FIRMWARE_UPDATE_SUCCESS			6			//升级成功
#define FIRMWARE_UPDATE_FAILED			7			//升级失败
#define FIRMWARE_ERASE_SUCCESS			8			//擦除FLASH成功
#define FIRMWARE_ERASE_FAIL				9			//擦除FLASH成功
#define FIRMWARE_ERASEING				10			//正在擦除FLASH
#define FIRMWARE_BAG_SIZE				258			//128 + 2字节crc
#define FIRMWARE_RUN_FLASH_BASE_ADD		0x08006000	//程序运行地址
#define FIRMWARE_BUCKUP_FLASH_BASE_ADD	0x08043000	//程序备份地址
#define FIRMWARE_MAX_FLASH_ADD			0x08080000	//FLSAH最大地址
#define FIRMWARE_SIZE					FIRMWARE_BUCKUP_FLASH_BASE_ADD - FIRMWARE_RUN_FLASH_BASE_ADD

#define DEVICE_TYPE					'G'			//设备类型
//#define DEBUG_LOG								//是否打印调试信息
#define EVENT_RECORD							//是否检测并记录

//#define INTFC_FIXED					INTFC_0_10V	//调光模式固定为0~10V


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

#define MAX_EVENT_NUM				256

#define SWITCH_ON_MIN_CURRENT		70.0f
#define SWITCH_OFF_MAX_CURRENT		70.0f

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

#define E_FTP_SERVER_INFO_ADD			7550	//FTP服务器信息
#define E_FTP_SERVER_INFO_LEN			214

#define E_FTP_FW_INFO_ADD				7764	//FTP固件信息
#define E_FTP_FW_INFO_LEN				44

#define E_FW_UPDATE_STATE_ADD			7808	//固件升级状态
#define E_FW_UPDATE_STATE_LEN			15

#define E_IMPORTEAT_ADD					8001	//重要事件记录SOE
#define E_NORMAL_ADD					14145	//一般事件记录SOE
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
extern time_t RTCTick1s;				//1s滴答时钟

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

/*************************固件升级状态***************************/
extern FrameWareState_S FrameWareState;


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
void SetRTCTick1s(time_t sec);
time_t GetRTCTick1s(void);
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
void WriteFrameWareStateToEeprom(void);
u8 ReadFrameWareState(void);
u8 UpdateSoftWareVer(void);
u8 UpdateSoftWareReleaseDate(void);
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
u8 CheckAppointmentControlValid(void);
u8 LookUpMatchedEnergySivingMode(u8 app_ctl);
void StrategyListFree(pControlStrategy head);
void StrategyListStateReset(pControlStrategy head,u8 mode);
u8 UpdateControlStrategyList(u8 app_ctl);

void RestoreFactorySettings(u8 mode);

void ReadParametersFromEEPROM(void);



void RemoveAllStrategy(void);































#endif
