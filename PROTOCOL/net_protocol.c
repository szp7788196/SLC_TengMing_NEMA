#include "net_protocol.h"
#include "rtc.h"
#include "usart.h"
#include "24cxx.h"
#include "common.h"
#include "task_net.h"
#include "error.h"
#include "utils.h"
#include "event.h"
#include "task_main.h"


ControlMsg_S control_msg_in;
UserDataSign_S user_data_sign_in;
UserData_S user_data_in;

ControlMsg_S control_msg_out;
UserDataSign_S user_data_sign_out;
UserData_S user_data_out;

u8 LogInOutState = 0x00;				//登录状态

//复位报文结构体变量
void ResetFrameStruct(u8 mode,
                      ControlMsg_S *control_msg_in,
                      UserDataSign_S *user_data_sign_in,
                      UserData_S *user_data_in)
{
	u8 i = 0;
	
	//控制域
	memset(control_msg_in->terminal_id,0,7);				//终端ID

	control_msg_in->terminal_type = 0;						//终端类型

	if(mode == 1)
	{
		control_msg_in->frame_count = 0;					//帧流水号
	}

	control_msg_in->DIR = 0;								//传输方向
	control_msg_in->PRM = 0;								//启动标志位
	control_msg_in->ACD = 0;								//请求访问位

	control_msg_in->encryption_type = 0;					//加密类型
	control_msg_in->encryption_version = 0;					//加密版本信息

	//用户数据
	user_data_sign_in->AFN = 0xFF;							//应用层功能码
	user_data_sign_in->SEQ = 0;								//帧序列域

	user_data_sign_in->TpV = 0;								//帧时间标签有效位
	user_data_sign_in->FIR = 0;								//首帧标志
	user_data_sign_in->FIN = 0;								//末帧标志
	user_data_sign_in->CON = 0;								//请求确认标志

	if(mode == 1)
	{
		user_data_sign_in->PSEQ = 0;						//启动帧序号
		user_data_sign_in->RSEQ = 0;						//响应帧序号
	}

	user_data_in->num = 0;									//数据单元标识数量

	if(mode == 1)
	{
		user_data_in->AUX.EC1 = 0;							//重要事件计数器
		user_data_in->AUX.EC2 = 0;							//一般事件计数器

		user_data_in->AUX.Tp.PFC = 0;						//启动帧帧序号计数器
	}
	
	for(i = 0; i < MAX_PNFN_NUM; i ++)						//清空数据单元标识和数据单元
	{
		user_data_in->data_unit[i].pn_fn.pn = 0;
		user_data_in->data_unit[i].pn_fn.fn = 0;
		user_data_in->data_unit[i].len = 0;
		user_data_in->data_unit[i].msg = NULL;
	}

	memset(user_data_in->AUX.Tp.start_frame_time,0,4);		//启动帧发送时标
	user_data_in->AUX.Tp.allow_timeout = 0;					//允许发送传输延时时间
}

//获取控制域标志
void GetControlMsgSign(u8 *inbuf)
{
	memcpy(control_msg_in.terminal_id,inbuf,8);
	control_msg_in.terminal_type = *(inbuf + 7);
	control_msg_in.frame_count = (((u16)(*(inbuf + 11))) << 8) + (u16)(*(inbuf + 10));

	control_msg_in.DIR = (*(inbuf + 8) & 0x80) >> 7;
	control_msg_in.PRM = (*(inbuf + 8) & 0x40) >> 6;

	control_msg_in.encryption_type = (*(inbuf + 10) & 0xE0) >> 5;
	control_msg_in.encryption_version = (*(inbuf + 11) & 0x1F);
}

//将控制域标志合并为控制域帧数据
//prm 0:应答帧 1:启动帧
void CombineControlMsg(u8 prm,u8 *c_msg)
{
	memcpy(control_msg_out.terminal_id,DeviceBaseInfo.mail_add,8);

	control_msg_out.terminal_type = DeviceBaseInfo.mail_add[7];

	if(prm == 1)			//启动帧 来自启动站
	{
		control_msg_out.frame_count += 1;		//流水号加一
	}
	else if(prm == 0)		//应答帧 来自从动站
	{
		control_msg_out.frame_count = control_msg_in.frame_count;	//流水号和启动帧相同
	}

	control_msg_out.DIR = 1;	//dir=1表示上行报文
	control_msg_out.PRM = prm;

	if(EventRecordList.important_event_flag == 1)
	{
		control_msg_out.ACD = 1;
	}

	control_msg_out.encryption_type = ENCRYPTION_TYPE;		//加密类型
	control_msg_out.encryption_version = ENCRYPTION_VERSION;	//加密版本

	memcpy(c_msg + 0,control_msg_out.terminal_id,8);	//填充通讯地址

	*(c_msg + 8) = 0;									//填充帧属性

	if(control_msg_out.DIR == 1)
	{
		*(c_msg + 8) |= (1 << 7);
	}

	if(control_msg_out.PRM == 1)
	{
		*(c_msg + 8) |= (1 << 6);
	}

	if(control_msg_out.ACD == 1)
	{
		*(c_msg + 8) |= (1 << 5);
	}

	*(c_msg + 9) = (control_msg_out.encryption_type << 5) + control_msg_out.encryption_version & 0x1F;

	*(c_msg + 10) = (u8)(control_msg_out.frame_count & 0x00FF);		//填充帧流水号
	*(c_msg + 11) = (u8)(control_msg_out.frame_count >> 8);
}

//获取用户数据标志
void GetUserDataSign(u8 *inbuf)
{
	user_data_sign_in.AFN = *(inbuf + 0);
	user_data_sign_in.SEQ = *(inbuf + 1);

	user_data_sign_in.TpV = (user_data_sign_in.SEQ & 0x80) >> 7;
	user_data_sign_in.FIR = (user_data_sign_in.SEQ & 0x40) >> 6;
	user_data_sign_in.FIN = (user_data_sign_in.SEQ & 0x20) >> 5;
	user_data_sign_in.CON = (user_data_sign_in.SEQ & 0x10) >> 4;

	if(control_msg_out.PRM == 1)	//判断是否为启动帧
	{
		user_data_sign_in.PSEQ = (user_data_sign_in.SEQ & 0x07);
	}
	else							//响应帧
	{
		user_data_sign_in.RSEQ = (user_data_sign_in.SEQ & 0x07);
	}
}

//将应用层功能码、帧序列域SEQ、数据单元表示以及数据单元合并为用户数据帧
u16 CombineUserData(u8 *u_data)
{
	u8 i = 0;
	u16 pos = 0;

	user_data_sign_out.TpV = 1;
	user_data_sign_out.FIR = 1;
	user_data_sign_out.FIN = 1;

	if(control_msg_out.PRM == 1)	//判断是否为启动帧 启动帧
	{
		if(user_data_sign_out.AFN == 0x02)
		{
			user_data_sign_out.CON = 1;
		}
		else						//响应帧
		{
			user_data_sign_out.CON = 0;
		}

		user_data_out.AUX.Tp.PFC += 1;		//时间标签中的 启动帧帧序号计数器

		user_data_sign_out.PSEQ = user_data_out.AUX.Tp.PFC & 0x0F;	//启动帧序号
	}
	else
	{
		user_data_sign_out.CON = 0;

		user_data_sign_out.RSEQ += 1;		//响应帧序号
	}

	user_data_out.AUX.EC1 = EventRecordList.ec1;	//时间标签中的 重要事件计数器
	user_data_out.AUX.EC2 = EventRecordList.ec2;	//时间标签中的 一般事件计数器

	memcpy(user_data_out.AUX.Tp.start_frame_time,CalendarClock,4);

	user_data_out.AUX.Tp.allow_timeout = 0;

	*(u_data + 0) = user_data_sign_out.AFN;			//填充应用层功能码AFN

	if(user_data_sign_out.TpV == 1)
	{
		*(u_data + 1) |= (1 << 7);
	}

	if(user_data_sign_out.FIR == 1)
	{
		*(u_data + 1) |= (1 << 6);
	}

	if(user_data_sign_out.FIN == 1)
	{
		*(u_data + 1) |= (1 << 5);
	}

	if(user_data_sign_out.CON == 1)
	{
		*(u_data + 1) |= (1 << 4);
	}

	if(control_msg_out.PRM == 1)	//判断是否为启动帧 启动帧
	{
		*(u_data + 1) += (user_data_sign_out.PSEQ & 0x0F);	//填充帧序列域SEQ
	}
	else
	{
		*(u_data + 1) += (user_data_sign_out.RSEQ & 0x0F);
	}

	pos += 2;

	for(i = 0; i < user_data_out.num; i ++)	//填充数据单元标识符和数据单元
	{
		SplitDataUnitSign(user_data_out.data_unit[i].pn_fn.pn,
						  user_data_out.data_unit[i].pn_fn.fn,
						  u_data + pos + 0,
						  u_data + pos + 1,
						  u_data + pos + 2,
						  u_data + pos + 3);	//填充数据单元标识符

		pos += 4;

		memcpy(u_data + pos,user_data_out.data_unit[i].msg,user_data_out.data_unit[i].len);	//填充数据单元

		pos += user_data_out.data_unit[i].len;
	}

	*(u_data + pos + 0) = user_data_out.AUX.EC1;	//填充重要事件计数器
	*(u_data + pos + 1) = user_data_out.AUX.EC2;	//填充一般事件计数器

	pos += 2;

	if(user_data_sign_out.TpV == 1)		//判断是否需要发送时间标签
	{
		*(u_data + pos + 0) = user_data_out.AUX.Tp.PFC;		//填充启动帧帧序号计数器

		memcpy(u_data + pos + 1,user_data_out.AUX.Tp.start_frame_time,4);	//填充启动帧发送时标

		*(u_data + pos + 5) = user_data_out.AUX.Tp.allow_timeout;	//填充允许发送传输延时时间
	}

	pos += 6;

	return pos;
}

