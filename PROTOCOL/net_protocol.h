#ifndef __NET_PROTOCOL_H
#define __NET_PROTOCOL_H

#include "sys.h"
#include "m53xx.h"


#define MAX_FRAME_LEN	1024
#define MAX_PNFN_NUM	20

#define LAMPS_SWITCH_MAX_DAYS		366		//开关灯最大天数
#define LAMPS_MAX_NUM				1		//最大灯具数量
#define MAX_OPERATION_TIMES			30		//单个节能模式操作次数
#define MAX_ENERGY_SAVING_MODE_NUM	10		//节能模式的最大个数

#define ENCRYPTION_TYPE				0
#define ENCRYPTION_VERSION			1

typedef struct ControlMsg					//控制域
{
	u8 terminal_id[8];						//终端ID
	u8 terminal_type;						//终端类型

	u8 DIR;									//传输方向
	u8 PRM;									//启动标志位
	u8 ACD;									//要求访问位

	u8 encryption_type;						//加密类型
	u8 encryption_version;					//加密版本信息
	
	u16 frame_count;						//帧流水号
}ControlMsg_S;

typedef struct TimeTags						//时间标签
{
	u8 PFC;									//启动帧帧序号计数器
	u8 start_frame_time[4];					//启动帧发送时标
	u8 allow_timeout;						//允许发送传输延时时间
}TimeTags_S;

typedef struct AdditionalInformationAUX		//附加信息域
{
	u8 EC1;									//重要事件计数器
	u8 EC2;									//一般事件计数器

	TimeTags_S Tp;							//时间标签
}AdditionalInformationAUX_S;

typedef struct PnFn							//数据单元标识
{
	u16 pn;
	u16 fn;
}PnFn_S;

typedef struct DataUnit						//数据单元标识
{
	PnFn_S pn_fn;							//数据单元标识
	
	u8 *msg;								//数据指针
	u16 len;								//数据长度
}DataUnit_S;

typedef struct UserData						//用户数据
{
	u8 num;									//数据单元个数
	DataUnit_S data_unit[MAX_PNFN_NUM];		//数据单元
	
	AdditionalInformationAUX_S AUX;			//附加信息域
}UserData_S;

typedef struct UserDataSign					//用户数据
{
	u8 AFN;									//应用层功能码
	u8 SEQ;									//帧序列域

	u8 TpV;									//帧时间标签有效位
	u8 FIR;									//首帧标志
	u8 FIN;									//末帧标志
	u8 CON;									//请求确认标志
	u8 PSEQ;								//启动帧序号 PFC的低4位
	u8 RSEQ;								//响应帧序号
}UserDataSign_S;

typedef struct UpCommPortParameters			//上行通讯口通信参数
{
	u8 master_retry_times;					//主站等待从机超时后的重发次数
	u8 need_master_confirm;					//需要主站确认标志 D0:实时数据自动上报 D1:历史数据自动上报 D2:事件数据自动上报
	u8 heart_beat_cycle;					//心跳周期 分钟
	u16 wait_slave_rsp_timeout;				//终端等待从站响应的超时时间 秒
	u16 random_peak_staggering_time;		//随机错峰时间
	u16 specify_peak_staggering_time;		//指定错峰时间
}UpCommPortParameters_S;

typedef struct ServerInfo					//主站信息
{
	u8 *ip1;								//服务器主IP地址
	u8 *port1;								//服务器主端口
	u8 *apn;								//接入点名称
	u8 *user_name;							//用户名
	u8 *pwd;								//密码
	u8 *domain1;							//域名1
	u8 *ip2;								//服务器备用IP地址
	u8 *port2;								//服务器备用端口
	u8 *domain2;							//域名2
}ServerInfo_S;

typedef struct EventDetectConf				//事件记录配置设置数据
{
	u8 comm_falut_detect_interval;			//终端通信故障检测时间间隔
	u8 router_fault_detect_interval;		//集中器路由板故障检测时间间隔
	u8 turn_on_collect_delay;				//单灯正常开灯采集延时
	u8 turn_off_collect_delay;				//单灯正常关灯采集延时
	u8 current_detect_delay;				//单灯电流检测延时时间
	
	u8 over_current_ratio;					//单灯电流过大事件电流限值比值
	u8 over_current_recovery_ratio;			//单灯电流过大事件恢复电流限值比值
	u8 low_current_ratio;					//单灯电流过小事件电流限值比值
	u8 low_current_recovery_ratio;			//单灯电流过小事件恢复电流限值比值
	u8 capacitor_fault_pf_ratio[2];			//单灯电容故障事件故障功率因数限值
	u8 capacitor_fault_recovery_pf_ratio[2];//单灯电容故障事件故障恢复功率因数限值
	u8 lamps_over_current_ratio;			//单灯灯具故障事件电流限值
	u8 lamps_over_current_recovery_ratio;	//单灯灯具故障事件恢复电流限值
	u8 fuse_over_current_ratio;				//单灯熔丝故障事件电流限值
	u8 fuse_over_current_recovery_ratio;	//单灯熔丝故障事件恢复电流限值
	u8 leakage_over_current_ratio;			//单灯漏电故障事件电流限值
	u8 leakage_over_current_recovery_ratio;	//单灯漏电故障事件恢复电流限值
	u8 leakage_over_voltage_ratio;			//单灯漏电故障事件电压限值
	u8 leakage_over_voltage_recovery_ratio;	//单灯漏电故障事件恢复电压限值
}EventDetectConf_S;

