#include "common.h"
#include "24cxx.h"
#include "m53xx.h"


//u8 HoldReg[HOLD_REG_LEN];			//保持寄存器 临时使用

/****************************互斥量相关******************************/
SemaphoreHandle_t  xMutex_IIC1 			= NULL;	//IIC总线1的互斥量
SemaphoreHandle_t  xMutex_INVENTR 		= NULL;	//英飞特电源的互斥量
SemaphoreHandle_t  xMutex_AT_COMMAND 	= NULL;	//AT指令的互斥量
SemaphoreHandle_t  xMutex_STRATEGY 		= NULL;	//AT指令的互斥量
SemaphoreHandle_t  xMutex_EVENT_RECORD 	= NULL;	//事件记录的互斥量

/***************************消息队列相关*****************************/
QueueHandle_t xQueue_sensor 		= NULL;	//用于存储传感器的数据

/***************************系统心跳相关*****************************/
u32 SysTick1ms = 0;					//1ms滴答时钟
u32 SysTick10ms = 0;				//10ms滴答时钟
u32 SysTick100ms = 0;				//10ms滴答时钟
time_t SysTick1s = 0;				//1s滴答时钟

/***************************其他*****************************/
u8 GetTimeOK = 0;							//成功获取时间标志
pControlStrategy ControlStrategy = NULL;	//控制策略
u8 NeedUpdateStrategyList = 1;				//更新策略列表标志

/***************************复位参数*****************************/
u8 DeviceReset = 0;					//1:设备重启 2:除网络参数外，均恢复到出厂设置 3:恢复出厂设置
u8 ReConnectToServer = 0;			//重新连接服务器

/********************终端上行通信口通信参数**********************/
UpCommPortParameters_S UpCommPortPara;

/*************************主站IP和端口***************************/
ServerInfo_S ServerInfo;

/**********************终端事件记录配置参数***********************/
EventRecordConf_S EventRecordConf;

/************************设备基本信息数据*************************/
DeviceBaseInfo_S DeviceBaseInfo;

/**********************终端事件检测配置参数***********************/
EventDetectConf_S EventDetectConf;

/**************************RSA加密参数****************************/
RSA_PublicKey_S RSA_PublicKey;

/*********************控制器开关灯时间数据************************/
LampsSwitchProject_S LampsSwitchProject;

/*************************灯具基本参数****************************/
LampsParameters_S LampsParameters;

/***********************单灯运行模式数据**************************/
LampsRunMode_S LampsRunMode;

/*********************单灯节能运行模式数据************************/
EnergySavingMode_S EnergySavingMode[MAX_ENERGY_SAVING_MODE_NUM];

/***********************单灯预约控制数据**************************/
AppointmentControl_S AppointmentControl;

/*********************单灯照度开关阈值数据************************/
IlluminanceThreshold_S IlluminanceThreshold;

/*********************单灯开关模式选择数据************************/
u8 SwitchMode = 0;	//开关模式 1:年表控制 2:经纬度控制 3:光照度控制 4:常亮(常开)

/***********************单灯遥控操作数据*************************/
RemoteControl_S RemoteControl;
RemoteControl_S CurrentControl;

/*************************对时命令数据***************************/
SyncTime_S SyncTime;

/*************************终端信息数据***************************/
DeviceInfo_S DeviceInfo;

/**************************单灯电参数****************************/
DeviceElectricPara_S DeviceElectricPara;

/**************************NB模块参数****************************/
NB_ModulePara_S NB_ModulePara;

/*************************终端日历时钟***************************/
u8 CalendarClock[6];

/**************************事件计数器****************************/
EventRecordList_S EventRecordList;

/***********************FTP服务器配置参数************************/
FTP_ServerInfo_S FTP_ServerInfo;

/***************************固件参数*****************************/
FTP_FrameWareInfo_S FTP_FrameWareInfo;

/*************************固件升级状态***************************/
FrameWareState_S FrameWareState;




//在str1中查找str2，失败返回0xFF,成功返回str2首个元素在str1中的位置
u16 MyStrstr(u8 *str1, u8 *str2, u16 str1_len, u16 str2_len)
{
	u16 len = str1_len;
	if(str1_len == 0 || str2_len == 0)
	{
		return 0xFFFF;
	}
	else
	{
		while(str1_len >= str2_len)
		{
			str1_len --;
			if (!memcmp(str1, str2, str2_len))
			{
				return len - str1_len - 1;
			}
			str1 ++;
		}
		return 0xFFFF;
	}
}

//获得整数的位数
u8 GetDatBit(u32 dat)
{
	u8 j = 1;
	u32 i;
	i = dat;
	while(i >= 10)
	{
		j ++;
		i /= 10;
	}
	return j;
}

//用个位数换算出一个整数 1 10 100 1000......
u32 GetADV(u8 len)
{
	u32 count = 1;
	if(len == 1)
	{
		return 1;
	}
	else
	{
		len --;
		while(len --)
		{
			count *= 10;
		}
	}
	return count;
}

//整数转换为字符串
void IntToString(u8 *DString,u32 Dint,u8 zero_num)
{
	u16 i = 0;
	u8 j = GetDatBit(Dint);
	for(i = 0; i < GetDatBit(Dint) + zero_num; i ++)
	{
		DString[i] = Dint / GetADV(j) % 10 + 0x30;
		j --;
	}
}

u32 StringToInt(u8 *String)
{
	u8 len;
	u8 i;
	u32 count=0;
	u32 dev;

	len = strlen((char *)String);
	dev = 1;
	for(i = 0; i < len; i ++)//len-1
	{
		if(String[i] != '.')
		{
			count += ((String[i] - 0x30) * GetADV(len) / dev);
			dev *= 10;
		}
		else
		{
			len --;
			count /= 10;
		}
	}
	if(String[i]!=0x00)
	{
		count += (String[i] - 0x30);
	}
	return count;
}

/*
// C prototype : void HexToStr(BYTE *pbDest, BYTE *pbSrc, int nLen)
// parameter(s): [OUT] pbDest - 存放目标字符串
// [IN] pbSrc - 输入16进制数的起始地址
// [IN] nLen - 16进制数的字节数
// return value:
// remarks : 将16进制数转化为字符串
*/
void HexToStr(char *pbDest, u8 *pbSrc, u16 len)
{
	char ddl,ddh;
	int i;

	for (i = 0; i < len; i ++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;
		pbDest[i * 2] = ddh;
		pbDest[i * 2 + 1] = ddl;
	}

	pbDest[len * 2] = '\0';
}

/*
// C prototype : void StrToHex(BYTE *pbDest, BYTE *pbSrc, int nLen)
// parameter(s): [OUT] pbDest - 输出缓冲区
// [IN] pbSrc - 字符串
// [IN] nLen - 16进制数的字节数(字符串的长度/2)
// return value:
// remarks : 将字符串转化为16进制数
*/
void StrToHex(u8 *pbDest, char *pbSrc, u16 len)
{
	char h1,h2;
	u8 s1,s2;
	int i;

	for (i = 0; i < len; i ++)
	{
		h1 = pbSrc[2 * i];
		h2 = pbSrc[2 * i + 1];

		s1 = toupper(h1) - 0x30;
		if (s1 > 9)
		s1 -= 7;

		s2 = toupper(h2) - 0x30;
		if (s2 > 9)
		s2 -= 7;

		pbDest[i] = s1 * 16 + s2;
	}
}

//输入日期获得此日期在一年中的第几天 按照闰年计算
u16 get_day_num(u8 m,u8 d)
{
	u8 i = 0;
	u8 x[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};
	u16 s=0;

	for(i = 1; i < m; i ++)
	{
		s += x[i];			//整月的天数
	}

	s += (u16)d;			//日的天数

	return s;				//返回总天数,相对公元1年
}

//已知天数计算日期
void get_date_from_days(u16 days, u8 *m, u8 *d)
{
	u8 i = 0;
	u8 x[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};
	u16 s=0;

	for(i = 0; i < 12; i ++)
	{
		s += x[i];
		
		if(days > s && days <= s + x[i + 1])
		{
			*m = i + 1;
			*d = days - s;
		}
	}
}

//获取同一年中 两个日期之间差的天数 m1 d1为起始日期
s16 get_dates_diff(u8 m1,u8 d1,u8 m2,u8 d2)
{
	s16 diff = 0;
	u16 sum1;
	u16 sum2;

	sum1 = get_day_num(m1,d1);
	sum2 = get_day_num(m2,d2);

	if(sum2 < sum1)
	{
		diff = -1;
	}
	else
	{
		diff = sum2 - sum1;
	}

	return diff;
}

unsigned short find_str(unsigned char *s_str, unsigned char *p_str, unsigned short count, unsigned short *seek)
{
	unsigned short _count = 1;
    unsigned short len = 0;
    unsigned char *temp_str = NULL;
    unsigned char *temp_ptr = NULL;
    unsigned char *temp_char = NULL;

	(*seek) = 0;
    if(0 == s_str || 0 == p_str)
        return 0;
    for(temp_str = s_str; *temp_str != '\0'; temp_str++)
    {
        temp_char = temp_str;

        for(temp_ptr = p_str; *temp_ptr != '\0'; temp_ptr++)
        {
            if(*temp_ptr != *temp_char)
            {
                len = 0;
                break;
            }
            temp_char++;
            len++;
        }
        if(*temp_ptr == '\0')
        {
            if(_count == count)
                return len;
            else
            {
                _count++;
                len = 0;
            }
        }
        (*seek) ++;
    }
    return 0;
}

int search_str(unsigned char *source, unsigned char *target)
{
	unsigned short seek = 0;
    unsigned short len;
    len = find_str(source, target, 1, &seek);
    if(len == 0)
        return -1;
    else
        return len;
}

unsigned short get_str1(unsigned char *source, unsigned char *begin, unsigned short count1, unsigned char *end, unsigned short count2, unsigned char *out)
{
	unsigned short i;
    unsigned short len1;
    unsigned short len2;
    unsigned short index1 = 0;
    unsigned short index2 = 0;
    unsigned short length = 0;
    len1 = find_str(source, begin, count1, &index1);
    len2 = find_str(source, end, count2, &index2);
    length = index2 - index1 - len1;
    if((len1 != 0) && (len2 != 0))
    {
        for( i = 0; i < index2 - index1 - len1; i++)
            out[i] = source[index1 + len1 + i];
        out[i] = '\0';
    }
    else
    {
        out[0] = '\0';
    }
    return length;
}