//将控制域和用户数据域合并成完整的数据帧
//prm 0:应答帧 1:启动帧
u16 CombineCompleteFrame(u8 prm, u8 *outbuf)
{
	u16 ret = 0;
	u16 crc_cal = 0;
	u8 i = 0;

	CombineControlMsg(prm,outbuf + 6);			//控制域从第6字节开始

	ret = CombineUserData(outbuf + 6 + 12);		//用户数据域从第18字节开始

	ret += 12;									//控制域+用户数据域的长度

	crc_cal = CRC16(outbuf + 6,ret,1);

	*(outbuf + 0) = 0x68;						//填充报文头 第0字节开始
	*(outbuf + 1) = (u8)(ret & 0x00FF);
	*(outbuf + 2) = (u8)(ret >> 8);
	*(outbuf + 3) = (u8)(ret & 0x00FF);
	*(outbuf + 4) = (u8)(ret >> 8);
	*(outbuf + 5) = 0x68;

	ret += 6;									//报文头+控制域+用户数据域的长度

	*(outbuf + ret + 0) = (u8)(crc_cal >> 8);	//填充CRC校验码
	*(outbuf + ret + 1) = (u8)(crc_cal & 0x00FF);

	*(outbuf + ret + 2) = 0x16;					//填充报文结束符

	ret += 3;									//报文头+控制域+用户数据域+CRC+结束符的长度
	
	for(i = 0; i < user_data_out.num; i ++)		//释放掉输出数据单元的内存
	{
		if(user_data_out.data_unit[i].len != 0)
		{
			myfree(user_data_out.data_unit[i].msg);
		}
	}

	return ret;
}

//制作登录/退出登录/心跳报文
//fn 1:登录 2:退出登录 3:心跳
u16 MakeLogin_out_heartbeatFrame(u8 afn,u8 fn,u8 *outbuf)
{
	u16 ret = 0;
	
	user_data_sign_out.AFN = afn;
	user_data_out.num = 1;
	user_data_out.data_unit[0].pn_fn.pn = 0;
	user_data_out.data_unit[0].pn_fn.fn = fn;
	
	switch(fn)
	{
		case 1:		//登录
			user_data_out.data_unit[0].len = 28;
			user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);
		
			memcpy(user_data_out.data_unit[0].msg + 0, DeviceInfo.iccid,20);
			memcpy(user_data_out.data_unit[0].msg + 20,DeviceInfo.protocol_ver,8);
		break;
		
		case 2:		//退出登录
			user_data_out.data_unit[0].len = 0;
			user_data_out.data_unit[0].msg = NULL;
		break;
		
		case 3:		//心跳
			user_data_out.data_unit[0].len = 0;
			user_data_out.data_unit[0].msg = NULL;
		break;
		
		case 13:
			user_data_out.data_unit[0].len = 4;
			user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);
		
			*(user_data_out.data_unit[0].msg + 0) = (u8)(FrameWareState.total_bags & 0x00FF);
			*(user_data_out.data_unit[0].msg + 1) = (u8)(FrameWareState.total_bags >> 8);
			*(user_data_out.data_unit[0].msg + 2) = (u8)(FrameWareState.current_bag_cnt & 0x00FF);
			*(user_data_out.data_unit[0].msg + 3) = (u8)(FrameWareState.current_bag_cnt >> 8);
		default:
		break;
	}
	
	ret = CombineCompleteFrame(1,outbuf);
	
	return ret;
}

//合并数据单元标识符
void CombineDataUnitSign(u8 da1,u8 da2,u8 dt1,u8 dt2,u16 *pn,u16 *fn)
{
	u8 i = 0;

	if(da2 >= 1 && da2 == 0)
	{
		da2 --;
	}

	for(i = 0; i < 8; i ++)
	{
		if((da1 & (1 << i)) != (u32)0x00)
		{
			break;
		}
	}

	if(i <= 7)
	{
		i ++;
	}
	else if(i == 8)
	{
		i = 0;
	}
	
	*pn = da2 * 8 + i;

	if(dt2 >= 1 && dt1 == 0)
	{
		dt2 --;
	}

	for(i = 0; i < 8; i ++)
	{
		if((dt1 & (1 << i)) != (u32)0x00)
		{
			break;
		}
	}

	if(i <= 7)
	{
		i ++;
	}
	else if(i == 8)
	{
		i = 0;
	}
	
	*fn = dt2 * 8 + i;
}

//拆分数据单元标识符
void SplitDataUnitSign(u16 pn,u16 fn,u8 *da1,u8 *da2,u8 *dt1,u8 *dt2)
{
	if(pn == 0)
	{
		*da1 = 0;
		*da2 = 0;
	}
	else
	{
		if(pn / 8 == 0)
		{
			*da2 = 0;
			*da1 = (1 << pn - 1);
		}
		else
		{
			if(pn % 8 == 0)
			{
				*da2 = pn / 8 - 1;
				*da1 = (1 << 7);
			}
			else
			{
				*da2 = pn / 8;
				*da1 = (1 << (pn % 8) - 1);
			}
		}
	}

	if(fn == 0)
	{
		*dt1 = 0;
		*dt2 = 0;
	}
	else
	{
		if(fn / 8 == 0)
		{
			*dt2 = 0;
			*dt1 = (1 << fn - 1);
		}
		else
		{
			if(fn % 8 == 0)
			{
				*dt2 = fn / 8 - 1;
				*dt1 = (1 << 7);
			}
			else
			{
				*dt2 = fn / 8;
				*dt1 = (1 << (fn % 8) - 1);
			}
		}
	}
}

//获取报文中的数据单元
void GetUserData(u8 *inbuf,u16 len)
{
	u8 *msg = NULL;
	s16 data_len = (s16)len;
	s16 TpV_len = 0;
	u8 DA1 = 0;
	u8 DA2 = 0;
	u8 DT1 = 0;
	u8 DT2 = 0;

	msg = inbuf + 2;
	data_len -= 2;

	if(user_data_sign_in.TpV == 0)		//无时间标签
	{
		TpV_len = 0;					//时间标签长度
	}
	else if(user_data_sign_in.TpV == 1)	//有时间标签
	{
		TpV_len = 6;					//时间标签长度
	}

	GET_PN_FN:
	if((s16)data_len == TpV_len)
	{
		if(TpV_len == 6)				//获取时间标签
		{
			user_data_in.AUX.Tp.PFC = *(msg + 0);
			memcpy(user_data_in.AUX.Tp.start_frame_time,msg + 1,4);
			user_data_in.AUX.Tp.allow_timeout = *(msg + 5);
		}

		return;
	}

	DA1 = *(msg + 0);
	DA2 = *(msg + 1);
	DT1 = *(msg + 2);
	DT2 = *(msg + 3);

	if(user_data_sign_in.AFN <= 0x11 &&
	   user_data_sign_in.AFN != 0x03 &&
	   user_data_sign_in.AFN != 0x06 &&
	   user_data_sign_in.AFN != 0x07 &&
	   user_data_sign_in.AFN != 0x08 &&
	   user_data_sign_in.AFN != 0x0B &&
	   user_data_sign_in.AFN != 0x0D &&
	   user_data_sign_in.AFN != 0x0F)
	{
		CombineDataUnitSign(DA1,
		                    DA2,
		                    DT1,
		                    DT2,
		                    &user_data_in.data_unit[user_data_in.num].pn_fn.pn,
		                    &user_data_in.data_unit[user_data_in.num].pn_fn.fn);

		msg += 4;
		data_len -= 4;
		user_data_in.data_unit[user_data_in.num].msg = msg;

		switch(user_data_sign_in.AFN)
		{
			case 0x00:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 2;
					break;

					case 2:
						user_data_in.data_unit[user_data_in.num].len = 3;
					break;

					case 3:
//							user_data_in.data_unit[user_data_in.num].len = 3;
					break;

					case 4:
						user_data_in.data_unit[user_data_in.num].len = 17;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x01:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 1;
					break;

					case 2:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x02:

			break;

			case 0x04:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 8;
					break;

					case 2:
						user_data_in.data_unit[user_data_in.num].len = 156;
					break;

					case 3:
						user_data_in.data_unit[user_data_in.num].len = 24;
					break;

					case 4:
						user_data_in.data_unit[user_data_in.num].len = 24;
					break;

					case 5:
						user_data_in.data_unit[user_data_in.num].len = 5;
					break;

					case 6:
						user_data_in.data_unit[user_data_in.num].len = 16;
					break;

					case 8:
						user_data_in.data_unit[user_data_in.num].len = 257;
					break;

					case 9:
						user_data_in.data_unit[user_data_in.num].len = 1 + 1 + ((((u16)(*(msg + 2))) << 8) + (u16)(*(msg + 3))) * 4;
					break;

					case 41:
						user_data_in.data_unit[user_data_in.num].len = 2 + ((((u16)(*(msg + 0))) << 8) + (u16)(*(msg + 1))) * 12;
					break;

					case 45:
						user_data_in.data_unit[user_data_in.num].len = 1 + (*(msg + 1)) * 4;
					break;

					case 46:
						user_data_in.data_unit[user_data_in.num].len = 1 + 32 + 1 + ((((u16)(*(msg + 33))) << 8) + (u16)(*(msg + 34))) * 10;
					break;

					case 48:
//						user_data_in.data_unit[user_data_in.num].len = 1 + 6 + 6 + 1 + (*(msg + 13)) * 4;
						user_data_in.data_unit[user_data_in.num].len = 1 + 6 + 6 + 1 + (*(msg + 13)) * 3;
					break;

					case 60:
						user_data_in.data_unit[user_data_in.num].len = 4;
					break;

					case 61:
						user_data_in.data_unit[user_data_in.num].len = 1;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x05:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 2:
						user_data_in.data_unit[user_data_in.num].len = 2;
					break;

					case 31:
						user_data_in.data_unit[user_data_in.num].len = 6;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x09:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x0A:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 2:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 3:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 4:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 5:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 6:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 8:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 9:
						user_data_in.data_unit[user_data_in.num].len = 4;
					break;

					case 41:
						user_data_in.data_unit[user_data_in.num].len = 2 + ((((u16)(*(msg + 0))) << 8) + (u16)(*(msg + 1))) * 2;
					break;

					case 45:
						user_data_in.data_unit[user_data_in.num].len = 1 + (*(msg + 1)) * 2;
					break;

					case 46:
						user_data_in.data_unit[user_data_in.num].len = 1;
					break;

					case 48:
						user_data_in.data_unit[user_data_in.num].len = 1;
					break;

					case 60:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 61:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x0C:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 2:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 69:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					case 70:
						user_data_in.data_unit[user_data_in.num].len = 0;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x0E:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 1:
						user_data_in.data_unit[user_data_in.num].len = 2;
					break;

					case 2:
						user_data_in.data_unit[user_data_in.num].len = 2;
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			case 0x10:
				switch(user_data_in.data_unit[user_data_in.num].pn_fn.fn)
				{
					case 9:
						user_data_in.data_unit[user_data_in.num].len = 212;
					break;

					case 12:
						user_data_in.data_unit[user_data_in.num].len = 42;
					break;

					case 13:
						user_data_in.data_unit[user_data_in.num].len = 2 + 2 + 2 + ((((u16)(*(msg + 5))) << 8) + (u16)(*(msg + 4)));
					break;

					default:
						user_data_in.data_unit[user_data_in.num].len = data_len;
					break;
				}
			break;

			default:
				user_data_in.data_unit[user_data_in.num].len = data_len;
			break;
		}

		msg += user_data_in.data_unit[user_data_in.num].len;
		data_len -= user_data_in.data_unit[user_data_in.num].len;
		user_data_in.num ++;

		goto GET_PN_FN;
	}
}