typedef struct EventRecordConf				//事件记录配置设置数据
{
	u8 effective[8];						//事件记录有效标志位
	u8 important[8];						//事件重要性等级标志位
	u8 auto_report[8];						//事件主动上报标志位
}EventRecordConf_S;

typedef struct DeviceBaseInfo				//设备基本信息
{
	u8 id[6];								//控制器编号
	u8 mail_add[8];							//终端地址(通信地址)
	u8 longitude[5];						//经度
	u8 latitude[5];							//纬度
}DeviceBaseInfo_S;

typedef struct RSA_PublicKey				//RSA加密参数
{
	u8 enable;								//RSA启用标志
	u8 n[128];								//RSA公钥对中 n
	u8 el[128];								//RSA公钥对中 el
}RSA_PublicKey_S;

typedef struct SwitchTime					//开关灯时间
{
	u8 on_time[2];							//开灯时间
	u8 off_time[2];							//关灯时间
}SwitchTime_S;

typedef struct LampsSwitchProject						//每天开关灯
{
	u8 start_month;										//开始月
	u8 start_date;										//开始日
	u16 total_days;										//有效总天数
	
	SwitchTime_S switch_time[LAMPS_SWITCH_MAX_DAYS];	//每天的开关灯时间
}LampsSwitchProject_S;

typedef struct Parameters					//灯具基本参数(具体参数)
{
	u8 type;								//类型
	u8 enable;								//启用标志
	u8 line_id;								//出线序号
	u8 a_b_c;								//所处相别
	u16 lamps_id;							//序号
	u16 power;								//功率
	u16 pf;									//功率因数
	u16 pole_number;						//所在杆号
}Parameters_S;

typedef struct LampsParameters				//灯具基本参数
{
	u16 num;								//灯具个数
	
	Parameters_S parameters[LAMPS_MAX_NUM];	//每个灯的具体基本参数
}LampsParameters_S;

typedef struct RunMode						//灯具运行模式(具体模式)
{
	u8 initial_brightness;					//初始调光值
	u8 energy_saving_mode_id;				//节能模式编号
	u16 lamps_id;							//灯具序号
}RunMode_S;

typedef struct LampsRunMode					//灯具运行模式
{
	u8 num;									//灯具个数
	
	RunMode_S run_mode[LAMPS_MAX_NUM];		//每个灯的具体运行模式
}LampsRunMode_S;

typedef struct ActualOperation				//实际操作内容
{
	u8 mode;								//操作方式
	u8 control_type;						//控制类型
	u8 brightness;							//开关档位(亮度)
	u8 relative_time;						//相对时间(单位:5min)
	u8 absolute_time[6];					//绝对时间
}ActualOperation_S;

typedef struct EnergySavingMode							//节能模式
{
	u8 mode_id;											//模式编号
	u8 mode_name[32];									//模式名称
	u8 control_times;									//控制次数
	
	ActualOperation_S operation[MAX_OPERATION_TIMES];	//具体操作
}EnergySavingMode_S;

typedef struct AppointmentControl			//单灯预约控制
{
	u8 appointment_id;						//预约编号
	u8 start_date[6];						//开始日期
	u8 end_date[6];							//结束日期
	u8 lamps_num;							//被控灯具数量
	
	RunMode_S run_mode[LAMPS_MAX_NUM];		//控制模式
}AppointmentControl_S;

typedef struct IlluminanceThreshold			//光照度开关阈值
{
	u16 on;									//开灯阈值
	u16 off;								//关灯阈值
}IlluminanceThreshold_S;

typedef struct RemoteControl				//遥控命令
{
	u8 _switch;								//开关状态
	u8 lock;								//状态锁
	u8 control_type;						//控制类型 0:开 1:关 2:调光
	u8 brightness;							//开关档位(亮度)
	u8 interface;							//电源控制接口类型 0:0~10V 1:PWM 2:UART 3:DALI
	u16 lamps_id;							//灯具序号
	float current;							//当前电流
	float voltage;							//当前电压
}RemoteControl_S;