unsigned short get_str2(unsigned char *source, unsigned char *begin, unsigned short count, unsigned short length, unsigned char *out)
{
	unsigned short i = 0;
    unsigned short len1 = 0;
    unsigned short index1 = 0;
    len1 = find_str(source, begin, count, &index1);
    if(len1 != 0)
    {
        for(i = 0; i < length; i++)
            out[i] = source[index1 + len1 + i];
        out[i] = '\0';
    }
    else
    {
        out[0] = '\0';
    }
    return length;
}

unsigned short get_str3(unsigned char *source, unsigned char *out, unsigned short length)
{
	unsigned short i = 0;
    for (i = 0 ; i < length ; i++)
    {
        out[i] = source[i];
    }
    out[i] = '\0';
    return length;
}

//32位CRC校验
u32 CRC32( const u8 *buf, u32 size)
{
     uint32_t i, crc;
     crc = 0xFFFFFFFF;
     for (i = 0; i < size; i++)
      crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
     return crc^0xFFFFFFFF;
}

/*****************************************************
函数：u16 CRC16(u8 *puchMsgg,u8 usDataLen)
功能：CRC校验用函数
参数：puchMsgg是要进行CRC校验的消息，usDataLen是消息中字节数 mode:0 低字节在前 1:高字节在前
返回：计算出来的CRC校验码。
*****************************************************/
u16 CRC16(u8 *puchMsgg,u16 usDataLen,u8 mode)
{
	u16 ret = 0;
    u8 uchCRCHi = 0xFF ; 											//高CRC字节初始化
    u8 uchCRCLo = 0xFF ; 											//低CRC 字节初始化
    u8 uIndex ; 													//CRC循环中的索引

    while (usDataLen--) 											//传输消息缓冲区
    {
		uIndex = uchCRCLo ^ *puchMsgg++; 							//计算CRC
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
		uchCRCHi = auchCRCLo[uIndex];
    }

	if(mode == 0)
	{
		ret = (((u16)uchCRCHi) << 8) + (u16)uchCRCLo;
	}
	else
	{
		ret = (((u16)uchCRCLo) << 8) + (u16)uchCRCHi;
	}

    return ret;
}

u16 GetCRC16(u8 *data,u16 len)
{
	u16 ax,lsb;
	int i,j;

	ax = 0xFFFF;

	for(i = 0; i < len; i ++)
	{
		ax ^= data[i];

		for(j = 0; j < 8; j ++)
		{
			lsb = ax & 0x0001;
			ax = ax >> 1;

			if(lsb != 0)
				ax ^= 0xA001;
		}
	}

	return ax;
}

//计算校验和
u8 CalCheckSum(u8 *buf, u16 len)
{
	u8 sum = 0;
	u16 i = 0;

	for(i = 0; i < len; i ++)
	{
		sum += *(buf + i);
	}

	return sum;
}

//产生一个系统1毫秒滴答时钟.
void SysTick1msAdder(void)
{
	SysTick1ms = (SysTick1ms + 1) & 0xFFFFFFFF;
}

//获取系统1毫秒滴答时钟
u32 GetSysTick1ms(void)
{
	return SysTick1ms;
}

//产生一个系统10毫秒滴答时钟.
void SysTick10msAdder(void)
{
	SysTick10ms = (SysTick10ms + 1) & 0xFFFFFFFF;
}

//获取系统10毫秒滴答时钟
u32 GetSysTick10ms(void)
{
	return SysTick10ms;
}

//产生一个系统100毫秒滴答时钟.
void SysTick100msAdder(void)
{
	SysTick100ms = (SysTick100ms + 1) & 0xFFFFFFFF;
}

//获取系统100毫秒滴答时钟
u32 GetSysTick100ms(void)
{
	return SysTick1ms;
}

void SetSysTick1s(time_t sec)
{
	SysTick1s = sec;
}

//获取系统1秒滴答时钟
time_t GetSysTick1s(void)
{
	return SysTick1s;
}

//获取RTC状态
u8 GetRTC_State(void)
{
	u8 ret = 0;
	
	if(calendar.w_year >= 2019)
	{
		if(GetTimeOK == 0)
		{
			ret = 2;
		}
		else if(GetTimeOK == 1)
		{
			ret = 1;
		}
	}
	
	return ret;
}

//从EEPROM中读取数据(带CRC16校验码)len包括CRC16校验码
u8 ReadDataFromEepromToMemory(u8 *buf,u16 s_add, u16 len)
{
	u16 i = 0;
	u16 j = 0;
	u16 ReadCrcCode;
	u16 CalCrcCode = 0;

	for(i = s_add,j = 0; i < s_add + len; i ++, j++)
	{
		*(buf + j) = AT24CXX_ReadOneByte(i);
	}

	ReadCrcCode = (u16)(*(buf + len - 2));
	ReadCrcCode = ReadCrcCode << 8;
	ReadCrcCode = ReadCrcCode | (u16)(*(buf + len - 1));

	CalCrcCode = CRC16(buf,len - 2,1);

	if(ReadCrcCode == CalCrcCode)
	{
		return 1;
	}

	return 0;
}

//向EEPROM中写入数据(带CRC16校验码)len不包括CRC16校验码
void WriteDataFromMemoryToEeprom(u8 *inbuf,u16 s_add, u16 len)
{
	u16 i = 0;
	u16 j = 0;
	u16 CalCrcCode = 0;

	CalCrcCode = CRC16(inbuf,len,1);

	for(i = s_add ,j = 0; i < s_add + len; i ++, j ++)			//写入原始数据
	{
		AT24CXX_WriteOneByte(i,*(inbuf + j));
	}

	AT24CXX_WriteOneByte(s_add + len + 0,(u8)(CalCrcCode >> 8));		//写入CRC
	AT24CXX_WriteOneByte(s_add + len + 1,(u8)(CalCrcCode & 0x00FF));
}

//将数字或者缓冲区当中的数据转换成字符串，并赋值给相应的指针
//type 0:转换数字id 1:转换缓冲区数据，add为缓冲区起始地址 2将字符串长度传到参数size中
u8 GetMemoryForString(u8 **str, u8 type, u32 id, u16 add, u16 size, u8 *hold_reg)
{
	u8 ret = 0;
	u8 len = 0;
	u8 new_len = 0;

	if(*str == NULL)
	{
		if(type == 0)
		{
			len = GetDatBit(id);
		}
		else if(type == 1)
		{
			len = *(hold_reg + add);
		}
		else if(type == 2)
		{
			len = size;
		}

		*str = (u8 *)mymalloc(sizeof(u8) * len + 1);
	}

	if(*str != NULL)
	{
		len = strlen((char *)*str);
		if(type == 0)
		{
			new_len = GetDatBit(id);
		}
		else if(type == 1)
		{
			new_len = *(hold_reg + add);
		}
		else if(type == 2)
		{
			new_len = size;

			add -= 1;
		}

		if(len == new_len)
		{
			memset(*str,0,new_len + 1);

			if(type == 0)
			{
				IntToString(*str,id,0);
			}
			else if(type == 1 || type == 2)
			{
				memcpy(*str,(hold_reg + add + 1),new_len);
			}
			ret = 1;
		}
		else
		{
			myfree(*str);
			*str = (u8 *)mymalloc(sizeof(u8) * new_len + 1);
			if(*str != NULL)
			{
				memset(*str,0,new_len + 1);

				if(type == 0)
				{
					IntToString(*str,id,0);
				}
				else if(type == 1 || type == 2)
				{
					memcpy(*str,(hold_reg + add + 1),new_len);
				}
				len = new_len;
				new_len = 0;
				ret = 1;
			}
		}
	}

	return ret;
}

//将内存中的数据拷贝到指定指针所指的内存中
u8 GetMemoryForSpecifyPointer(u8 **str,u16 size, u8 *memory)
{
	u8 ret = 0;
	u8 len = 0;
	u8 new_len = 0;

	if(*str == NULL)
	{
		len = size;

		*str = (u8 *)mymalloc(sizeof(u8) * len + 1);
	}

	if(*str != NULL)
	{
		len = strlen((char *)*str);

		new_len = size;

		if(len == new_len)
		{
			memset(*str,0,new_len + 1);

			memcpy(*str,memory,new_len);

			ret = 1;
		}
		else
		{
			myfree(*str);
			*str = (u8 *)mymalloc(sizeof(u8) * new_len + 1);

			if(*str != NULL)
			{
				memset(*str,0,new_len + 1);

				memcpy(*str,memory,new_len);

				len = new_len;
				new_len = 0;
				ret = 1;
			}
		}
	}

	return ret;
}

u8 GetIpAdderssFromMemory(u8 **ip,u8 *memory)
{
	u8 ret = 0;
	char ipbuf[16];
	char tmp[10];
	u8 len = 0;
	u8 new_len = 0;

	memset(ipbuf,0,16);
	memset(tmp,0,10);

	nbiot_itoa(*(memory + 0),tmp,10);
	strcat(ipbuf,tmp);
	strcat(ipbuf,".");

	nbiot_itoa(*(memory + 1),tmp,10);
	strcat(ipbuf,tmp);
	strcat(ipbuf,".");

	nbiot_itoa(*(memory + 2),tmp,10);
	strcat(ipbuf,tmp);
	strcat(ipbuf,".");

	nbiot_itoa(*(memory + 3),tmp,10);
	strcat(ipbuf,tmp);

	if(*ip == NULL)
	{
		len = strlen(ipbuf);

		*ip = (u8 *)mymalloc(sizeof(u8) * len + 1);
	}

	if(*ip != NULL)
	{
		len = strlen((char *)*ip);

		new_len = strlen(ipbuf);

		if(len == new_len)
		{
			memset(*ip,0,new_len + 1);

			memcpy(*ip,ipbuf,new_len);

			ret = 1;
		}
		else
		{
			myfree(*ip);
			*ip = (u8 *)mymalloc(sizeof(u8) * new_len + 1);

			if(*ip != NULL)
			{
				memset(*ip,0,new_len + 1);

				memcpy(*ip,ipbuf,new_len);

				len = new_len;
				new_len = 0;
				ret = 1;
			}
		}
	}

	return ret;
}