//用户数据单元处理函数
u16 UserDataUnitHandle(void)
{
	u16 ret = 0;
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;
	u8 err_code = 0;
	u8 unit_num = user_data_in.num;
	u8 *msg = NULL;
	u8 *msg1 = NULL;
	u8 temp0 = 0;
//	u8 temp1 = 0;
	u16 temp2 = 0;
	u16 temp3 = 0;
	u8 temp4[10];
	s16 temp5 = 0;
	u16 temp6 = 0;
	u16 crc_read = 0;
	u16 crc_cal = 0;
	u8 temp_buf[64];
	u16 temp7 = 0;

	switch(user_data_sign_in.AFN)
	{
		case 0x00:
			for(i = 0; i < unit_num; i ++)
			{
				msg = user_data_in.data_unit[i].msg;
				
				if(LogInOutState == 0x00)
				{
					LogInOutState = 0x01;				//登录腾明平台成功
				}
				
				switch(user_data_in.data_unit[i].pn_fn.fn)
				{
					case 1:

					break;

					case 2:

					break;

					case 3:

					break;

					case 4:

					break;

					default:
					break;
				}
			}
		break;

		case 0x01:
			user_data_sign_out.AFN = 0x00;
			user_data_out.num = 1;
			user_data_out.data_unit[0].pn_fn.pn = 0;

			msg = user_data_in.data_unit[0].msg;

			switch(user_data_in.data_unit[0].pn_fn.fn)
			{
				case 1:
					if(*(msg + 0) == 1 || *(msg + 0) == 2 || *(msg + 0) == 3)
					{
						DeviceReset = *(msg + 0);

						user_data_out.data_unit[0].pn_fn.fn = 1;
					}
					else
					{
						user_data_out.data_unit[0].pn_fn.fn = 2;

						err_code = 2;
					}
				break;

				case 2:
					ReConnectToServer = 1;

					user_data_out.data_unit[0].pn_fn.fn = 1;
				break;

				default:
				break;
			}
		break;

		case 0x04:
			user_data_sign_out.AFN = 0x00;
			user_data_out.num = 1;
			user_data_out.data_unit[0].pn_fn.pn = 0;

			for(i = 0; i < unit_num; i ++)
			{
				msg = user_data_in.data_unit[i].msg;

				switch(user_data_in.data_unit[i].pn_fn.fn)
				{
					case 1:		//上行通讯口通讯参数
						UpCommPortPara.wait_slave_rsp_timeout 		= ((((u16)(*(msg + 1))) << 8) & 0x0F00) + (u16)(*(msg + 0));
						UpCommPortPara.master_retry_times 			= (*(msg + 1) >> 4) & 0x03;
						UpCommPortPara.need_master_confirm 			= *(msg + 2);
						UpCommPortPara.heart_beat_cycle 			= *(msg + 3);
						UpCommPortPara.random_peak_staggering_time 	= ((((u16)(*(msg + 5))) << 8) + (u16)(*(msg + 4)));
						UpCommPortPara.specify_peak_staggering_time = ((((u16)(*(msg + 7))) << 8) + (u16)(*(msg + 6)));

						WriteDataFromMemoryToEeprom(msg + 0,UP_COMM_PARA_ADD, UP_COMM_PARA_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 2:		//主站服务器配置信息
						GetIpAdderssFromMemory(&ServerInfo.ip1,msg + 0);
						GetPortFromMemory(&ServerInfo.port1,msg + 4);

						memset(temp_buf,0,64);
						memcpy(temp_buf,msg + 6,16);
						GetMemoryForSpecifyPointer(&ServerInfo.apn,strlen((char *)temp_buf), temp_buf);

						memset(temp_buf,0,64);
						memcpy(temp_buf,msg + 22,32);
						GetMemoryForSpecifyPointer(&ServerInfo.user_name,strlen((char *)temp_buf), temp_buf);

						memset(temp_buf,0,64);
						memcpy(temp_buf,msg + 54,32);
						GetMemoryForSpecifyPointer(&ServerInfo.pwd,strlen((char *)temp_buf), temp_buf);

						memset(temp_buf,0,64);
						memcpy(temp_buf,msg + 86,32);
						GetMemoryForSpecifyPointer(&ServerInfo.domain1,strlen((char *)temp_buf), temp_buf);

						GetIpAdderssFromMemory(&ServerInfo.ip2,msg + 118);
						GetPortFromMemory(&ServerInfo.port2,msg + 122);

						memset(temp_buf,0,64);
						memcpy(temp_buf,msg + 124,32);
						GetMemoryForSpecifyPointer(&ServerInfo.domain2,strlen((char *)temp_buf), temp_buf);

						WriteDataFromMemoryToEeprom(msg + 0,
						                            SERVER_INFO_ADD,
													SERVER_INFO_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 3:		//终端事件记录配置设置
						memcpy(EventRecordConf.effective,msg + 0,8);
						memcpy(EventRecordConf.important,msg + 8,8);
						memcpy(EventRecordConf.auto_report,msg + 16,8);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                ER_OTHER_CONF_ADD,
					                                ER_OTHER_CONF_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 4:		//设备基本信息
						memcpy(DeviceBaseInfo.id,msg + 0,6);
						memcpy(DeviceBaseInfo.mail_add,msg + 6,8);
						memcpy(DeviceBaseInfo.longitude,msg + 14,5);
						memcpy(DeviceBaseInfo.latitude,msg + 19,5);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                DEV_BASIC_INFO_ADD,
					                                DEV_BASIC_INFO_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 5:		//终端事件检测配置时间参数
						EventDetectConf.comm_falut_detect_interval 		= *(msg + 0);
						EventDetectConf.router_fault_detect_interval	= *(msg + 1);
						EventDetectConf.turn_on_collect_delay 			= *(msg + 2);
						EventDetectConf.turn_off_collect_delay 			= *(msg + 3);
						EventDetectConf.current_detect_delay 			= *(msg + 4);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                ER_TIME_CONF_ADD,
					                                ER_TIME_CONF_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 6:		//终端事件检测配置阈值参数
						EventDetectConf.over_current_ratio 			= *(msg + 0);
						EventDetectConf.over_current_recovery_ratio = *(msg + 1);
						EventDetectConf.low_current_ratio 			= *(msg + 2);
						EventDetectConf.low_current_recovery_ratio 	= *(msg + 3);

						memcpy(EventDetectConf.capacitor_fault_pf_ratio,msg + 4,2);
						memcpy(EventDetectConf.capacitor_fault_recovery_pf_ratio,msg + 6,2);

						EventDetectConf.lamps_over_current_ratio 			= *(msg + 8);
						EventDetectConf.lamps_over_current_recovery_ratio 	= *(msg + 9);
						EventDetectConf.fuse_over_current_ratio 			= *(msg + 10);
						EventDetectConf.fuse_over_current_recovery_ratio 	= *(msg + 11);
						EventDetectConf.leakage_over_current_ratio 			= *(msg + 12);
						EventDetectConf.leakage_over_current_recovery_ratio = *(msg + 13);
						EventDetectConf.leakage_over_voltage_ratio 			= *(msg + 14);
						EventDetectConf.leakage_over_voltage_recovery_ratio = *(msg + 15);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                ER_THRE_CONF_ADD,
					                                ER_THRE_CONF_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 8:		//RSA加密参数
						RSA_PublicKey.enable = *(msg + 0);
						memcpy(RSA_PublicKey.n,msg + 1,128);
						memcpy(RSA_PublicKey.el,msg + 129,128);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                RSA_ENCRY_PARA_ADD,
					                                RSA_ENCRY_PARA_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 9:		//控制器开关灯时间数据
						LampsSwitchProject.start_month = *(msg + 0);
						LampsSwitchProject.start_date  = *(msg + 1);

						LampsSwitchProject.total_days = ((((u16)(*(msg + 3))) << 8) + (u16)(*(msg + 2)));

						WriteDataFromMemoryToEeprom(msg + 0,
					                                SWITCH_DATE_DAYS_ADD,
					                                SWITCH_DATE_DAYS_LEN - 2);	//将数据写入EEPROM

						msg += 4;

						for(k = 0; k < LampsSwitchProject.total_days; k ++)
						{
							LampsSwitchProject.switch_time[k].on_time[0] = *(msg + k * 4 + 0);
							LampsSwitchProject.switch_time[k].on_time[1] = *(msg + k * 4 + 1);
							LampsSwitchProject.switch_time[k].off_time[0] = *(msg + k * 4 + 2);
							LampsSwitchProject.switch_time[k].off_time[1] = *(msg + k * 4 + 3);

							WriteDataFromMemoryToEeprom(msg + k * (SWITCH_TIME_LEN - 2) + 0,
							                            SWITCH_TIME_ADD + k * SWITCH_TIME_LEN,
							                            SWITCH_TIME_LEN - 2);	//将数据写入EEPROM
						}

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 41:		//灯具基本参数
						LampsParameters.num = ((((u16)(*(msg + 1))) << 8) + (u16)(*(msg + 0)));

						WriteDataFromMemoryToEeprom(msg + 0,
					                                LAMPS_NUM_PARA_ADD,
					                                LAMPS_NUM_PARA_LEN - 2);	//将数据写入EEPROM

						msg += 2;

						for(k = 0; k < LampsParameters.num; k ++)
						{
							LampsParameters.parameters[k].lamps_id  	= ((((u16)(*(msg + k * 12 + 1))) << 8) + (u16)(*(msg + k * 12 + 0)));
							LampsParameters.parameters[k].type  		= *(msg + k * 12 + 2);
							LampsParameters.parameters[k].power  		= ((((u16)(*(msg + k * 12 + 4))) << 8) + (u16)(*(msg + k * 12 + 3)));
							LampsParameters.parameters[k].pf  			= ((((u16)(*(msg + k * 12 + 6))) << 8) + (u16)(*(msg + k * 12 + 5)));
							LampsParameters.parameters[k].enable  		= *(msg + k * 12 + 7);
							LampsParameters.parameters[k].line_id  		= *(msg + k * 12 + 8);
							LampsParameters.parameters[k].a_b_c  		= *(msg + k * 12 + 9);
							LampsParameters.parameters[k].pole_number	= ((((u16)(*(msg + k * 12 + 11))) << 8) + (u16)(*(msg + k * 12 + 10)));

							WriteDataFromMemoryToEeprom(msg + k * (LAMPS_PARA_LEN - 2) + 0,
							                            LAMPS_PARA_ADD + k * LAMPS_PARA_LEN,
							                            LAMPS_PARA_LEN - 2);	//将数据写入EEPROM
						}
						
						NeedUpdateStrategyList = 1;		//需要更新策略列表

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 45:		//单灯运行模式数据
						LampsRunMode.num = *(msg + 0);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                LAMPS_NUM_MODE_ADD,
					                                LAMPS_NUM_MODE_LEN - 2);	//将数据写入EEPROM

						msg += 1;

						for(k = 0; k < LampsRunMode.num; k ++)
						{
							LampsRunMode.run_mode[k].lamps_id  				= ((((u16)(*(msg + k * 4 + 1))) << 8) + (u16)(*(msg + k * 4 + 0)));
							LampsRunMode.run_mode[k].initial_brightness  	= *(msg + k * 4 + 2);
							LampsRunMode.run_mode[k].energy_saving_mode_id  = *(msg + k * 4 + 3);

							WriteDataFromMemoryToEeprom(msg + k * (LAMPS_MODE_LEN - 2) + 0,
							                            LAMPS_MODE_ADD + k * LAMPS_MODE_LEN,
							                            LAMPS_MODE_LEN - 2);	//将数据写入EEPROM
						}

						NeedUpdateStrategyList = 1;		//需要更新策略列表
						
						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 46:		//节能运行模式
						for(k = 0; k < MAX_ENERGY_SAVING_MODE_NUM; k ++)
						{
							if(EnergySavingMode[k].mode_id == 0 || EnergySavingMode[k].mode_id == *(msg + 0))	//空模式或者重复模式
							{
								EnergySavingMode[k].mode_id = *(msg + 0);
								memcpy(EnergySavingMode[k].mode_name,msg + 1,32);
								EnergySavingMode[k].control_times = *(msg + 33);

								WriteDataFromMemoryToEeprom(msg + 0,
								                            E_SAVE_MODE_LABLE_ADD + k * E_SAVE_MODE_LEN,
								                            E_SAVE_MODE_LABLE_LEN - 2);	//将数据写入EEPROM

								msg += 34;

								if(EnergySavingMode[k].control_times <= MAX_OPERATION_TIMES)
								{
									for(j = 0; j < EnergySavingMode[k].control_times; j ++)
									{
										EnergySavingMode[k].operation[j].mode 			= *(msg + j * 10 + 0);
										EnergySavingMode[k].operation[j].control_type 	= *(msg + j * 10 + 1);
										EnergySavingMode[k].operation[j].brightness 	= *(msg + j * 10 + 2);
										EnergySavingMode[k].operation[j].relative_time 	= *(msg + j * 10 + 3);
										memcpy(EnergySavingMode[k].operation[j].absolute_time,msg + j * 10 + 4,6);

										WriteDataFromMemoryToEeprom(msg + j * 10 + 0,
															        E_SAVE_MODE_CONTENT_ADD + k * E_SAVE_MODE_LEN + j * E_SAVE_MODE_CONTENT_LEN,
								                                    E_SAVE_MODE_CONTENT_LEN - 2);	//将数据写入EEPROM
									}
								}
								break;
							}
						}

						NeedUpdateStrategyList = 1;		//需要更新策略列表
						
						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 48:		//单灯预约控制数据
						AppointmentControl.appointment_id = *(msg + 0);
						memcpy(AppointmentControl.start_date,msg + 1,6);
						memcpy(AppointmentControl.end_date,msg + 7,6);
						AppointmentControl.lamps_num = *(msg + 13);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                APPOIN_LABLE_ADD,
					                                APPOIN_LABLE_LEN - 2);	//将数据写入EEPROM

						msg += 14;

						for(k = 0; k < AppointmentControl.lamps_num; k ++)
						{
//							AppointmentControl.run_mode[k].lamps_id 				= ((((u16)(*(msg + k * 4 + 1))) << 8) + (u16)(*(msg + k * 4 + 0)));
//							AppointmentControl.run_mode[k].initial_brightness 		= *(msg + k * 4 + 2);
//							AppointmentControl.run_mode[k].energy_saving_mode_id 	= *(msg + k * 4 + 3);
							
							AppointmentControl.run_mode[k].lamps_id 				= ((((u16)(*(msg + k * 4 + 1))) << 8) + (u16)(*(msg + k * 4 + 0)));
							AppointmentControl.run_mode[k].initial_brightness 		= 100;
							AppointmentControl.run_mode[k].energy_saving_mode_id 	= *(msg + k * 4 + 2);

							WriteDataFromMemoryToEeprom(msg + k * (APPOIN_CONTENT_LEN - 2) + 0,
							                            APPOIN_CONTENT_ADD + k * APPOIN_CONTENT_LEN,
							                            APPOIN_CONTENT_LEN - 2);	//将数据写入EEPROM
						}

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 60:		//开关光照阈值
						IlluminanceThreshold.on 	= ((((u16)(*(msg + 1))) << 8) + (u16)(*(msg + 0)));
						IlluminanceThreshold.off 	= ((((u16)(*(msg + 3))) << 8) + (u16)(*(msg + 2)));

						WriteDataFromMemoryToEeprom(msg + 0,
					                                ILLUM_THRE_ADD,
					                                ILLUM_THRE_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					case 61:		//开关模式
						SwitchMode = *(msg + 0);

						WriteDataFromMemoryToEeprom(msg + 0,
					                                SWITCH_MODE_ADD,
					                                SWITCH_MODE_LEN - 2);	//将数据写入EEPROM

						user_data_out.data_unit[i].pn_fn.fn = 1;
					break;

					default:
					break;
				}
			}
		break;

		case 0x05:
			user_data_sign_out.AFN = 0x00;
			user_data_out.num = 1;
			user_data_out.data_unit[0].pn_fn.pn = 0;

			for(i = 0; i < unit_num; i ++)
			{
				msg = user_data_in.data_unit[i].msg;

				switch(user_data_in.data_unit[i].pn_fn.fn)
				{
					case 2:
						RemoteControl.lamps_id 		= user_data_in.data_unit[i].pn_fn.pn;
						RemoteControl.control_type	= *(msg + 0);
						RemoteControl.brightness	= *(msg + 1);
						RemoteControl.lock = 1;
					
						CurrentControl.lamps_id 	= RemoteControl.lamps_id;
						CurrentControl.control_type = RemoteControl.control_type;
						CurrentControl.brightness 	= RemoteControl.brightness;
						CurrentControl.lock 		= RemoteControl.lock;
					break;

					case 31:
						SyncTime.enable = 1;
						memcpy(SyncTime.date_time,msg,6);
						
						temp_buf[0] = (SyncTime.date_time[5] >> 4) * 10 + (SyncTime.date_time[5] & 0x0F);				//年
						temp_buf[1] = ((SyncTime.date_time[4] >> 4) & 0x01) * 10 + (SyncTime.date_time[4] & 0x0F);		//月
						temp_buf[2] = (SyncTime.date_time[3] >> 4) * 10 + (SyncTime.date_time[3] & 0x0F);				//日
						temp_buf[3] = (SyncTime.date_time[2] >> 4) * 10 + (SyncTime.date_time[2] & 0x0F);				//时
						temp_buf[4] = (SyncTime.date_time[1] >> 4) * 10 + (SyncTime.date_time[1] & 0x0F);				//分
						temp_buf[5] = (SyncTime.date_time[0] >> 4) * 10 + (SyncTime.date_time[0] & 0x0F);				//秒
					
						memcpy(&temp_buf[6],CalendarClock,6);
					
						RTC_Set(temp_buf[0] + 2000,temp_buf[1],temp_buf[2],temp_buf[3],temp_buf[4],temp_buf[5]);
					
						CheckEventsEC28(&temp_buf[6],CalendarClock);
					break;
				}
			}

			user_data_out.data_unit[0].pn_fn.fn = 1;
		break;

		case 0x09:
			user_data_sign_out.AFN = 0x09;
			user_data_out.num = 1;
			user_data_out.data_unit[0].pn_fn.pn = 0;
			user_data_out.data_unit[0].pn_fn.fn = user_data_in.data_unit[0].pn_fn.fn;

			if(user_data_in.data_unit[0].pn_fn.fn == 1)
			{
				user_data_out.data_unit[0].len = 71;

				user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

				memcpy(user_data_out.data_unit[0].msg + 0,DeviceInfo.factory_code,4);
				memcpy(user_data_out.data_unit[0].msg + 4,DeviceInfo.factory_dev_id,8);
				memcpy(user_data_out.data_unit[0].msg + 12,DeviceInfo.software_ver,8);
				memcpy(user_data_out.data_unit[0].msg + 20,DeviceInfo.software_release_date,3);
				memcpy(user_data_out.data_unit[0].msg + 23,DeviceInfo.protocol_ver,8);
				memcpy(user_data_out.data_unit[0].msg + 31,DeviceInfo.hardware_ver,8);
				memcpy(user_data_out.data_unit[0].msg + 39,DeviceInfo.device_model,8);
				memcpy(user_data_out.data_unit[0].msg + 47,DeviceInfo.iccid,20);
				memcpy(user_data_out.data_unit[0].msg + 67,&DeviceInfo.lamps_num_support,1);
				memcpy(user_data_out.data_unit[0].msg + 68,DeviceInfo.hardware_release_date,3);
			}
		break;

		case 0x0A:
			user_data_sign_out.AFN = 0x0A;
			user_data_out.num = unit_num;

			for(i = 0; i < unit_num; i ++)
			{
				msg = user_data_in.data_unit[i].msg;

				user_data_out.data_unit[i].pn_fn.pn = user_data_in.data_unit[i].pn_fn.pn;
				user_data_out.data_unit[i].pn_fn.fn = user_data_in.data_unit[i].pn_fn.fn;

				switch(user_data_in.data_unit[i].pn_fn.fn)
				{
					case 1:
						user_data_out.data_unit[i].len = 8;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = (u8)(UpCommPortPara.wait_slave_rsp_timeout & 0x00FF);
						*(user_data_out.data_unit[i].msg + 1) = (u8)((UpCommPortPara.wait_slave_rsp_timeout >> 8) & 0x000F) +
																(u8)((UpCommPortPara.master_retry_times << 4) & 0x30);
						*(user_data_out.data_unit[i].msg + 2) = UpCommPortPara.need_master_confirm;
						*(user_data_out.data_unit[i].msg + 3) = UpCommPortPara.heart_beat_cycle;
						*(user_data_out.data_unit[i].msg + 4) = (u8)(UpCommPortPara.random_peak_staggering_time & 0x00FF);
						*(user_data_out.data_unit[i].msg + 5) = (u8)(UpCommPortPara.random_peak_staggering_time >> 8);
						*(user_data_out.data_unit[i].msg + 6) = (u8)(UpCommPortPara.specify_peak_staggering_time & 0x00FF);
						*(user_data_out.data_unit[i].msg + 7) = (u8)(UpCommPortPara.specify_peak_staggering_time >> 8);
					break;

					case 2:
						user_data_out.data_unit[i].len = 156;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memset(temp4,0,10);
						msg = ServerInfo.ip1;
						for(j = 0; j < 4; j ++)
						{
							k = 0;
							memset(temp_buf,0,64);
							while(*msg != '.' && *msg != '\0')
								temp_buf[k ++] = *(msg ++);
							temp_buf[k] = '\0';

							msg = msg + 1;
							temp4[j] = nbiot_atoi((char *)temp_buf,strlen((char *)temp_buf));
						}
						
						memcpy(user_data_out.data_unit[i].msg + 0,temp4,4);

						msg = ServerInfo.port1;
						temp2 = nbiot_atoi((char *)msg,strlen((char *)msg));

						*(user_data_out.data_unit[i].msg + 4) = (u8)(temp2 & 0x00FF);
						*(user_data_out.data_unit[i].msg + 5) = (u8)(temp2 >> 8);

						memset(temp_buf,0,64);
						memcpy(temp_buf,ServerInfo.apn,strlen((char *)ServerInfo.apn));
						memcpy(user_data_out.data_unit[i].msg + 6,temp_buf,16);

						memset(temp_buf,0,64);
						memcpy(temp_buf,ServerInfo.user_name,strlen((char *)ServerInfo.user_name));
						memcpy(user_data_out.data_unit[i].msg + 22,temp_buf,32);

						memset(temp_buf,0,64);
						memcpy(temp_buf,ServerInfo.pwd,strlen((char *)ServerInfo.pwd));
						memcpy(user_data_out.data_unit[i].msg + 54,temp_buf,32);

						memset(temp_buf,0,64);
						memcpy(temp_buf,ServerInfo.domain1,strlen((char *)ServerInfo.domain1));
						memcpy(user_data_out.data_unit[i].msg + 86,temp_buf,32);

						memset(temp4,0,10);
						msg = ServerInfo.ip2;
						for(j = 0; j < 4; j ++)
						{
							k = 0;
							memset(temp_buf,0,64);
							while(*msg != '.' && *msg != '\0')
								temp_buf[k ++] = *(msg ++);
							temp_buf[k] = '\0';

							msg = msg + 1;
							temp4[j] = nbiot_atoi((char *)temp_buf,strlen((char *)temp_buf));
						}
						
						memcpy(user_data_out.data_unit[i].msg + 118,temp4,4);

						msg = ServerInfo.port2;
						temp2= nbiot_atoi((char *)msg,strlen((char *)msg));

						*(user_data_out.data_unit[i].msg + 122) = (u8)(temp2 & 0x00FF);
						*(user_data_out.data_unit[i].msg + 123) = (u8)(temp2 >> 8);

						memset(temp_buf,0,64);
						memcpy(temp_buf,ServerInfo.domain2,strlen((char *)ServerInfo.domain2));
						memcpy(user_data_out.data_unit[i].msg + 124,temp_buf,32);
					break;

					case 3:
						user_data_out.data_unit[i].len = 24;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memcpy(user_data_out.data_unit[i].msg + 0,EventRecordConf.effective,8);
						memcpy(user_data_out.data_unit[i].msg + 8,EventRecordConf.important,8);
						memcpy(user_data_out.data_unit[i].msg + 16,EventRecordConf.auto_report,8);
					break;

					case 4:
						user_data_out.data_unit[i].len = 24;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memcpy(user_data_out.data_unit[i].msg + 0,DeviceBaseInfo.id,6);
						memcpy(user_data_out.data_unit[i].msg + 6,DeviceBaseInfo.mail_add,8);
						memcpy(user_data_out.data_unit[i].msg + 14,DeviceBaseInfo.longitude,5);
						memcpy(user_data_out.data_unit[i].msg + 19,DeviceBaseInfo.latitude,5);
					break;

					case 5:
						user_data_out.data_unit[i].len = 5;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = EventDetectConf.comm_falut_detect_interval;
						*(user_data_out.data_unit[i].msg + 1) = EventDetectConf.router_fault_detect_interval;
						*(user_data_out.data_unit[i].msg + 2) = EventDetectConf.turn_on_collect_delay;
						*(user_data_out.data_unit[i].msg + 3) = EventDetectConf.turn_off_collect_delay;
						*(user_data_out.data_unit[i].msg + 4) = EventDetectConf.current_detect_delay;
					break;

					case 6:
						user_data_out.data_unit[i].len = 16;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = EventDetectConf.over_current_ratio;
						*(user_data_out.data_unit[i].msg + 1) = EventDetectConf.over_current_recovery_ratio;
						*(user_data_out.data_unit[i].msg + 2) = EventDetectConf.low_current_ratio;
						*(user_data_out.data_unit[i].msg + 3) = EventDetectConf.low_current_recovery_ratio;

						memcpy(user_data_out.data_unit[i].msg + 4,EventDetectConf.capacitor_fault_pf_ratio,2);
						memcpy(user_data_out.data_unit[i].msg + 6,EventDetectConf.capacitor_fault_recovery_pf_ratio,2);

						*(user_data_out.data_unit[i].msg + 8)  = EventDetectConf.lamps_over_current_ratio;
						*(user_data_out.data_unit[i].msg + 9)  = EventDetectConf.lamps_over_current_recovery_ratio;
						*(user_data_out.data_unit[i].msg + 10) = EventDetectConf.fuse_over_current_ratio;
						*(user_data_out.data_unit[i].msg + 11) = EventDetectConf.fuse_over_current_recovery_ratio;
						*(user_data_out.data_unit[i].msg + 12) = EventDetectConf.leakage_over_current_ratio;
						*(user_data_out.data_unit[i].msg + 13) = EventDetectConf.leakage_over_current_recovery_ratio;
						*(user_data_out.data_unit[i].msg + 14) = EventDetectConf.leakage_over_voltage_ratio;
						*(user_data_out.data_unit[i].msg + 15) = EventDetectConf.leakage_over_voltage_recovery_ratio;
					break;

					case 8:
						user_data_out.data_unit[i].len = 257;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = RSA_PublicKey.enable;

						memcpy(user_data_out.data_unit[i].msg + 1,RSA_PublicKey.n,128);
						memcpy(user_data_out.data_unit[i].msg + 129,RSA_PublicKey.el,128);
					break;

					case 9:
						temp2 = ((((u16)(*(msg + 3))) << 8) + (u16)(*(msg + 2)));	//要查询的时间天数

						user_data_out.data_unit[i].len = 1 + 1 + 2 + temp2 * 4;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						temp5 = get_dates_diff(LampsSwitchProject.start_month,LampsSwitchProject.start_date,*(msg + 0),*(msg + 1));

						if(temp5 >= 0)										//要查询的日期 是第几个开关
						{
							if(temp5 + temp2 <= LampsSwitchProject.total_days)		//总天数要小于等于之前设置的总天数
							{
								memcpy(user_data_out.data_unit[i].msg + 0,msg,4);

								for(k = 0; k < temp2; k ++)
								{
									*(user_data_out.data_unit[i].msg + 4 + k * 4 + 0) = LampsSwitchProject.switch_time[temp5 + k].on_time[0];
									*(user_data_out.data_unit[i].msg + 4 + k * 4 + 1) = LampsSwitchProject.switch_time[temp5 + k].on_time[1];
									*(user_data_out.data_unit[i].msg + 4 + k * 4 + 2) = LampsSwitchProject.switch_time[temp5 + k].off_time[0];
									*(user_data_out.data_unit[i].msg + 4 + k * 4 + 3) = LampsSwitchProject.switch_time[temp5 + k].off_time[1];
								}
							}
						}

					break;

					case 41:
						temp2 = ((((u16)(*(msg + 1))) << 8) + (u16)(*(msg + 0)));	//要查询灯具的个数

						user_data_out.data_unit[i].len = 2 + temp2 * 12;			//预计需要的数据长度
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memcpy(user_data_out.data_unit[i].msg + 0,msg + 0,2);		//填写要读取的灯具个数

						for(k = 0; k < temp2; k ++)
						{
							temp3 = ((((u16)(*(msg + 2 + 2 * k + 1))) << 8) + (u16)(*(msg + 2 + 2 * k + 0)));	//灯具的序号

							for(j = 0; j < LampsParameters.num; j ++)				//轮训查找已存在的灯具序号
							{
								if(temp3 == LampsParameters.parameters[j].lamps_id)	//查找成功
								{
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 0)  = (u8)(LampsParameters.parameters[j].lamps_id & 0x00FF);
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 1)  = (u8)(LampsParameters.parameters[j].lamps_id >> 8);		//填写灯具序号
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 2)  = LampsParameters.parameters[j].type;					//类型
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 3)  = (u8)(LampsParameters.parameters[j].power & 0x00FF);
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 4)  = (u8)(LampsParameters.parameters[j].power >> 8);		//功率
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 5)  = (u8)(LampsParameters.parameters[j].pf & 0x00FF);
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 6)  = (u8)(LampsParameters.parameters[j].pf >> 8);			//功率因数
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 7)  = LampsParameters.parameters[j].enable;					//启用标志
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 8)  = LampsParameters.parameters[j].line_id;					//出线序号
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 9)  = LampsParameters.parameters[j].a_b_c;					//所处相别
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 10) = (u8)(LampsParameters.parameters[j].pole_number & 0x00FF);
									*(user_data_out.data_unit[i].msg + 2 + k * 12 + 11) = (u8)(LampsParameters.parameters[j].pole_number >> 8);	//所在杆号

									j = 0xFF;			//找到序号为temp3的灯具成功标志
								}
							}

							if(j != 0xFF)				//没有找到序号为temp3的灯具
							{
								if(k >= 1)
								{
									k --;				//为了使数据连续，不出现空当
								}
							}
						}

						user_data_out.data_unit[i].len = 2 + k * 12;				//实际需要的数据长度
					break;

					case 45:
						temp2 = (u16)(*(msg + 0));											//要查询灯具的个数

						user_data_out.data_unit[i].len = 1 + temp2 * 4;				//预计需要的数据长度
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memcpy(user_data_out.data_unit[i].msg + 0,msg + 0,1);		//填写要读取的灯具个数

						for(k = 0; k < temp2; k ++)
						{
							temp3 = (u16)((*(msg + 1 + 2 * k + 1)) << 8) + (u16)(*(msg + 1 + 2 * k + 0));	//灯具的序号

							for(j = 0; j < LampsRunMode.num; j ++)				//轮训查找已存在的灯具序号
							{
								if(temp3 == LampsRunMode.run_mode[j].lamps_id)	//查找成功
								{
									*(user_data_out.data_unit[i].msg + 1 + k * 4 + 0) = (u8)(LampsRunMode.run_mode[j].lamps_id & 0x00FF);
									*(user_data_out.data_unit[i].msg + 1 + k * 4 + 1) = (u8)(LampsRunMode.run_mode[j].lamps_id >> 8);		//填写灯具序号
									*(user_data_out.data_unit[i].msg + 1 + k * 4 + 2) = LampsRunMode.run_mode[j].initial_brightness;		//灯具初始调光值
									*(user_data_out.data_unit[i].msg + 1 + k * 4 + 3) = LampsRunMode.run_mode[j].energy_saving_mode_id;		//节能模式编号
									
									j = 0xFF;			//找到序号为temp3的灯具成功标志
								}
							}

							if(j != 0xFF)				//没有找到序号为temp3的灯具
							{
								if(k >= 1)
								{
									k --;				//为了使数据连续，不出现空当
								}
							}
						}

						user_data_out.data_unit[i].len = 1 + k * 4;				 //实际需要的数据长度
					break;

					case 46:
						for(k = 0; k < MAX_ENERGY_SAVING_MODE_NUM; k ++)		//轮训查找编号为*(msg + 0)的节能模式
						{
							if(EnergySavingMode[k].mode_id == *(msg + 0))		//查找成功
							{
								user_data_out.data_unit[i].len = 1 + 32 + 1 + EnergySavingMode[k].control_times * 10;				//预计需要的数据长度
								user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

								*(user_data_out.data_unit[i].msg + 0) = EnergySavingMode[k].mode_id;
								memcpy(user_data_out.data_unit[i].msg + 1,EnergySavingMode[k].mode_name,32);
								*(user_data_out.data_unit[i].msg + 33) = EnergySavingMode[k].control_times;

								if(EnergySavingMode[k].control_times <= MAX_OPERATION_TIMES)
								{
									for(j = 0; j < EnergySavingMode[k].control_times; j ++)
									{
										*(user_data_out.data_unit[i].msg + 34 + j * 10 + 0) = EnergySavingMode[k].operation[j].mode;
										*(user_data_out.data_unit[i].msg + 34 + j * 10 + 1) = EnergySavingMode[k].operation[j].control_type;
										*(user_data_out.data_unit[i].msg + 34 + j * 10 + 2) = EnergySavingMode[k].operation[j].brightness;
										*(user_data_out.data_unit[i].msg + 34 + j * 10 + 3) = EnergySavingMode[k].operation[j].relative_time;

										memcpy(user_data_out.data_unit[i].msg + 34 + j * 10 + 4,EnergySavingMode[k].operation[j].absolute_time,6);
									}
								}
								break;
							}
						}
					break;

					case 48:
						if(*(msg + 0) == AppointmentControl.appointment_id)
						{
//							user_data_out.data_unit[i].len = 1 + 6 + 6 + 1 + AppointmentControl.lamps_num * 4;
							user_data_out.data_unit[i].len = 1 + 6 + 6 + 1 + AppointmentControl.lamps_num * 3;
							user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

							*(user_data_out.data_unit[i].msg + 0) = AppointmentControl.appointment_id;
							memcpy(user_data_out.data_unit[i].msg + 1,AppointmentControl.start_date,6);
							memcpy(user_data_out.data_unit[i].msg + 7,AppointmentControl.end_date,6);
							*(user_data_out.data_unit[i].msg + 13) = AppointmentControl.lamps_num;

							for(k = 0; k < AppointmentControl.lamps_num; k ++)
							{
//								*(user_data_out.data_unit[i].msg + 14 + k * 4 + 0) = (u8)(AppointmentControl.run_mode[k].lamps_id >> 8);
//								*(user_data_out.data_unit[i].msg + 14 + k * 4 + 1) = (u8)(AppointmentControl.run_mode[k].lamps_id & 0x00FF);
//								*(user_data_out.data_unit[i].msg + 14 + k * 4 + 2) = AppointmentControl.run_mode[k].initial_brightness;
//								*(user_data_out.data_unit[i].msg + 14 + k * 4 + 3) = AppointmentControl.run_mode[k].energy_saving_mode_id;
								
								*(user_data_out.data_unit[i].msg + 14 + k * 3 + 0) = (u8)(AppointmentControl.run_mode[k].lamps_id >> 8);
								*(user_data_out.data_unit[i].msg + 14 + k * 3 + 1) = (u8)(AppointmentControl.run_mode[k].lamps_id & 0x00FF);
								*(user_data_out.data_unit[i].msg + 14 + k * 3 + 2) = AppointmentControl.run_mode[k].energy_saving_mode_id;
							}
						}
					break;

					case 60:
						user_data_out.data_unit[i].len = 4;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = (u8)(IlluminanceThreshold.on & 0x00FF);
						*(user_data_out.data_unit[i].msg + 1) = (u8)(IlluminanceThreshold.on >> 8);
						*(user_data_out.data_unit[i].msg + 2) = (u8)(IlluminanceThreshold.off & 0x00FF);
						*(user_data_out.data_unit[i].msg + 3) = (u8)(IlluminanceThreshold.off >> 8);
					break;

					case 61:
						user_data_out.data_unit[i].len = 1;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = SwitchMode;
					break;

					default:
					break;
				}
			}
		break;

		case 0x0C:
			user_data_sign_out.AFN = 0x0C;
			user_data_out.num = unit_num;

			for(i = 0; i < unit_num; i++)
			{
				msg = user_data_in.data_unit[i].msg;

				user_data_out.data_unit[i].pn_fn.pn = user_data_in.data_unit[i].pn_fn.pn;
				user_data_out.data_unit[i].pn_fn.fn = user_data_in.data_unit[i].pn_fn.fn;

				switch(user_data_in.data_unit[i].pn_fn.fn)
				{
					case 2:
						user_data_out.data_unit[i].len = 6;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						memcpy(user_data_out.data_unit[i].msg + 0,CalendarClock,6);
					break;

					case 69:
						user_data_out.data_unit[i].len = 24;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = DeviceElectricPara.initial_brightness;
						*(user_data_out.data_unit[i].msg + 1) = DeviceElectricPara.switch_state;
						*(user_data_out.data_unit[i].msg + 2) = DeviceElectricPara.brightness;
						*(user_data_out.data_unit[i].msg + 3) = DeviceElectricPara.run_state;

						memcpy(user_data_out.data_unit[i].msg + 4,DeviceElectricPara.volatge,2);
						memcpy(user_data_out.data_unit[i].msg + 6,DeviceElectricPara.current,3);
						memcpy(user_data_out.data_unit[i].msg + 9,DeviceElectricPara.active_power,2);
						memcpy(user_data_out.data_unit[i].msg + 11,DeviceElectricPara.pf,2);
						memcpy(user_data_out.data_unit[i].msg + 13,DeviceElectricPara.leakage_current,3);
						memcpy(user_data_out.data_unit[i].msg + 16,DeviceElectricPara.Illuminance,2);
						memcpy(user_data_out.data_unit[i].msg + 18,DeviceElectricPara.time_stamp,6);
					break;

					case 70:
						user_data_out.data_unit[i].len = 8;
						user_data_out.data_unit[i].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[i].len);

						*(user_data_out.data_unit[i].msg + 0) = NB_ModulePara.csq;
						*(user_data_out.data_unit[i].msg + 1) = NB_ModulePara.band;

						memcpy(user_data_out.data_unit[i].msg + 2,NB_ModulePara.pci,2);
						memcpy(user_data_out.data_unit[i].msg + 2,NB_ModulePara.rsrp,2);
						memcpy(user_data_out.data_unit[i].msg + 2,NB_ModulePara.snr,2);
					break;

					default:
					break;
				}
			}
		break;

		case 0x0E:
			user_data_sign_out.AFN = 0x0E;
			user_data_out.num = unit_num;

			user_data_out.data_unit[0].pn_fn.pn = user_data_in.data_unit[0].pn_fn.pn;
			user_data_out.data_unit[0].pn_fn.fn = user_data_in.data_unit[0].pn_fn.fn;

			msg = user_data_in.data_unit[0].msg;

			if(user_data_in.data_unit[0].pn_fn.fn == 1)			//重要事件
			{
				msg1 = EventRecordList.lable1;
				temp3 = E_IMPORTEAT_ADD;						//EEPROM存储偏移地址
			}
			else if(user_data_in.data_unit[0].pn_fn.fn == 2)	//一般事件
			{
				msg1 = EventRecordList.lable2;
				temp3 = E_NORMAL_ADD;
			}

			if(*(msg + 0) <= *(msg + 1))
			{
				temp2 = 0;

				for(i = *(msg + 0); i <= *(msg + 1); i ++)
				{
					switch(*(msg1 + i))
					{
						case 15:
							temp2 += 14;
						break;

						case 16:
							temp2 += 14;
						break;

						case 17:
							temp2 += 12;
						break;

						case 18:
							temp2 += 12;
						break;

						case 19:
							temp2 += 15;
						break;

						case 20:
							temp2 += 15;
						break;

						case 21:
							temp2 += 17;
						break;

						case 22:
							temp2 += 17;
						break;

						case 23:
							temp2 += 17;
						break;

						case 28:
							temp2 += 21;
						break;

						case 36:
							temp2 += 16;
						break;

						case 37:
							temp2 += 11;
						break;

						case 51:
							temp2 += 17;
						break;

						case 52:
							temp2 += 22;
						break;

						default:
						break;
					}
				}
			}
			else
			{
				temp2 = 0;

				for(i = 0; i <= *(msg + 1); i ++)
				{
					switch(*(msg1 + i))
					{
						case 15:
							temp2 += 14;
						break;

						case 16:
							temp2 += 14;
						break;

						case 17:
							temp2 += 12;
						break;

						case 18:
							temp2 += 12;
						break;

						case 19:
							temp2 += 15;
						break;

						case 20:
							temp2 += 15;
						break;

						case 21:
							temp2 += 17;
						break;

						case 22:
							temp2 += 17;
						break;

						case 23:
							temp2 += 17;
						break;

						case 28:
							temp2 += 21;
						break;

						case 36:
							temp2 += 16;
						break;

						case 37:
							temp2 += 11;
						break;

						case 51:
							temp2 += 17;
						break;

						case 52:
							temp2 += 22;
						break;

						default:
						break;
					}
				}

				for(i = *(msg + 0); i <= 255; i ++)
				{
					switch(*(msg1 + i))
					{
						case 15:
							temp2 += 14;
						break;

						case 16:
							temp2 += 14;
						break;

						case 17:
							temp2 += 12;
						break;

						case 18:
							temp2 += 12;
						break;

						case 19:
							temp2 += 15;
						break;

						case 20:
							temp2 += 15;
						break;

						case 21:
							temp2 += 17;
						break;

						case 22:
							temp2 += 17;
						break;

						case 23:
							temp2 += 17;
						break;

						case 28:
							temp2 += 21;
						break;

						case 36:
							temp2 += 16;
						break;

						case 37:
							temp2 += 11;
						break;

						case 51:
							temp2 += 17;
						break;

						case 52:
							temp2 += 22;
						break;

						default:
						break;
					}
				}
			}

			user_data_out.data_unit[0].len = temp2 + 4;
			user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

			*(user_data_out.data_unit[0].msg + 0) = EventRecordList.ec1;
			*(user_data_out.data_unit[0].msg + 1) = EventRecordList.ec2;
			memcpy(user_data_out.data_unit[0].msg + 2,msg,2);

			temp6 = 0;	//每一个事件之间的偏移地址

			if(*(msg + 0) <= *(msg + 1))
			{
				for(i = *(msg + 0); i <= *(msg + 1); i ++)
				{
					memset(temp_buf,0,32);

					switch(*(msg1 + i))
					{
						case 15:
							temp0 = 14;	//当前事件所占内存长度
						break;

						case 16:
							temp0 = 14;
						break;

						case 17:
							temp0 = 12;
						break;

						case 18:
							temp0 = 12;
						break;

						case 19:
							temp0 = 15;
						break;

						case 20:
							temp0 = 15;
						break;

						case 21:
							temp0 = 17;
						break;

						case 22:
							temp0 = 17;
						break;

						case 23:
							temp0 = 17;
						break;

						case 28:
							temp0 = 21;
						break;

						case 36:
							temp0 = 16;
						break;

						case 37:
							temp0 = 11;
						break;

						case 51:
							temp0 = 17;
						break;

						case 52:
							temp0 = 22;
						break;

						default:
						break;
					}

					ret = ReadDataFromEepromToMemory(temp_buf,temp3 + i * EVENT_LEN,EVENT_LEN);		//从EEPROM中读取时间内容

					if(ret == 1)
					{
						memcpy(user_data_out.data_unit[0].msg + 4 + temp6,temp_buf,temp0);			//读取成功 将时间内容放入数据单元
					}
					else
					{
						memset(user_data_out.data_unit[0].msg + 4 + temp6,0,temp0);					//读取失败 将数据单元清空
					}

					temp6 += temp0;
				}
			}
			else
			{
				for(i = 0; i <= *(msg + 1); i ++)
				{
					memset(temp_buf,0,32);

					switch(*(msg1 + i))
					{
						case 15:
							temp0 = 14;
						break;

						case 16:
							temp0 = 14;
						break;

						case 17:
							temp0 = 12;
						break;

						case 18:
							temp0 = 12;
						break;

						case 19:
							temp0 = 15;
						break;

						case 20:
							temp0 = 15;
						break;

						case 21:
							temp0 = 17;
						break;

						case 22:
							temp0 = 17;
						break;

						case 23:
							temp0 = 17;
						break;

						case 28:
							temp0 = 21;
						break;

						case 36:
							temp0 = 16;
						break;

						case 37:
							temp0 = 11;
						break;

						case 51:
							temp0 = 17;
						break;

						case 52:
							temp0 = 22;
						break;

						default:
						break;
					}

					ret = ReadDataFromEepromToMemory(temp_buf,temp3 + i * EVENT_LEN,EVENT_LEN);		//从EEPROM中读取时间内容

					if(ret == 1)
					{
						memcpy(user_data_out.data_unit[0].msg + 4 + temp6,temp_buf,temp0);			//读取成功 将时间内容放入数据单元
					}
					else
					{
						memset(user_data_out.data_unit[0].msg + 4 + temp6,0,temp0);					//读取失败 将数据单元清空
					}

					temp6 += temp0;
				}

				for(i = *(msg + 0); i <= 255; i ++)
				{
					memset(temp_buf,0,32);

					switch(*(msg1 + i))
					{
						case 15:
							temp0 = 14;
						break;

						case 16:
							temp0 = 14;
						break;

						case 17:
							temp0 = 12;
						break;

						case 18:
							temp0 = 12;
						break;

						case 19:
							temp0 = 15;
						break;

						case 20:
							temp0 = 15;
						break;

						case 21:
							temp0 = 17;
						break;

						case 22:
							temp0 = 17;
						break;

						case 23:
							temp0 = 17;
						break;

						case 28:
							temp0 = 21;
						break;

						case 36:
							temp0 = 16;
						break;

						case 37:
							temp0 = 11;
						break;

						case 51:
							temp0 = 17;
						break;

						case 52:
							temp0 = 22;
						break;

						default:
						break;
					}

					ret = ReadDataFromEepromToMemory(temp_buf,temp3 + i * EVENT_LEN,EVENT_LEN);		//从EEPROM中读取时间内容

					if(ret == 1)
					{
						memcpy(user_data_out.data_unit[0].msg + 4 + temp6,temp_buf,temp0);			//读取成功 将时间内容放入数据单元
					}
					else
					{
						memset(user_data_out.data_unit[0].msg + 4 + temp6,0,temp0);					//读取失败 将数据单元清空
					}

					temp6 += temp0;
				}
			}
		break;

		case 0x10:
			user_data_out.num = unit_num;

			user_data_out.data_unit[0].pn_fn.pn = user_data_in.data_unit[0].pn_fn.pn;
			user_data_out.data_unit[0].pn_fn.fn = user_data_in.data_unit[0].pn_fn.fn;

			msg = user_data_in.data_unit[0].msg;

			switch(user_data_in.data_unit[0].pn_fn.fn)
			{
				case 9:		//FTP服务器信息
					user_data_sign_out.AFN = 0x00;
				
					GetIpAdderssFromMemory(&FTP_ServerInfo.ip1,msg + 0);
					GetPortFromMemory(&FTP_ServerInfo.port1,msg + 4);
					GetIpAdderssFromMemory(&FTP_ServerInfo.ip2,msg + 6);
					GetPortFromMemory(&FTP_ServerInfo.port2,msg + 8);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 12,10);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.user_name,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 22,10);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.pwd,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 32,40);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.path1,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 72,40);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.path2,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 112,20);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.file1,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 132,20);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.file2,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 152,20);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.file3,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 172,20);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.file4,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 192,20);
					GetMemoryForSpecifyPointer(&FTP_ServerInfo.file5,strlen((char *)temp_buf), temp_buf);

					WriteDataFromMemoryToEeprom(msg + 0,
					                            E_FTP_SERVER_INFO_ADD,
					                            E_FTP_SERVER_INFO_LEN - 2);	//将数据写入EEPROM

					user_data_out.data_unit[i].pn_fn.fn = 1;
				break;

				case 12:		//FTP固件信息
					user_data_sign_out.AFN = 0x10;
					
					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 0,30);
					GetMemoryForSpecifyPointer(&FTP_FrameWareInfo.name,strlen((char *)temp_buf), temp_buf);

					memset(temp_buf,0,64);
					memcpy(temp_buf,msg + 30,8);
					GetMemoryForSpecifyPointer(&FTP_FrameWareInfo.version,strlen((char *)temp_buf), temp_buf);

					FTP_FrameWareInfo.length = (((u32)(*(msg + 41))) << 24) +
				                               (((u32)(*(msg + 40))) << 16) +
				                               (((u32)(*(msg + 39))) << 8) +
				                               (((u32)(*(msg + 38))) << 0);

					WriteDataFromMemoryToEeprom(msg + 0,
					                            E_FTP_FW_INFO_ADD,
					                            E_FTP_FW_INFO_LEN - 2);	//将数据写入EEPROM

					user_data_out.data_unit[0].len = 2;
					user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

					if(FTP_FrameWareInfo.length > FIRMWARE_SIZE)	//固件要小于240K = ((512 - 32) / 2) * 1024 = 245760 32K为BootLoader存储区域
					{
						*(user_data_out.data_unit[0].msg + 0) = 0;
						*(user_data_out.data_unit[0].msg + 1) = 2;
					}
					else if(search_str(DeviceInfo.software_ver, FTP_FrameWareInfo.version) != -1)	//与现有版本相同
					{
						*(user_data_out.data_unit[0].msg + 0) = 0;
						*(user_data_out.data_unit[0].msg + 1) = 1;
					}
					else	//可以升级
					{
						u16 page_num = 0;
//						u8 start_page = 0;
						
						FrameWareState.state 			= FIRMWARE_DOWNLOADING;
						FrameWareState.total_bags 		= FTP_FrameWareInfo.length % FIRMWARE_BAG_SIZE != 0 ? 
						                                  FTP_FrameWareInfo.length / FIRMWARE_BAG_SIZE + 1 : FTP_FrameWareInfo.length / FIRMWARE_BAG_SIZE;
						FrameWareState.current_bag_cnt 	= 1;
						FrameWareState.bag_size 		= FIRMWARE_BAG_SIZE;
						FrameWareState.last_bag_size 	= FTP_FrameWareInfo.length % FIRMWARE_BAG_SIZE != 0 ? 
						                                  FTP_FrameWareInfo.length % FIRMWARE_BAG_SIZE : FIRMWARE_BAG_SIZE;
						FrameWareState.total_size 		= FTP_FrameWareInfo.length;
						
						WriteFrameWareStateToEeprom();	//将固件升级状态写入EEPROM
						
//						start_page = (FIRMWARE_BUCKUP_FLASH_BASE_ADD - 0x08000000) / 2048;				//得到备份区的起始扇区
						page_num = (FIRMWARE_MAX_FLASH_ADD - FIRMWARE_BUCKUP_FLASH_BASE_ADD) / 2048;	//得到备份区的扇区总数
						
						FLASH_Unlock();						//解锁FLASH
						
						for(i = 0; i < page_num; i ++)
						{
							FLASH_ErasePage(i * 2048 + FIRMWARE_BUCKUP_FLASH_BASE_ADD);	//擦除当前FLASH扇区
						}
						
						FLASH_Lock();						//上锁
						
						*(user_data_out.data_unit[0].msg + 0) = 1;
						*(user_data_out.data_unit[0].msg + 1) = 0;
					}
				break;

				case 13:
					user_data_out.num = 0;
					user_data_sign_out.AFN = 0xFF;		//不需要给服务器回复ACK
				
					temp2 = ((((u16)(*(msg + 1))) << 8) + (u16)(*(msg + 0)));		//总分包数
					temp3 = ((((u16)(*(msg + 3))) << 8) + (u16)(*(msg + 2)));		//当前分包数
					temp6 = ((((u16)(*(msg + 5))) << 8) + (u16)(*(msg + 4)));		//包大小
				
					if(temp2 != FrameWareState.total_bags)	//总包数匹配错误
					{
						break;
					}

					msg += 6;
				
					crc_read = (((u16)(*(msg + temp6 - 2))) << 8) + (u16)(*(msg + temp6 - 1));
				
					crc_cal = CRC16(msg,temp6 - 2,1);
				
					if(crc_cal == crc_read)
					{
						if(temp3 == FrameWareState.current_bag_cnt)
						{
							FLASH_Unlock();						//解锁FLASH
							
							if(temp6 == FIRMWARE_BAG_SIZE)
							{
								for(i = 0; i < (FIRMWARE_BAG_SIZE - 2) / 2; i ++)
								{
									temp7 = ((u16)(*(msg + i * 2 + 1)) << 8) + (u16)(*(msg + i * 2));
									
									FLASH_ProgramHalfWord(FIRMWARE_BUCKUP_FLASH_BASE_ADD + (temp3 - 1) * (FIRMWARE_BAG_SIZE - 2) + i * 2,temp7);
								}
							}
							
							FLASH_Lock();						//上锁
							
							if(temp3 < FrameWareState.total_bags)
							{
								FrameWareState.current_bag_cnt ++;
								
								FrameWareState.state = FIRMWARE_DOWNLOADING;	//当前包下载完成
							}
							else if(temp3 == FrameWareState.total_bags)
							{
								FrameWareState.state = FIRMWARE_DOWNLOADED;		//全部下载完成
								
								WriteFrameWareStateToEeprom();
							}
						}
					}
				break;

				default:
				break;
			}
		break;

		default:
		break;
	}

	if(user_data_sign_out.AFN == 0x00)				//回复确认信息
	{
		switch(user_data_out.data_unit[0].pn_fn.fn)	//错误类型
		{
			case 1:
				user_data_out.data_unit[0].len = 2;
				user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

				*(user_data_out.data_unit[0].msg + 0) = (u8)control_msg_in.frame_count;
				*(user_data_out.data_unit[0].msg + 1) = (u8)(control_msg_in.frame_count >> 8);	//填充流水号
			break;

			case 2:
				user_data_out.data_unit[0].len = 3;
				user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

				*(user_data_out.data_unit[0].msg + 0) = (u8)control_msg_in.frame_count;
				*(user_data_out.data_unit[0].msg + 1) = (u8)(control_msg_in.frame_count >> 8);	//填充流水号
				*(user_data_out.data_unit[0].msg + 2) = err_code;
			break;

			case 3:
				user_data_out.data_unit[0].len = 7;
				user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

				*(user_data_out.data_unit[0].msg + 0) = (u8)control_msg_in.frame_count;
				*(user_data_out.data_unit[0].msg + 1) = (u8)(control_msg_in.frame_count >> 8);	//填充流水号

				SplitDataUnitSign(user_data_in.data_unit[0].pn_fn.pn,
			                      user_data_in.data_unit[0].pn_fn.fn,
			                      user_data_out.data_unit[0].msg + 2,
			                      user_data_out.data_unit[0].msg + 3,
			                      user_data_out.data_unit[0].msg + 4,
			                      user_data_out.data_unit[0].msg + 5);

				*(user_data_out.data_unit[0].msg + 6) = err_code;
			break;

			case 4:
				user_data_out.data_unit[0].len = 17;
				user_data_out.data_unit[0].msg = (u8 *)mymalloc(sizeof(u8) * user_data_out.data_unit[0].len);

				*(user_data_out.data_unit[0].msg + 0) = err_code;

				memset(user_data_out.data_unit[0].msg + 1,0,16);
			break;

			default:
			break;
		}
	}

	return ret;
}