typedef struct SyncTime						//同步时间
{
	u8 enable;								//对时启用标志
	u8 date_time[6];						//日期和时间
}SyncTime_S;

typedef struct DeviceInfo					//设备信息
{
	u8 factory_code[4];						//厂商代号
	u8 factory_dev_id[8];					//厂商设备编号
	u8 software_ver[8];						//终端软件版本号
	u8 software_release_date[3];			//终端软件发布日期
	u8 protocol_ver[8];						//终端通讯协议版本号
	u8 hardware_ver[8];						//终端硬件版本号
	u8 device_model[8];						//终端型号
	u8 lamps_num_support;					//支持独立控制灯数
	u8 hardware_release_date[3];			//终端硬件发布日期
	u8 iccid[20];							//iccid
	u8 imsi[15];							//imsi
	u8 imei[15];							//imei
}DeviceInfo_S;

typedef struct DeviceElectricPara			//单灯电参数
{
	u8 initial_brightness;					//初始调光值
	u8 switch_state;						//开关状态 0开 1关 2调光
	u8 brightness;							//开关档位(亮度)
	u8 run_state;							//运行状态 0正常 1告警 2故障
	u8 volatge[2];							//电压
	u8 current[3];							//电流
	u8 active_power[2];						//有功功率
	u8 pf[2];								//功率因数
	u8 leakage_current[3];					//单灯漏电流
	u8 Illuminance[2];						//单灯光照度 默认为0
	u8 time_stamp[6];						//数据时间戳
}DeviceElectricPara_S;

typedef struct NB_ModulePara				//NB模块参数
{
	u8 csq;									//通信状态质量
	u8 band;								//频段
	u8 pci[2];								//基站
	u8 rsrp[2];								//信号强度
	u8 snr[2];								//信噪比
}NB_ModulePara_S;

typedef struct EventRecordList				//事件记录表
{
	u8 ec1;									//重要事件计数器
	u8 ec2;									//一般事件计数器
	u8 lable1[256];							//重要事件标签
	u8 lable2[256];							//一般事件标签
	
	u8 important_event_flag;				//系统中有重要事件标志
}EventRecordList_S;

typedef struct FTP_ServerInfo				//FTP升级服务器信息
{
	u8 *ip1;								//FTP服务器主IP地址
	u8 *port1;								//FTP服务器主端口
	u8 *ip2;								//FTP服务器备用IP地址
	u8 *port2;								//FTP服务器备用端口
	u8 *user_name;							//FTP用户名
	u8 *pwd;								//FTP密码
	u8 *path1;								//FTP服务器文件路径
	u8 *path2;								//FTP备用服务器文件路径
	u8 *file1;								//FTP文件名1
	u8 *file2;								//FTP文件名2
	u8 *file3;								//FTP文件名3
	u8 *file4;								//FTP文件名4
	u8 *file5;								//FTP文件名5
}FTP_ServerInfo_S;

typedef struct FTP_FrameWareInfo			//FTP升级固件信息
{
	u8 *name;								//固件名称
	u8 *version;							//固件版本
	u32 length;								//固件大小
}FTP_FrameWareInfo_S;



extern ControlMsg_S control_msg_in;
extern UserDataSign_S user_data_sign_in;
extern UserData_S user_data_in;

extern ControlMsg_S control_msg_out;
extern UserDataSign_S user_data_sign_out;
extern UserData_S user_data_out;

extern u8 LogInOutState;				//登录状态


void ResetFrameStruct(u8 mode,
                      ControlMsg_S *control_msg_in,
                      UserDataSign_S *user_data_sign_in,
                      UserData_S *user_data_in);
void GetControlMsgSign(u8 *inbuf);
void CombineControlMsg(u8 prm,u8 *c_msg);
void GetUserDataSign(u8 *inbuf);
u16 CombineUserData(u8 *u_data);
u16 CombineCompleteFrame(u8 prm, u8 *outbuf);
u16 MakeLogin_out_heartbeatFrame(u8 fn,u8 *outbuf);
void CombineDataUnitSign(u8 da1,u8 da2,u8 dt1,u8 dt2,u16 *pn,u16 *fn);
void SplitDataUnitSign(u16 pn,u16 fn,u8 *da1,u8 *da2,u8 *dt1,u8 *dt2);
void GetUserData(u8 *inbuf,u16 len);
u16 UserDataUnitHandle(void);
u16 NetDataFrameHandle(u8 *inbuf,u16 inbuf_len,u8 *outbuf);





































#endif