u8 GetPortFromMemory(u8 **port,u8 *memory)
{
	u8 ret = 0;
	char portbuf[16];
	char tmp[10];
	u16 port_num = 0;
	u8 len = 0;
	u8 new_len = 0;

	port_num = ((((u16)(*(memory + 1))) << 8) + (u16)(*(memory + 0)));

	memset(portbuf,0,16);
	memset(tmp,0,10);

	nbiot_itoa(port_num,tmp,10);
	strcat(portbuf,tmp);

	if(*port == NULL)
	{
		len = strlen(portbuf);

		*port = (u8 *)mymalloc(sizeof(u8) * len + 1);
	}

	if(*port != NULL)
	{
		len = strlen((char *)*port);

		new_len = strlen(portbuf);

		if(len == new_len)
		{
			memset(*port,0,new_len + 1);

			memcpy(*port,portbuf,new_len);

			ret = 1;
		}
		else
		{
			myfree(*port);
			*port = (u8 *)mymalloc(sizeof(u8) * new_len + 1);

			if(*port != NULL)
			{
				memset(*port,0,new_len + 1);

				memcpy(*port,portbuf,new_len);

				len = new_len;
				new_len = 0;
				ret = 1;
			}
		}
	}

	return ret;
}

//将字符串拷贝到指定地址
u8 CopyStrToPointer(u8 **pointer, u8 *str, u8 len)
{
	u8 ret = 0;

	if(*pointer == NULL)
	{
		*pointer = (u8 *)mymalloc(len + 1);
	}
	else if(*pointer != NULL)
	{
		myfree(*pointer);
		*pointer = (u8 *)mymalloc(sizeof(u8) * len + 1);
	}

	if(*pointer != NULL)
	{
		memset(*pointer,0,len + 1);

		memcpy(*pointer,str,len);

		ret = 1;
	}

	return ret;
}

//读取上行通讯口通讯参数
u8 ReadUpCommPortPara(void)
{
	u8 ret = 0;
	u8 buf[UP_COMM_PARA_LEN];

	memset(buf,0,UP_COMM_PARA_LEN);

	ret = ReadDataFromEepromToMemory(buf,UP_COMM_PARA_ADD,UP_COMM_PARA_LEN);

	if(ret == 1)
	{
		UpCommPortPara.wait_slave_rsp_timeout 		= ((((u16)(*(buf + 1))) << 8) & 0x0F00) + (u16)(*(buf + 0));
		UpCommPortPara.master_retry_times 			= (*(buf + 1) >> 4) & 0x03;
		UpCommPortPara.need_master_confirm 			= *(buf + 2);
		UpCommPortPara.heart_beat_cycle 			= *(buf + 3);
		UpCommPortPara.random_peak_staggering_time 	= ((((u16)(*(buf + 5))) << 8) + (u16)(*(buf + 4)));
		UpCommPortPara.specify_peak_staggering_time = ((((u16)(*(buf + 7))) << 8) + (u16)(*(buf + 6)));
	}
	else
	{
		UpCommPortPara.wait_slave_rsp_timeout 		= 30;
		UpCommPortPara.master_retry_times 			= 3;
		UpCommPortPara.need_master_confirm 			= 1;
		UpCommPortPara.heart_beat_cycle 			= 30;
		UpCommPortPara.random_peak_staggering_time 	= 50;
		UpCommPortPara.specify_peak_staggering_time = 0;
	}

	return ret;
}

//主站服务器配置信息
u8 ReadServerInfo(void)
{
	u8 ret = 0;
	u8 buf[SERVER_INFO_LEN];
	u8 temp_buf[64];

	memset(buf,0,SERVER_INFO_LEN);

	ret = ReadDataFromEepromToMemory(buf,SERVER_INFO_ADD,SERVER_INFO_LEN);

	if(ret == 1)
	{
		GetIpAdderssFromMemory(&ServerInfo.ip1,buf + 0);
		GetPortFromMemory(&ServerInfo.port1,buf + 4);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 6,16);
		GetMemoryForSpecifyPointer(&ServerInfo.apn,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 22,32);
		GetMemoryForSpecifyPointer(&ServerInfo.user_name,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 54,32);
		GetMemoryForSpecifyPointer(&ServerInfo.pwd,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 86,32);
		GetMemoryForSpecifyPointer(&ServerInfo.domain1,strlen((char *)temp_buf), temp_buf);

		GetIpAdderssFromMemory(&ServerInfo.ip2,buf + 118);
		GetPortFromMemory(&ServerInfo.port2,buf + 122);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 124,32);
		GetMemoryForSpecifyPointer(&ServerInfo.domain2,strlen((char *)temp_buf), temp_buf);
	}
	else
	{
		if(ServerInfo.ip1 == NULL)
		{
			ServerInfo.ip1 = (u8 *)mymalloc(sizeof(u8) * 16);
			memset(ServerInfo.ip1,0,16);
			sprintf((char *)ServerInfo.ip1, "183.207.215.143");
		}

		if(ServerInfo.port1 == NULL)
		{
			ServerInfo.port1 = (u8 *)mymalloc(sizeof(u8) * 6);
			memset(ServerInfo.port1,0,6);
			sprintf((char *)ServerInfo.port1, "5683");
		}

		if(ServerInfo.apn == NULL)
		{
			ServerInfo.apn = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(ServerInfo.apn,0,5);
			sprintf((char *)ServerInfo.apn, "NULL");
		}

		if(ServerInfo.user_name == NULL)
		{
			ServerInfo.user_name = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(ServerInfo.user_name,0,5);
			sprintf((char *)ServerInfo.user_name, "NULL");
		}

		if(ServerInfo.pwd == NULL)
		{
			ServerInfo.pwd = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(ServerInfo.pwd,0,5);
			sprintf((char *)ServerInfo.pwd, "NULL");
		}

		if(ServerInfo.domain1 == NULL)
		{
			ServerInfo.domain1 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(ServerInfo.domain1,0,5);
			sprintf((char *)ServerInfo.domain1, "NULL");
		}

		if(ServerInfo.ip2 == NULL)
		{
			ServerInfo.ip2 = (u8 *)mymalloc(sizeof(u8) * 16);
			memset(ServerInfo.ip2,0,16);
			sprintf((char *)ServerInfo.ip2, "183.207.215.143");
		}

		if(ServerInfo.port2 == NULL)
		{
			ServerInfo.port2 = (u8 *)mymalloc(sizeof(u8) * 6);
			memset(ServerInfo.port2,0,6);
			sprintf((char *)ServerInfo.port2, "5683");
		}

		if(ServerInfo.domain2 == NULL)
		{
			ServerInfo.domain2 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(ServerInfo.domain2,0,5);
			sprintf((char *)ServerInfo.domain2, "NULL");
		}
	}

	return ret;
}

//终端事件记录配置设置
u8 ReadEventRecordConf(void)
{
	u8 ret = 0;
	u8 buf[ER_OTHER_CONF_LEN];

	memset(buf,0,ER_OTHER_CONF_LEN);

	ret = ReadDataFromEepromToMemory(buf,ER_OTHER_CONF_ADD,ER_OTHER_CONF_LEN);

	if(ret == 1)
	{
		memcpy(EventRecordConf.effective,buf + 0,8);
		memcpy(EventRecordConf.important,buf + 8,8);
		memcpy(EventRecordConf.auto_report,buf + 16,8);
	}
	else
	{
		memset(EventRecordConf.effective,0xFF,8);
		memset(EventRecordConf.important,0xFF,8);
		memset(EventRecordConf.auto_report,0xFF,8);
	}

	return ret;
}

//设备基本信息
u8 ReadDeviceBaseInfo(void)
{
	u8 ret = 0;
	u8 buf[DEV_BASIC_INFO_LEN];
	u8 default_mail_id[8] = {0x00,0x11,0x20,0x01,0x00,0x00,0x00,0x20};	//设备缺省通信地址

	memset(buf,0,DEV_BASIC_INFO_LEN);

	ret = ReadDataFromEepromToMemory(buf,DEV_BASIC_INFO_ADD,DEV_BASIC_INFO_LEN);

	if(ret == 1)
	{
		memcpy(DeviceBaseInfo.id,buf + 0,6);
		memcpy(DeviceBaseInfo.mail_add,buf + 6,8);
		memcpy(DeviceBaseInfo.longitude,buf + 14,5);
		memcpy(DeviceBaseInfo.latitude,buf + 19,5);
	}
	else
	{
		memcpy(DeviceBaseInfo.mail_add,default_mail_id,8);

		memset(DeviceBaseInfo.id,0,6);
		memset(DeviceBaseInfo.longitude,0,5);
		memset(DeviceBaseInfo.latitude,0,5);
	}

	return ret;
}

//终端事件检测配置时间参数
u8 ReadEventDetectTimeConf(void)
{
	u8 ret = 0;
	u8 buf[ER_TIME_CONF_LEN];

	memset(buf,0,ER_TIME_CONF_LEN);

	ret = ReadDataFromEepromToMemory(buf,ER_TIME_CONF_ADD,ER_TIME_CONF_LEN);

	if(ret == 1)
	{
		EventDetectConf.comm_falut_detect_interval 		= *(buf + 0);
		EventDetectConf.router_fault_detect_interval	= *(buf + 1);
		EventDetectConf.turn_on_collect_delay 			= *(buf + 2);
		EventDetectConf.turn_off_collect_delay 			= *(buf + 3);
		EventDetectConf.current_detect_delay 			= *(buf + 4);
	}
	else
	{
		EventDetectConf.comm_falut_detect_interval 		= 3;
		EventDetectConf.router_fault_detect_interval	= 3;
		EventDetectConf.turn_on_collect_delay 			= 1;
		EventDetectConf.turn_off_collect_delay 			= 1;
		EventDetectConf.current_detect_delay 			= 10;
	}

	return ret;
}