//读取/处理网络数据
u16 NetDataFrameHandle(u8 *inbuf,u16 inbuf_len,u8 *outbuf)
{
	u16 ret = 0;
	u16 frame_len = 0;
	u16 user_data_len = 0;
	u16 crc16_recv = 0;
	u16 crc16_cal = 0;
	u8 *msg = NULL;
	u8 need_rsp = 1;
	u8 default_mail_id[8] = {0x00,0x11,0x20,0x01,0x00,0x00,0x00,0x20};	//设备缺省通信地址

	if(inbuf_len > MAX_FRAME_LEN)				//报文总长度超限
	{
		return 0;
	}

	if(*(inbuf + 0) != 0x68 ||					//起始符1
	   *(inbuf + 5)  != 0x68 ||					//起始符2
	   *(inbuf + inbuf_len - 1)  != 0x16)		//结束符
	{
		return 0;
	}

	crc16_recv = (((u16)*(inbuf + inbuf_len - 3)) << 8) + (u16)*(inbuf + inbuf_len - 2);
	crc16_cal = CRC16(&*(inbuf + 6),inbuf_len - 9,1);

	if(crc16_recv != crc16_cal)					//CRC校验失败
	{
		return 0;
	}

	if(*(inbuf + 1) != *(inbuf + 3) ||
	   *(inbuf + 2) != *(inbuf + 4))			//两个用户数据长度不同
	{
		return 0;
	}

	frame_len = (((u16)*(inbuf + 2)) << 8) + (u16)*(inbuf + 1);

	if(frame_len != inbuf_len - 9)				//用户数据长度有误
	{
		return 0;
	}

	ResetFrameStruct(0,
	                 &control_msg_in,
	                 &user_data_sign_in,
	                 &user_data_in);			//清空各个数据标识
	ResetFrameStruct(0,
	                 &control_msg_out,
	                 &user_data_sign_out,
	                 &user_data_out);

	msg = inbuf + 6;							//指到控制域起始位置

	GetControlMsgSign(msg);						//获取控制域标志

	if(control_msg_in.DIR != 0)					//判断传输方向 此处应为下行
	{
		return 0;
	}

	if(MyStrstr(DeviceBaseInfo.mail_add, control_msg_in.terminal_id, 8, 8) == 0xFFFF && 
	   MyStrstr(default_mail_id, control_msg_in.terminal_id, 8, 8) == 0xFFFF)	//判断8位设备ID(通信地址)
	{
		return 0;
	}

	msg += 12;									//指到用户数据起始位置

	GetUserDataSign(msg);

	user_data_len = frame_len - 12;				//用户数据长度 = 总长 - 控制域

	GetUserData(msg,user_data_len);				//获取报文中的数据单元标识和数据单元

	ret = UserDataUnitHandle();					//对各个数据单元进行处理

	if(user_data_sign_out.AFN == 0x00)			//上行确认帧
	{
		if(user_data_sign_in.CON == 0)			//判断是否为需要确认的帧
		{
			need_rsp = 0;
		}
	}
	else if(user_data_sign_out.AFN == 0xFF)
	{
		need_rsp = 0;
	}

	if(need_rsp == 1)							//有需要响应的报文
	{
		ret = CombineCompleteFrame(0, outbuf);	//填充整帧报文
	}

	return ret;
}



