//终端事件检测配置阈值参数
u8 ReadEventDetectThreConf(void)
{
	u8 ret = 0;
	u8 buf[ER_THRE_CONF_LEN];

	memset(buf,0,ER_THRE_CONF_LEN);

	ret = ReadDataFromEepromToMemory(buf,ER_THRE_CONF_ADD,ER_THRE_CONF_LEN);

	if(ret == 1)
	{
		EventDetectConf.over_current_ratio 			= *(buf + 0);
		EventDetectConf.over_current_recovery_ratio = *(buf + 1);
		EventDetectConf.low_current_ratio 			= *(buf + 2);
		EventDetectConf.low_current_recovery_ratio 	= *(buf + 3);

		memcpy(EventDetectConf.capacitor_fault_pf_ratio,buf + 4,2);
		memcpy(EventDetectConf.capacitor_fault_recovery_pf_ratio,buf + 6,2);

		EventDetectConf.lamps_over_current_ratio 			= *(buf + 8);
		EventDetectConf.lamps_over_current_recovery_ratio 	= *(buf + 9);
		EventDetectConf.fuse_over_current_ratio 			= *(buf + 10);
		EventDetectConf.fuse_over_current_recovery_ratio 	= *(buf + 11);
		EventDetectConf.leakage_over_current_ratio 			= *(buf + 12);
		EventDetectConf.leakage_over_current_recovery_ratio = *(buf + 13);
		EventDetectConf.leakage_over_voltage_ratio 			= *(buf + 14);
		EventDetectConf.leakage_over_voltage_recovery_ratio = *(buf + 15);
	}
	else
	{
		EventDetectConf.over_current_ratio 			= 10;
		EventDetectConf.over_current_recovery_ratio = 5;
		EventDetectConf.low_current_ratio 			= 10;
		EventDetectConf.low_current_recovery_ratio 	= 5;

		EventDetectConf.capacitor_fault_pf_ratio[0] = 0x00;				//50 见协议附录a.5
		EventDetectConf.capacitor_fault_pf_ratio[1] = 0x05;

		EventDetectConf.capacitor_fault_recovery_pf_ratio[0] = 0x00;	//70 见协议附录a.5
		EventDetectConf.capacitor_fault_recovery_pf_ratio[1] = 0x07;

		EventDetectConf.lamps_over_current_ratio 			= 50;
		EventDetectConf.lamps_over_current_recovery_ratio 	= 100;
		EventDetectConf.fuse_over_current_ratio 			= 50;
		EventDetectConf.fuse_over_current_recovery_ratio 	= 100;
		EventDetectConf.leakage_over_current_ratio 			= 100;
		EventDetectConf.leakage_over_current_recovery_ratio = 80;
		EventDetectConf.leakage_over_voltage_ratio 			= 60;
		EventDetectConf.leakage_over_voltage_recovery_ratio = 36;
	}

	return ret;
}

//RSA加密参数
u8 ReadRSA_PublicKey(void)
{
	u8 ret = 0;
	u8 buf[RSA_ENCRY_PARA_LEN];

	memset(buf,0,RSA_ENCRY_PARA_LEN);

	ret = ReadDataFromEepromToMemory(buf,RSA_ENCRY_PARA_ADD,RSA_ENCRY_PARA_LEN);

	if(ret == 1)
	{
		RSA_PublicKey.enable = *(buf + 0);
		memcpy(RSA_PublicKey.n,buf + 1,128);
		memcpy(RSA_PublicKey.el,buf + 129,128);
	}
	else
	{
		RSA_PublicKey.enable = 0;
		memset(RSA_PublicKey.n,0,128);
		memset(RSA_PublicKey.el,0,128);
	}

	return ret;
}

//控制器开关灯时间数据
u8 ReadLampsSwitchProject(void)
{
	u8 ret = 0;
	u16 i = 0;
	u8 buf[SWITCH_DATE_DAYS_LEN];

	memset(buf,0,SWITCH_DATE_DAYS_LEN);

	ret = ReadDataFromEepromToMemory(buf,SWITCH_DATE_DAYS_ADD,SWITCH_DATE_DAYS_LEN);

	if(ret == 1)
	{
		LampsSwitchProject.start_month = *(buf + 0);
		LampsSwitchProject.start_date  = *(buf + 1);

		LampsSwitchProject.total_days = ((((u16)(*(buf + 3))) << 8) + (u16)(*(buf + 2)));
	}
	else
	{
		LampsSwitchProject.start_month = 0xFF;
		LampsSwitchProject.start_date  = 0xFF;

		LampsSwitchProject.total_days = 0;
	}
	
	for(i = 0; i < LampsSwitchProject.total_days; i ++)
	{
		memset(buf,0,SWITCH_TIME_LEN);

		ret = ReadDataFromEepromToMemory(buf,SWITCH_TIME_ADD + i * SWITCH_TIME_LEN,SWITCH_TIME_LEN);

		if(ret == 1)
		{
			LampsSwitchProject.switch_time[i].on_time[0]  = *(buf + 0);
			LampsSwitchProject.switch_time[i].on_time[1]  = *(buf + 1);
			LampsSwitchProject.switch_time[i].off_time[0] = *(buf + 2);
			LampsSwitchProject.switch_time[i].off_time[1] = *(buf + 3);
		}
		else
		{
			LampsSwitchProject.switch_time[i].on_time[0]  = 0xFF;
			LampsSwitchProject.switch_time[i].on_time[1]  = 0xFF;
			LampsSwitchProject.switch_time[i].off_time[0] = 0xFF;
			LampsSwitchProject.switch_time[i].off_time[1] = 0xFF;
		}
	}

	return ret;
}

//灯具基本参数
u8 ReadLampsParameters(void)
{
	u8 ret = 0;
	u16 i = 0;
	u8 buf[LAMPS_PARA_LEN];

	memset(buf,0,LAMPS_PARA_LEN);

	ret = ReadDataFromEepromToMemory(buf,LAMPS_NUM_PARA_ADD,LAMPS_NUM_PARA_LEN);

	if(ret == 1)
	{
		LampsParameters.num = ((((u16)(*(buf + 1))) << 8) + (u16)(*(buf + 0)));
	}
	else
	{
		LampsParameters.num = 1;
	}

	for(i = 0; i < LampsParameters.num; i ++)
	{
		memset(buf,0,LAMPS_PARA_LEN);

		ret = ReadDataFromEepromToMemory(buf,LAMPS_PARA_ADD + i * LAMPS_PARA_LEN,LAMPS_PARA_LEN);

		if(ret == 1)
		{
			LampsParameters.parameters[i].lamps_id  	= ((((u16)(*(buf + 1))) << 8) + (u16)(*(buf + 0)));
			LampsParameters.parameters[i].type  		= *(buf + 2);
			LampsParameters.parameters[i].power  		= ((((u16)(*(buf + 4))) << 8) + (u16)(*(buf + 3)));
			LampsParameters.parameters[i].pf  			= ((((u16)(*(buf + 6))) << 8) + (u16)(*(buf + 5)));
			LampsParameters.parameters[i].enable  		= *(buf + 7);
			LampsParameters.parameters[i].line_id  		= *(buf + 8);
			LampsParameters.parameters[i].a_b_c  		= *(buf + 9);
			LampsParameters.parameters[i].pole_number	= ((((u16)(*(buf + 11))) << 8) + (u16)(*(buf + 10)));
		}
		else
		{
			LampsParameters.parameters[i].lamps_id  	= 0x0001;		//灯具ID
			LampsParameters.parameters[i].type  		= 0x02;			//灯具类型 LED灯具
			LampsParameters.parameters[i].power  		= 0x0078;		//功率120W
			LampsParameters.parameters[i].pf  			= 0x60;			//功率因数96%
			LampsParameters.parameters[i].enable  		= 0x01;			//启用标志
			LampsParameters.parameters[i].line_id  		= 0x01;			//所处出线序号
			LampsParameters.parameters[i].a_b_c  		= 0x00;			//所处相别
			LampsParameters.parameters[i].pole_number	= 0x01;			//灯具所在杆号
		}
	}

	return ret;
}

//单灯运行模式数据
u8 ReadLampsRunMode(void)
{
	u8 ret = 0;
	u16 i = 0;
	u8 buf[LAMPS_MODE_LEN];

	memset(buf,0,LAMPS_MODE_LEN);

	ret = ReadDataFromEepromToMemory(buf,LAMPS_NUM_MODE_ADD,LAMPS_NUM_MODE_LEN);

	if(ret == 1)
	{
		LampsRunMode.num = *(buf + 0);
	}
	else
	{
		LampsRunMode.num = 1;
	}

	for(i = 0; i < LampsRunMode.num; i ++)
	{
		memset(buf,0,LAMPS_MODE_LEN);

		ret = ReadDataFromEepromToMemory(buf,LAMPS_MODE_ADD + i * LAMPS_MODE_LEN,LAMPS_MODE_LEN);

		if(ret == 1)
		{
			LampsRunMode.run_mode[i].lamps_id  				= ((((u16)(*(buf + 1))) << 8) + (u16)(*(buf + 0)));
			LampsRunMode.run_mode[i].initial_brightness  	= *(buf + 2);
			LampsRunMode.run_mode[i].energy_saving_mode_id  = *(buf + 3);
		}
		else
		{
			LampsRunMode.run_mode[i].lamps_id  				= 0x0001;	//灯具序号
			LampsRunMode.run_mode[i].initial_brightness  	= 100;		//默认亮度100%
			LampsRunMode.run_mode[i].energy_saving_mode_id  = 0x01;		//策略(节能模式)编号
		}
	}

	return ret;
}

//单灯节能运行模式
u8 ReadEnergySavingMode(void)
{
	u8 ret = 0;
	u16 i = 0;
	u16 j = 0;
	u8 buf[E_SAVE_MODE_LABLE_LEN];

	memset(buf,0,E_SAVE_MODE_LABLE_LEN);

	for(i = 0; i < MAX_ENERGY_SAVING_MODE_NUM; i ++)
	{
		ret = ReadDataFromEepromToMemory(buf,
		                                 E_SAVE_MODE_LABLE_ADD + i * E_SAVE_MODE_LEN,
		                                 E_SAVE_MODE_LABLE_LEN);

		if(ret == 1)
		{
			EnergySavingMode[i].mode_id = *(buf + 0);
			memcpy(EnergySavingMode[i].mode_name,buf + 1,32);
			EnergySavingMode[i].control_times = *(buf + 33);
		}
		else
		{
			EnergySavingMode[i].mode_id = 0;			//模式编号
			memset(EnergySavingMode[i].mode_name,0,32);
			memcpy(EnergySavingMode[i].mode_name,"DEFAULT ENERGY SAVING MODE",26);	//模式名称
			EnergySavingMode[i].control_times = 0;		//控制次数
		}
		
		for(j = 0; j < EnergySavingMode[i].control_times; j ++)
		{
			ret = ReadDataFromEepromToMemory(buf,
											 E_SAVE_MODE_CONTENT_ADD + i * E_SAVE_MODE_LEN + j * E_SAVE_MODE_CONTENT_LEN,
											 E_SAVE_MODE_CONTENT_LEN);

			if(ret == 1)
			{
				EnergySavingMode[i].operation[j].mode 			= *(buf + 0);
				EnergySavingMode[i].operation[j].control_type 	= *(buf + 1);
				EnergySavingMode[i].operation[j].brightness 	= *(buf + 2);
				EnergySavingMode[i].operation[j].relative_time 	= *(buf + 3);
				memcpy(EnergySavingMode[i].operation[j].absolute_time,buf + 4,6);
			}
			else
			{
				EnergySavingMode[i].operation[j].mode 			= 0x01;			//绝对时间
				EnergySavingMode[i].operation[j].control_type 	= 0;			//开灯
				EnergySavingMode[i].operation[j].brightness 	= 100;			//亮度100%
				EnergySavingMode[i].operation[j].relative_time 	= 0;			//相对时间 单位5min
				memset(EnergySavingMode[i].operation[j].absolute_time,0,6);		//绝对时间
			}
		}
	}

	return ret;
}

//单灯预约控制数据
u8 ReadAppointmentControl(void)
{
	u8 ret = 0;
	u16 i = 0;
	u8 buf[APPOIN_LABLE_LEN];

	memset(buf,0,APPOIN_LABLE_LEN);

	ret = ReadDataFromEepromToMemory(buf,APPOIN_LABLE_ADD,APPOIN_LABLE_LEN);

	if(ret == 1)
	{
		AppointmentControl.appointment_id = *(buf + 0);
		memcpy(AppointmentControl.start_date,buf + 1,6);
		memcpy(AppointmentControl.end_date,buf + 7,6);
		AppointmentControl.lamps_num = *(buf + 13);
	}
	else
	{
		AppointmentControl.appointment_id = 0;		//预约配置编号
		memset(AppointmentControl.start_date,0,6);	//起始日期
		memset(AppointmentControl.end_date,0,6);	//结束日期
		AppointmentControl.lamps_num = 0;			//灯数
	}
	
	for(i = 0; i < AppointmentControl.lamps_num; i ++)
	{
		memset(buf,0,APPOIN_CONTENT_LEN);

		ret = ReadDataFromEepromToMemory(buf,APPOIN_CONTENT_ADD + i * APPOIN_CONTENT_LEN,APPOIN_CONTENT_LEN);

		if(ret == 1)
		{
			AppointmentControl.run_mode[i].lamps_id 				= ((((u16)(*(buf + 1))) << 8) + (u16)(*(buf + 0)));
			AppointmentControl.run_mode[i].initial_brightness 		= *(buf + 2);
			AppointmentControl.run_mode[i].energy_saving_mode_id 	= *(buf + 3);
		}
		else
		{
			AppointmentControl.run_mode[i].lamps_id 				= 0x00;	//灯具序号
			AppointmentControl.run_mode[i].initial_brightness 		= 100;	//默认亮度100%
			AppointmentControl.run_mode[i].energy_saving_mode_id 	= 0;	//节能模式编号
		}
	}

	return ret;
}

//开关光照阈值
u8 ReadIlluminanceThreshold(void)
{
	u8 ret = 0;
	u8 buf[ILLUM_THRE_LEN];

	memset(buf,0,ILLUM_THRE_LEN);

	ret = ReadDataFromEepromToMemory(buf,ILLUM_THRE_ADD,ILLUM_THRE_LEN);

	if(ret == 1)
	{
		IlluminanceThreshold.on 	= ((((u16)(*(buf + 1))) << 8) + (u16)(*(buf + 0)));
		IlluminanceThreshold.off 	= ((((u16)(*(buf + 3))) << 8) + (u16)(*(buf + 2)));
	}
	else
	{
		IlluminanceThreshold.on 	= 25;
		IlluminanceThreshold.off 	= 35;
	}

	return ret;
}

//开关模式
u8 ReadSwitchMode(void)
{
	u8 ret = 0;
	u8 buf[SWITCH_MODE_LEN];

	memset(buf,0,SWITCH_MODE_LEN);

	ret = ReadDataFromEepromToMemory(buf,SWITCH_MODE_ADD,SWITCH_MODE_LEN);

	if(ret == 1)
	{
		SwitchMode = *(buf + 0);
	}
	else
	{
		SwitchMode = 4;
	}

	return ret;
}

//复位控制命令
void ResetRemoteCurrentControl(void)
{
	RemoteControl._switch = 1;
	RemoteControl.lock = 0;
	RemoteControl.control_type = 0;
	RemoteControl.brightness = 100;
	RemoteControl.lamps_id = 1;
	RemoteControl.interface = INTFC_0_10V;
	RemoteControl.current = 0.0f;
	RemoteControl.voltage = 0.0f;
	
	CurrentControl._switch = 1;
	CurrentControl.lock = 0;
	CurrentControl.control_type = 0;
	CurrentControl.brightness = 100;
	CurrentControl.lamps_id = 1;
	CurrentControl.interface = INTFC_0_10V;
	CurrentControl.current = 0.0f;
	CurrentControl.voltage = 0.0f;
}

//FTP服务器信息
u8 ReadFTP_ServerInfo(void)
{
	u8 ret = 0;
	u8 buf[E_FTP_SERVER_INFO_LEN];
	u8 temp_buf[64];

	memset(buf,0,E_FTP_SERVER_INFO_LEN);

	ret = ReadDataFromEepromToMemory(buf,E_FTP_SERVER_INFO_ADD,E_FTP_SERVER_INFO_LEN);

	if(ret == 1)
	{
		GetIpAdderssFromMemory(&FTP_ServerInfo.ip1,buf + 0);
		GetPortFromMemory(&FTP_ServerInfo.port1,buf + 4);
		GetIpAdderssFromMemory(&FTP_ServerInfo.ip2,buf + 6);
		GetPortFromMemory(&FTP_ServerInfo.port2,buf + 8);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 12,10);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.user_name,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 22,10);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.pwd,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 32,40);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.path1,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 72,40);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.path2,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 112,20);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.file1,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 132,20);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.file2,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 152,20);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.file3,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 172,20);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.file4,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 192,20);
		GetMemoryForSpecifyPointer(&FTP_ServerInfo.file5,strlen((char *)temp_buf), temp_buf);
	}
	else
	{
		if(FTP_ServerInfo.ip1 == NULL)
		{
			FTP_ServerInfo.ip1 = (u8 *)mymalloc(sizeof(u8) * 16);
			memset(FTP_ServerInfo.ip1,0,16);
			sprintf((char *)FTP_ServerInfo.ip1, "183.207.215.143");
		}

		if(FTP_ServerInfo.port1 == NULL)
		{
			FTP_ServerInfo.port1 = (u8 *)mymalloc(sizeof(u8) * 6);
			memset(FTP_ServerInfo.port1,0,6);
			sprintf((char *)FTP_ServerInfo.port1, "5683");
		}

		if(FTP_ServerInfo.ip2 == NULL)
		{
			FTP_ServerInfo.ip2 = (u8 *)mymalloc(sizeof(u8) * 16);
			memset(FTP_ServerInfo.ip2,0,16);
			sprintf((char *)FTP_ServerInfo.ip2, "183.207.215.143");
		}

		if(FTP_ServerInfo.port2 == NULL)
		{
			FTP_ServerInfo.port2 = (u8 *)mymalloc(sizeof(u8) * 6);
			memset(FTP_ServerInfo.port2,0,6);
			sprintf((char *)FTP_ServerInfo.port2, "5683");
		}

		if(FTP_ServerInfo.user_name == NULL)
		{
			FTP_ServerInfo.user_name = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.user_name,0,5);
			sprintf((char *)FTP_ServerInfo.user_name, "NULL");
		}

		if(FTP_ServerInfo.user_name == NULL)
		{
			FTP_ServerInfo.user_name = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.user_name,0,5);
			sprintf((char *)FTP_ServerInfo.user_name, "NULL");
		}

		if(FTP_ServerInfo.pwd == NULL)
		{
			FTP_ServerInfo.pwd = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.pwd,0,5);
			sprintf((char *)FTP_ServerInfo.pwd, "NULL");
		}

		if(FTP_ServerInfo.path1 == NULL)
		{
			FTP_ServerInfo.path1 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.path1,0,5);
			sprintf((char *)FTP_ServerInfo.path1, "NULL");
		}

		if(FTP_ServerInfo.path2 == NULL)
		{
			FTP_ServerInfo.path2 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.path2,0,5);
			sprintf((char *)FTP_ServerInfo.path2, "NULL");
		}

		if(FTP_ServerInfo.file1 == NULL)
		{
			FTP_ServerInfo.file1 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.file1,0,5);
			sprintf((char *)FTP_ServerInfo.file1, "NULL");
		}

		if(FTP_ServerInfo.file2 == NULL)
		{
			FTP_ServerInfo.file2 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.file2,0,5);
			sprintf((char *)FTP_ServerInfo.file2, "NULL");
		}

		if(FTP_ServerInfo.file3 == NULL)
		{
			FTP_ServerInfo.file3 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.file3,0,5);
			sprintf((char *)FTP_ServerInfo.file3, "NULL");
		}

		if(FTP_ServerInfo.file4 == NULL)
		{
			FTP_ServerInfo.file4 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.file4,0,5);
			sprintf((char *)FTP_ServerInfo.file4, "NULL");
		}

		if(FTP_ServerInfo.file5 == NULL)
		{
			FTP_ServerInfo.file5 = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_ServerInfo.file5,0,5);
			sprintf((char *)FTP_ServerInfo.file5, "NULL");
		}
	}

	return ret;
}

//FTP固件信息
u8 ReadFTP_FrameWareInfo(void)
{
	u8 ret = 0;
	u8 buf[E_FTP_FW_INFO_LEN];
	u8 temp_buf[64];

	memset(buf,0,E_FTP_FW_INFO_LEN);

	ret = ReadDataFromEepromToMemory(buf,E_FTP_FW_INFO_ADD,E_FTP_FW_INFO_LEN);

	if(ret == 1)
	{
		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 0,30);
		GetMemoryForSpecifyPointer(&FTP_FrameWareInfo.name,strlen((char *)temp_buf), temp_buf);

		memset(temp_buf,0,64);
		memcpy(temp_buf,buf + 30,8);
		GetMemoryForSpecifyPointer(&FTP_FrameWareInfo.version,strlen((char *)temp_buf), temp_buf);

		FTP_FrameWareInfo.length = (((u32)(*(buf + 41))) << 24) +
								   (((u32)(*(buf + 40))) << 16) +
								   (((u32)(*(buf + 39))) << 8) +
								   (((u32)(*(buf + 38))) << 0);
	}
	else
	{
		if(FTP_FrameWareInfo.name == NULL)
		{
			FTP_FrameWareInfo.name = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_FrameWareInfo.name,0,5);
			sprintf((char *)FTP_FrameWareInfo.name, "NULL");
		}

		if(FTP_FrameWareInfo.version == NULL)
		{
			FTP_FrameWareInfo.version = (u8 *)mymalloc(sizeof(u8) * 5);
			memset(FTP_FrameWareInfo.version,0,5);
			sprintf((char *)FTP_FrameWareInfo.version, "NULL");
		}

		FTP_FrameWareInfo.length = 0;
	}

	return ret;
}

//将固件升级状态写入到EEPROM
void WriteFrameWareStateToEeprom(void)
{
	u8 temp_buf[20];
	
	temp_buf[0]  = FrameWareState.state;
	temp_buf[1]  = (u8)(FrameWareState.total_bags >> 8);
	temp_buf[2]  = (u8)FrameWareState.total_bags;
	temp_buf[3]  = (u8)(FrameWareState.current_bag_cnt >> 8);
	temp_buf[4]  = (u8)FrameWareState.current_bag_cnt;
	temp_buf[5]  = (u8)(FrameWareState.bag_size >> 8);
	temp_buf[6]  = (u8)FrameWareState.bag_size;
	temp_buf[7]  = (u8)(FrameWareState.last_bag_size >> 8);
	temp_buf[8]  = (u8)FrameWareState.last_bag_size;
	temp_buf[9]  = (u8)(FrameWareState.total_size >> 24);
	temp_buf[10] = (u8)(FrameWareState.total_size >> 16);
	temp_buf[11] = (u8)(FrameWareState.total_size >> 8);
	temp_buf[12] = (u8)FrameWareState.total_size;
	
	WriteDataFromMemoryToEeprom(temp_buf,E_FW_UPDATE_STATE_ADD,E_FW_UPDATE_STATE_LEN - 2);
}

//读取固件设计状态
u8 ReadFrameWareState(void)
{
	u8 ret = 0;
	u16 page_num = 0;
	u16 i = 0;
	u8 buf[E_FW_UPDATE_STATE_LEN];

	memset(buf,0,E_FW_UPDATE_STATE_LEN);

	ret = ReadDataFromEepromToMemory(buf,E_FW_UPDATE_STATE_ADD,E_FW_UPDATE_STATE_LEN);

	if(ret == 1)
	{
		FrameWareState.state 			= *(buf + 0);
		FrameWareState.total_bags 		= ((((u16)(*(buf + 1))) << 8) & 0xFF00) + 
		                                  (((u16)(*(buf + 2))) & 0x00FF);
		FrameWareState.current_bag_cnt 	= ((((u16)(*(buf + 3))) << 8) & 0xFF00) + 
		                                  (((u16)(*(buf + 4))) & 0x00FF);
		FrameWareState.bag_size 		= ((((u16)(*(buf + 5))) << 8) & 0xFF00) + 
		                                  (((u16)(*(buf + 6))) & 0x00FF);
		FrameWareState.last_bag_size 	= ((((u16)(*(buf + 7))) << 8) & 0xFF00) + 
		                                  (((u16)(*(buf + 8))) & 0x00FF);

		FrameWareState.total_size 		= ((((u32)(*(buf + 9))) << 24) & 0xFF000000) +
								          ((((u32)(*(buf + 10))) << 16) & 0x00FF0000) +
								          ((((u32)(*(buf + 11))) << 8) & 0x0000FF00) +
								          ((((u32)(*(buf + 12))) << 0) & 0x000000FF);
		
		ret = 1;
	}
	else
	{
		RESET_STATE:
		FrameWareState.state 			= FIRMWARE_FREE;
		FrameWareState.total_bags 		= 0;
		FrameWareState.current_bag_cnt 	= 0;
		FrameWareState.bag_size 		= 0;
		FrameWareState.last_bag_size 	= 0;

		FrameWareState.total_size 		= 0;
		
		WriteFrameWareStateToEeprom();			//将默认值写入EEPROM
	}
	
	if(FrameWareState.state == FIRMWARE_DOWNLOADING ||
	   FrameWareState.state == FIRMWARE_DOWNLOAD_WAIT)
	{
		page_num = (FIRMWARE_MAX_FLASH_ADD - FIRMWARE_BUCKUP_FLASH_BASE_ADD) / 2048;	//得到备份区的扇区总数
						
		FLASH_Unlock();						//解锁FLASH
		
		for(i = 0; i < page_num; i ++)
		{
			FLASH_ErasePage(i * 2048 + FIRMWARE_BUCKUP_FLASH_BASE_ADD);	//擦除当前FLASH扇区
		}
		
		FLASH_Lock();						//上锁
	}
	
	if(FrameWareState.state == FIRMWARE_UPDATE_SUCCESS)
	{
		UpdateSoftWareVer();
		
		//添加升级成功事件记录
		
		goto RESET_STATE;
	}
	
	return ret;
}

//终端软件版本号
u8 UpdateSoftWareVer(void)
{
	u8 ret = 0;
	
	if(FTP_FrameWareInfo.version != NULL)
	{	
		if(search_str(DeviceInfo.software_ver, FTP_FrameWareInfo.version) == -1)
		{
			memcpy(DeviceInfo.software_ver,FTP_FrameWareInfo.version,8);
			
			WriteDataFromMemoryToEeprom(DeviceInfo.software_ver,SW_VERSION_ADD,SW_VERSION_LEN - 2);
			
			ret = 1;
		}
	}
	
	return ret;
}

//厂商代号
u8 ReadFactoryCode(void)
{
	u8 ret = 0;
	u8 buf[FACTORY_CODE_LEN];

	memset(buf,0,FACTORY_CODE_LEN);

	ret = ReadDataFromEepromToMemory(buf,FACTORY_CODE_ADD,FACTORY_CODE_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.factory_code,buf,FACTORY_CODE_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.factory_code, "0011");
	}

	return ret;
}

//厂商设备编号
u8 ReadFactoryDeviceId(void)
{
	u8 ret = 0;
	u8 buf[FACTORY_DEV_ID_LEN];

	memset(buf,0,FACTORY_DEV_ID_LEN);

	ret = ReadDataFromEepromToMemory(buf,FACTORY_DEV_ID_ADD,FACTORY_DEV_ID_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.factory_dev_id,buf,FACTORY_DEV_ID_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.factory_dev_id, "00000011");
	}

	return ret;
}

//终端软件版本号
u8 ReadSoftWareVer(void)
{
	u8 ret = 0;
	u8 buf[SW_VERSION_LEN];

	memset(buf,0,SW_VERSION_LEN);

	ret = ReadDataFromEepromToMemory(buf,SW_VERSION_ADD,SW_VERSION_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.software_ver,buf,SW_VERSION_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.software_ver, "19.06.06");
	}

	return ret;
}

//终端软件发布日期
u8 ReadSoftWareReleaseDate(void)
{
	u8 ret = 0;
	u8 buf[SW_RE_DATE_LEN];

	memset(buf,0,SW_RE_DATE_LEN);

	ret = ReadDataFromEepromToMemory(buf,SW_RE_DATE_ADD,SW_RE_DATE_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.software_release_date,buf,SW_RE_DATE_LEN - 2);
	}
	else
	{
		DeviceInfo.software_release_date[0] = 0x06;
		DeviceInfo.software_release_date[1] = 0x06;
		DeviceInfo.software_release_date[2] = 0x19;
	}

	return ret;
}

//终端通讯协议版本号
u8 ReadProtocolVer(void)
{
	u8 ret = 0;
	u8 buf[PROTOCOL_VERSION_LEN];

	memset(buf,0,PROTOCOL_VERSION_LEN);

	ret = ReadDataFromEepromToMemory(buf,PROTOCOL_VERSION_ADD,PROTOCOL_VERSION_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.protocol_ver,buf,PROTOCOL_VERSION_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.protocol_ver, "03.16.00");
	}

	return ret;
}

//终端硬件版本号
u8 ReadHardWareVer(void)
{
	u8 ret = 0;
	u8 buf[HW_VERSION_LEN];

	memset(buf,0,HW_VERSION_LEN);

	ret = ReadDataFromEepromToMemory(buf,HW_VERSION_ADD,HW_VERSION_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.hardware_ver,buf,HW_VERSION_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.hardware_ver, "19.06.06");
	}

	return ret;
}

//终端型号
u8 ReadDeviceModel(void)
{
	u8 ret = 0;
	u8 buf[DEV_MODEL_LEN];

	memset(buf,0,DEV_MODEL_LEN);

	ret = ReadDataFromEepromToMemory(buf,DEV_MODEL_ADD,DEV_MODEL_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.device_model,buf,DEV_MODEL_LEN - 2);
	}
	else
	{
		sprintf((char *)DeviceInfo.device_model, "19.06.06");
	}

	return ret;
}

//终端硬件发布日期
u8 ReadHardWareReleaseDate(void)
{
	u8 ret = 0;
	u8 buf[HW_RE_DATE_LEN];

	memset(buf,0,HW_RE_DATE_LEN);

	ret = ReadDataFromEepromToMemory(buf,HW_RE_DATE_ADD,HW_RE_DATE_LEN);

	if(ret == 1)
	{
		memcpy(DeviceInfo.hardware_release_date,buf,HW_RE_DATE_LEN - 2);
	}
	else
	{
		DeviceInfo.hardware_release_date[0] = 0x06;
		DeviceInfo.hardware_release_date[1] = 0x06;
		DeviceInfo.hardware_release_date[2] = 0x19;
	}

	return ret;
}

//终端硬件发布日期
u8 ReadLamosNumSupport(void)
{
	u8 ret = 0;
	u8 buf[HW_RE_DATE_LEN];

	memset(buf,0,SUP_LAMPS_NUM_LEN);

	ret = ReadDataFromEepromToMemory(buf,SUP_LAMPS_NUM_ADD,SUP_LAMPS_NUM_LEN);

	if(ret == 1)
	{
		DeviceInfo.lamps_num_support = buf[0];
	}
	else
	{
		DeviceInfo.lamps_num_support = 1;
	}

	return ret;
}

//事件记录表
u8 ReadEventRecordList(void)
{
	u8 ret = 0;
	u8 buf[EC1_LABLE_LEN];

	memset(buf,0,EC1_LABLE_LEN);
	
	ret = ReadDataFromEepromToMemory(buf,E_IMPORTANT_FLAG_ADD,E_IMPORTANT_FLAG_LEN);

	if(ret == 1)
	{
		EventRecordList.important_event_flag = buf[0];
	}
	else
	{
		EventRecordList.important_event_flag = 0;
	}

	ret = ReadDataFromEepromToMemory(buf,EC1_ADD,EC1_LEN);

	if(ret == 1)
	{
		EventRecordList.ec1 = buf[0];
	}
	else
	{
		EventRecordList.ec1 = 0;
		
		EventRecordList.important_event_flag = 0;

		WriteDataFromMemoryToEeprom(&EventRecordList.important_event_flag,E_IMPORTANT_FLAG_ADD, 1);
		
		WriteDataFromMemoryToEeprom(&EventRecordList.ec1,EC1_ADD, 1);

		WriteDataFromMemoryToEeprom(EventRecordList.lable1,EC1_LABLE_ADD, EC1_LABLE_LEN - 2);
	}

	ret = ReadDataFromEepromToMemory(buf,EC2_ADD,EC2_LEN);

	if(ret == 1)
	{
		EventRecordList.ec2 = buf[0];
	}
	else
	{
		EventRecordList.ec2 = 0;
		
		EventRecordList.important_event_flag = 0;

		WriteDataFromMemoryToEeprom(&EventRecordList.important_event_flag,E_IMPORTANT_FLAG_ADD, 1);

		WriteDataFromMemoryToEeprom(&EventRecordList.ec2,EC2_ADD, 1);

		WriteDataFromMemoryToEeprom(EventRecordList.lable2,EC2_LABLE_ADD, EC2_LABLE_LEN - 2);
	}

	ret = ReadDataFromEepromToMemory(buf,EC1_LABLE_ADD,EC1_LABLE_LEN);

	if(ret == 1)
	{
		memcpy(EventRecordList.lable1,buf,EC1_LABLE_LEN - 2);
	}
	else
	{
		EventRecordList.important_event_flag = 0;

		memset(EventRecordList.lable1,0,EC1_LABLE_LEN - 2);
		
		WriteDataFromMemoryToEeprom(&EventRecordList.important_event_flag,E_IMPORTANT_FLAG_ADD, 1);

		WriteDataFromMemoryToEeprom(EventRecordList.lable1,EC1_LABLE_ADD, EC1_LABLE_LEN - 2);

		WriteDataFromMemoryToEeprom(&EventRecordList.ec1,EC1_ADD, 1);
	}

	ret = ReadDataFromEepromToMemory(buf,EC2_LABLE_ADD,EC2_LABLE_LEN);

	if(ret == 1)
	{
		memcpy(EventRecordList.lable2,buf,EC2_LABLE_LEN - 2);
	}
	else
	{
		EventRecordList.important_event_flag = 0;
		
		WriteDataFromMemoryToEeprom(&EventRecordList.important_event_flag,E_IMPORTANT_FLAG_ADD, 1);
		
		memset(EventRecordList.lable2,0,EC2_LABLE_LEN - 2);

		WriteDataFromMemoryToEeprom(EventRecordList.lable2,EC2_LABLE_ADD, EC2_LABLE_LEN - 2);

		WriteDataFromMemoryToEeprom(&EventRecordList.ec2,EC2_ADD, 1);
	}

	return ret;
}

//查询预约控制是否有效
u8 CheckAppointmentControlValid(void)
{
	u8 ret = 0;
	
	struct tm tm_time;
	time_t s_seconds = 0;	//起始秒计数 2000年开始
	time_t e_seconds = 0;	//结束秒计数 2000年开始
	time_t n_seconds = 0;	//当前秒计数 2000年开始
	
	tm_time.tm_year = (AppointmentControl.start_date[5] >> 4) * 10 + (AppointmentControl.start_date[5] & 0x0F) + 2000 - 1900;	//年
	tm_time.tm_mon 	= ((AppointmentControl.start_date[4] >> 4) & 0x01) * 10 + (AppointmentControl.start_date[4] & 0x0F) - 1;	//月
	tm_time.tm_mday = (AppointmentControl.start_date[3] >> 4) * 10 + (AppointmentControl.start_date[3] & 0x0F);					//日
	tm_time.tm_hour = (AppointmentControl.start_date[2] >> 4) * 10 + (AppointmentControl.start_date[2] & 0x0F);					//时
	tm_time.tm_min 	= (AppointmentControl.start_date[1] >> 4) * 10 + (AppointmentControl.start_date[1] & 0x0F);					//分
	tm_time.tm_sec 	= (AppointmentControl.start_date[0] >> 4) * 10 + (AppointmentControl.start_date[0] & 0x0F);					//秒
	
	s_seconds = mktime(&tm_time);
	
	tm_time.tm_year = (AppointmentControl.end_date[5] >> 4) * 10 + (AppointmentControl.end_date[5] & 0x0F) + 2000 - 1900;	//年
	tm_time.tm_mon 	= ((AppointmentControl.end_date[4] >> 4) & 0x01) * 10 + (AppointmentControl.end_date[4] & 0x0F) - 1;	//月
	tm_time.tm_mday = (AppointmentControl.end_date[3] >> 4) * 10 + (AppointmentControl.end_date[3] & 0x0F);					//日
	tm_time.tm_hour = (AppointmentControl.end_date[2] >> 4) * 10 + (AppointmentControl.end_date[2] & 0x0F);					//时
	tm_time.tm_min 	= (AppointmentControl.end_date[1] >> 4) * 10 + (AppointmentControl.end_date[1] & 0x0F);					//分
	tm_time.tm_sec 	= (AppointmentControl.end_date[0] >> 4) * 10 + (AppointmentControl.end_date[0] & 0x0F);					//秒
	
	e_seconds = mktime(&tm_time);
	
	if(s_seconds >= e_seconds)	//起始时间等于或晚于结束时间
	{
		return 0;
	}
	
	tm_time.tm_year = calendar.w_year - 1900;	//年
	tm_time.tm_mon 	= calendar.w_month - 1;		//月
	tm_time.tm_mday = calendar.w_date;			//日
	tm_time.tm_hour = calendar.hour;			//时
	tm_time.tm_min 	= calendar.min;				//分
	tm_time.tm_sec 	= calendar.sec;				//秒
	
	n_seconds = mktime(&tm_time);
	
	if(s_seconds <= n_seconds && n_seconds <= e_seconds)	//当前时间大于等于起始时间并且小于等于结束日时间
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	
	return ret;
}

//查找和1号灯相匹配的节能运行模式
//返回值为系统中的第n个节能模式
u8 LookUpMatchedEnergySivingMode(u8 app_ctl)
{
	u8 ret = 0xFF;
	u8 i = 0;
	u8 match_num = 0;
	u8 match_state = 0;
	
	if(app_ctl == 1)								//预约控制有效
	{
		if(AppointmentControl.lamps_num != 0)		//预约控制灯数
		{
			if(LampsParameters.parameters[0].lamps_id == 
			   AppointmentControl.run_mode[0].lamps_id)	//灯具编号匹配成功
			{
				for(i = 0; i < MAX_ENERGY_SAVING_MODE_NUM; i ++)
				{
					if(EnergySavingMode[i].mode_id == AppointmentControl.run_mode[0].energy_saving_mode_id)		//节能运行模式匹配成功
					{
						match_num = i;					//获取到的节能运行模式
						match_state = 1;				//匹配成功标志
						
						i = MAX_ENERGY_SAVING_MODE_NUM;	//用于跳出for循环
					}
				}
				
				if(match_state == 1)					//节能运行模式匹配成功
				{
					if(EnergySavingMode[match_num].control_times >= 1)	//控制次数有效
					{
						ret = match_num;
					}
				}
			}
		}
	}
	else
	{
		if(LampsParameters.num >= 1)		//已设置基本参数的灯数
		{
			if(LampsRunMode.num >= 1)		//已设置运行模式的灯数
			{
				if(LampsParameters.parameters[0].lamps_id == 
					LampsRunMode.run_mode[0].lamps_id)		//灯具编号匹配成功
				{
					for(i = 0; i < MAX_ENERGY_SAVING_MODE_NUM; i ++)
					{
						if(EnergySavingMode[i].mode_id == LampsRunMode.run_mode[0].energy_saving_mode_id)		//节能运行模式匹配成功
						{
							match_num = i;					//获取到的节能运行模式
							match_state = 1;				//匹配成功标志
							
							i = MAX_ENERGY_SAVING_MODE_NUM;	//用于跳出for循环
						}
					}
					
					if(match_state == 1)					//节能运行模式匹配成功
					{
						if(EnergySavingMode[match_num].control_times >= 1)	//控制次数有效
						{
							ret = match_num;
						}
					}
				}
			}
		}
	}
	
	return ret;
}

//释放策略列表内存
void StrategyListFree(pControlStrategy head)
{
    while ( head )
    {
        pControlStrategy temp;

        temp = head;
        head = head->next;
        myfree( temp );
    }
}

//mode=1复位绝对策略执行状态,每天0点0分0秒复位或开关状态改变后
//mode=2复位相对策略执行状态,开关状态改变后
//mode=0复位所有策略执行状态
void StrategyListStateReset(pControlStrategy head,u8 mode)
{
    while ( head )
    {
        pControlStrategy temp;

        temp = head;
		
		if(mode == 1)
		{
			if(temp->mode == 0x01)			//绝对时间控制方式的策略需要复位
			{
				temp->state = WAIT_EXECUTE;	//复位为等待执行状态
			}
		}
		else if(mode == 2)
		{
			if(temp->mode == 0x00)			//绝对时间控制方式的策略需要复位
			{
				temp->state = WAIT_EXECUTE;	//复位为等待执行状态
			}
		}
		else if(mode == 0)
		{
			temp->state = WAIT_EXECUTE;	//复位为等待执行状态
		}
		
        head = head->next;
    }
}

//获取控制策略(在节能运行模式中获取)
u8 UpdateControlStrategyList(u8 app_ctl)
{
	u8 ret = 0;
	u8 i = 0;
	u8 num = 0;
	pControlStrategy strategy = NULL;
	pControlStrategy temp = NULL;
	
	if(xSchedulerRunning == 1)
	{
		xSemaphoreTake(xMutex_STRATEGY, portMAX_DELAY);
	}

	StrategyListFree(ControlStrategy);		//释放策略列表
	ControlStrategy = NULL;

	num = LookUpMatchedEnergySivingMode(app_ctl);	//获取节能模式

	if(num < MAX_ENERGY_SAVING_MODE_NUM)			//获取节能模式成功
	{
		ControlStrategy = (pControlStrategy)mymalloc(sizeof(pControlStrategy));
		
		ControlStrategy->prev 	= NULL;
		ControlStrategy->next 	= NULL;
		ControlStrategy->state 	= WAIT_EXECUTE;
		
		for(i = 0; i < EnergySavingMode[num].control_times; i ++)
		{
			strategy = (pControlStrategy)mymalloc(sizeof(pControlStrategy));
			
			strategy->prev  = NULL;
			strategy->next  = NULL;
			strategy->state = WAIT_EXECUTE;
			
			strategy->mode = EnergySavingMode[num].operation[i].mode;
			strategy->type = EnergySavingMode[num].operation[i].control_type;
			strategy->brightness = EnergySavingMode[num].operation[i].brightness;

			strategy->year   = (EnergySavingMode[num].operation[i].absolute_time[5] >> 4) * 10 +
			                   (EnergySavingMode[num].operation[i].absolute_time[5] & 0x0f);
			strategy->month  = ((EnergySavingMode[num].operation[i].absolute_time[4] >> 4) & 0x01) * 10 + 
			                    (EnergySavingMode[num].operation[i].absolute_time[4] & 0x0f);
			strategy->date   = (EnergySavingMode[num].operation[i].absolute_time[3] >> 4) * 10 +
			                   (EnergySavingMode[num].operation[i].absolute_time[3] & 0x0f);
			strategy->hour   = (EnergySavingMode[num].operation[i].absolute_time[2] >> 4) * 10 +
			                   (EnergySavingMode[num].operation[i].absolute_time[2] & 0x0f);
			strategy->minute = (EnergySavingMode[num].operation[i].absolute_time[1] >> 4) * 10 +
			                   (EnergySavingMode[num].operation[i].absolute_time[1] & 0x0f);
			strategy->second = (EnergySavingMode[num].operation[i].absolute_time[0] >> 4) * 10 +
			                   (EnergySavingMode[num].operation[i].absolute_time[0] & 0x0f);

			strategy->time_re = EnergySavingMode[num].operation[i].relative_time * 5 * 60;
			
			if(ControlStrategy != NULL)
			{
				for(temp = ControlStrategy; temp != NULL; temp = temp->next)
				{
					if(temp->next == NULL)
					{
						temp->next = strategy;
						temp->next->prev = temp;
						
						ret = 1;
						
						break;
					}
				}
			}
		}
	}
	
	if(xSchedulerRunning == 1)
	{
		xSemaphoreGive(xMutex_STRATEGY);
	}
	
	return ret;
}

//恢复出厂设置
//mode=3时恢复网络设置
void RestoreFactorySettings(u8 mode)
{
	u8 i = 0;
	
	if(mode == 3)
	{
		AT24CXX_WriteLenByte(SERVER_INFO_ADD + SERVER_INFO_LEN - 2,0xFFFF,2);			//恢复主站IP和端口配置
	}
	
	AT24CXX_WriteLenByte(UP_COMM_PARA_ADD + UP_COMM_PARA_LEN - 2,0xFFFF,2);				//恢复上行通信口通信参数
	AT24CXX_WriteLenByte(ER_OTHER_CONF_ADD + ER_OTHER_CONF_LEN - 2,0xFFFF,2);			//恢复终端事件记录配置参数
	AT24CXX_WriteLenByte(DEV_BASIC_INFO_ADD + DEV_BASIC_INFO_LEN - 2,0xFFFF,2);			//恢复设备基本信息
	AT24CXX_WriteLenByte(ER_TIME_CONF_ADD + ER_TIME_CONF_LEN - 2,0xFFFF,2);				//恢复终端事件检测时间参数
	AT24CXX_WriteLenByte(ER_THRE_CONF_ADD + ER_THRE_CONF_LEN - 2,0xFFFF,2);				//恢复终端事件检测阈值参数
	AT24CXX_WriteLenByte(RSA_ENCRY_PARA_ADD + RSA_ENCRY_PARA_LEN - 2,0xFFFF,2);			//恢复RSA加密参数
	AT24CXX_WriteLenByte(SWITCH_DATE_DAYS_ADD + SWITCH_DATE_DAYS_LEN - 2,0xFFFF,2);		//恢复控制器开关灯时间数据
	AT24CXX_WriteLenByte(LAMPS_NUM_PARA_ADD + LAMPS_NUM_PARA_LEN - 2,0xFFFF,2);			//恢复灯具基本信息参数
	AT24CXX_WriteLenByte(LAMPS_NUM_MODE_ADD + LAMPS_NUM_MODE_LEN - 2,0xFFFF,2);			//恢复单灯运行模式参数
	
	for(i = 0; i < MAX_ENERGY_SAVING_MODE_NUM; i ++)
	{
		AT24CXX_WriteOneByte(E_SAVE_MODE_LABLE_ADD + i * E_SAVE_MODE_LEN,0);			//恢复单灯节能模式
	}
	
	AT24CXX_WriteLenByte(APPOIN_LABLE_ADD + APPOIN_LABLE_LEN - 2,0xFFFF,2);				//恢复单灯预约控制数据
	AT24CXX_WriteLenByte(ILLUM_THRE_ADD + ILLUM_THRE_LEN - 2,0xFFFF,2);					//恢复单灯照度开关阈值
	AT24CXX_WriteLenByte(SWITCH_MODE_ADD + SWITCH_MODE_LEN - 2,0xFFFF,2);				//恢复单灯开关模式选择
	AT24CXX_WriteLenByte(EC1_ADD + EC1_LEN - 2,0xFFFF,2);								//恢复重要事件计数器
	AT24CXX_WriteLenByte(EC2_ADD + EC2_LEN - 2,0xFFFF,2);								//恢复一般事件计数器
	AT24CXX_WriteLenByte(EC1_LABLE_ADD + EC1_LABLE_LEN - 2,0xFFFF,2);					//恢复重要事件标签数组
	AT24CXX_WriteLenByte(EC2_LABLE_ADD + EC2_LABLE_LEN - 2,0xFFFF,2);					//恢复一般事件标签数组
	AT24CXX_WriteLenByte(E_IMPORTANT_FLAG_ADD + E_IMPORTANT_FLAG_LEN - 2,0xFFFF,2);		//恢复重要事件标志
	AT24CXX_WriteLenByte(E_FTP_SERVER_INFO_ADD + E_FTP_SERVER_INFO_LEN - 2,0xFFFF,2);	//恢复FTP服务器信息
	AT24CXX_WriteLenByte(E_FTP_FW_INFO_ADD + E_FTP_FW_INFO_LEN - 2,0xFFFF,2);			//恢复重要事件标志
	AT24CXX_WriteLenByte(E_FW_UPDATE_STATE_ADD + E_FW_UPDATE_STATE_LEN - 2,0xFFFF,2);	//恢复OTA状态信息
	
	ReadParametersFromEEPROM();															//恢复缓存中的数据
	
	NeedUpdateStrategyList = 1;															//重新加载策略
}

//在EEPROM中读取运行参数
void ReadParametersFromEEPROM(void)
{
	ReadUpCommPortPara();
	ReadServerInfo();
	ReadEventRecordConf();
	ReadDeviceBaseInfo();
	ReadEventDetectTimeConf();
	ReadEventDetectThreConf();
	ReadRSA_PublicKey();
	ReadLampsSwitchProject();
	ReadLampsParameters();
	ReadLampsRunMode();
	ReadEnergySavingMode();
	ReadAppointmentControl();
	ReadIlluminanceThreshold();
	ReadSwitchMode();
	ReadFTP_ServerInfo();
	ReadFTP_FrameWareInfo();
	ReadFactoryCode();
	ReadFactoryDeviceId();
	ReadSoftWareVer();
	ReadSoftWareReleaseDate();
	ReadProtocolVer();
	ReadHardWareVer();
	ReadDeviceModel();
	ReadHardWareReleaseDate();
	ReadLamosNumSupport();
	ReadEventRecordList();
	ReadFrameWareState();
	
//	UpdateControlStrategyList();

	ResetRemoteCurrentControl();

	ResetFrameStruct(1,
	                 &control_msg_in,
	                 &user_data_sign_in,
	                 &user_data_in);
	ResetFrameStruct(1,
	                 &control_msg_out,
	                 &user_data_sign_out,
	                 &user_data_out);
}











