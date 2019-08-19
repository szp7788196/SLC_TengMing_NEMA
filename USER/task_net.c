#include "task_net.h"
#include "common.h"
#include "delay.h"
#include "net_protocol.h"
#include "rtc.h"
#include "UART4.h"


TaskHandle_t xHandleTaskNET = NULL;
SensorMsg_S *p_tSensorMsgNet = NULL;			//用于装在传感器数据的结构体变量
unsigned portBASE_TYPE NET_Satck;

u32 LoginStaggeredPeakInterval = 0;				//登录信息错峰后周期
u32 UploadDataStaggeredPeakInterval = 0;		//数据上报错峰后周期
u32 HeartBeatStaggeredPeakInterval = 0;			//心跳上报错峰后周期

CONNECT_STATE_E ConnectState = UNKNOW_STATE;	//连接状态

nbiot_device_t *dev = NULL;

nbiot_value_t LinkLayerDownPacketCarrier;			//数据链路层数据包载体 hexstring  objectID:3200 instanceID:0 resourceID:5501 (下行数据)
nbiot_value_t LinkLayerUpPacketCarrier;				//数据链路层数据包载体 hexstring  objectID:3200 instanceID:0 resourceID:5505 (上行数据)

void write_callback( uint16_t       objid,
                     uint16_t       instid,
                     uint16_t       resid,
                     nbiot_value_t  *data )
{
#ifdef DEBUG_LOG
    printf( "write /%d/%d/%d：%d\r\n",objid,instid,resid,data->value.as_bool);
#endif

    if(objid == 3200 && instid == 0 && resid == 5501)
	{
		//协议数据包解析

	}
}

void read_callback( uint16_t       objid,
                    uint16_t       instid,
                    uint16_t       resid,
                    nbiot_value_t *data )
{
#ifdef DEBUG_LOG
	printf( "read /%d/%d/%d\r\n",objid,instid,resid );
#endif
}

void execute_callback( uint16_t       objid,
                       uint16_t       instid,
                       uint16_t       resid,
                       nbiot_value_t  *data,
                       const void     *buff,
                       size_t         size)
{
	u8 hex_buf_in[512];
	u8 hex_buf_out[512];
	char str_buf[1024];

//	u8 *hex_buf_in;
//	u8 *hex_buf_out;
//	char *str_buf;

	u16 len = 0;

//	hex_buf_in = (u8 *)mymalloc(sizeof(u8) * 1024);
//	hex_buf_out = (u8 *)mymalloc(sizeof(u8) * 1024);
//	str_buf = (char *)mymalloc(sizeof(char) * 1024);

#ifdef DEBUG_LOG
    printf( "execute /%d/%d/%d\r\n",objid,instid,resid );
#endif

	if(objid == 3200 && instid == 0 && resid == 5501)
	{
		//协议数据包解析
#ifdef DEBUG_LOG
		printf( "recv data len:%d\r\ndata:%s\r\n",size,(char *)buff);
#endif
		StrToHex(hex_buf_in, (char *)buff, size / 2);

		len = NetDataFrameHandle(hex_buf_in,size / 2,hex_buf_out);

		if(len >= 1 && len <= 512)
		{
			HexToStr((char *)str_buf, hex_buf_out,len);

			nbiot_free(LinkLayerUpPacketCarrier.value.as_buf.val);
			LinkLayerUpPacketCarrier.value.as_buf.val = nbiot_bufdup((u8 *)str_buf,len * 2);
			LinkLayerUpPacketCarrier.value.as_buf.len = len * 2;

			LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;				//数据上传标志置位
		}
	}
}

void res_update(void)
{
	static time_t times_sec = 0;
	static time_t times_sec1 = 0;
	static u16 download_time_out = 0;
	static u8  download_failed_times = 0;
	u8 buf[64];
	char str_buf[128];
	u8 afn = 0;
	u8 fn = 0;
	u16 len = 0;

	if(NeedServerConfirm != 0)			//有需要被主站确认的消息,重发上一条数据
	{	
		if(GetSysTick1s() - times_sec1 >= UpCommPortPara.wait_slave_rsp_timeout)
		{
			times_sec1 = GetSysTick1s();
			
			NeedServerConfirm --;
			
			LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;				//数据上传标志置位
		}

		return;
	}
	else if(LogInOutState == 0x00)		//未登录，发送登陆数据包
	{
		if(times_sec == 0)
		{
			times_sec = GetSysTick1s();
		}

		if(GetSysTick1s() - times_sec >= LoginStaggeredPeakInterval)
		{
			times_sec = GetSysTick1s();

			if(UpCommPortPara.random_peak_staggering_time != 0)		//使用随机错峰时间
			{
				LoginStaggeredPeakInterval = rand() % UpCommPortPara.random_peak_staggering_time;
			}
			else													//使用固定错峰时间
			{
				LoginStaggeredPeakInterval = UpCommPortPara.specify_peak_staggering_time;
			}

			afn = 0x02;
			fn = 1;

			LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;					//数据上传标志置位
		}
	}
	else if(GetSysTick1s() - times_sec >= HeartBeatStaggeredPeakInterval)
	{
		times_sec = GetSysTick1s();

		if(UpCommPortPara.random_peak_staggering_time != 0)		//使用随机错峰时间
		{
			HeartBeatStaggeredPeakInterval = UpCommPortPara.heart_beat_cycle * 60 + rand() % UpCommPortPara.random_peak_staggering_time;
		}
		else													//使用固定错峰时间
		{
			HeartBeatStaggeredPeakInterval = UpCommPortPara.heart_beat_cycle * 60 + UpCommPortPara.specify_peak_staggering_time;
		}

		afn = 0x02;
		fn = 3;

		LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;					//数据上传标志置位
	}
	else if(EventRecordList.important_event_flag != 0 ||
		    EventRecordList.normal_event_flag != 0)
	{
		times_sec1 = GetSysTick1s();
		
		afn = 0x0E;
		fn = 2;
		
		LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;					//数据上传标志置位
	}
	else if(FrameWareState.state == FIRMWARE_DOWNLOADING)
	{
		times_sec = GetSysTick1s();

		download_time_out = 0;											//固件包等待超时
		download_failed_times = 0;										//固件包下载失败次数
		FrameWareState.state = FIRMWARE_DOWNLOAD_WAIT;					//等待当前固件包

		afn = 0x10;
		fn = 13;

		LinkLayerUpPacketCarrier.flag |= NBIOT_UPDATED;					//数据上传标志置位
	}
	else if(FrameWareState.state == FIRMWARE_DOWNLOAD_WAIT)
	{
		if((download_time_out ++) >= 250)
		{
			if((download_failed_times ++) >= 15)
			{
				FrameWareState.state = FIRMWARE_DOWNLOAD_FAILED;	//判定为固件下载失败
			}
			else
			{
				FrameWareState.state = FIRMWARE_DOWNLOADING;		//重新下载当前固件包
			}
		}
	}

	if((LinkLayerUpPacketCarrier.flag & NBIOT_UPDATED) != (u32)0x00)	//有数据需要发送
	{
		len = MakeLogin_out_heartbeatFrame(afn,fn,buf);
	}

	if(len == 0)
	{
		LinkLayerUpPacketCarrier.flag &= ~NBIOT_UPDATED;
	}
	else
	{
		HexToStr((char *)str_buf, buf,len);

		nbiot_free(LinkLayerUpPacketCarrier.value.as_buf.val);
		LinkLayerUpPacketCarrier.value.as_buf.val = nbiot_bufdup((u8 *)str_buf,len * 2);
		LinkLayerUpPacketCarrier.value.as_buf.len = len * 2;
	}
}

int create_device(void)
{
	int ret = 0;
	int life_time = 1000;

	ret = nbiot_device_create( &dev,
							   life_time,
							   write_callback,
							   read_callback,
							   execute_callback );

	if ( ret )
	{
		nbiot_device_destroy( dev );
#ifdef DEBUG_LOG
		printf( "device create failed, code = %d.\r\n", ret );
#endif
	}

	return ret;
}

int add_object_resource(void)
{
	int ret = 0;

	LinkLayerDownPacketCarrier.type = NBIOT_HEX_STRING;
	LinkLayerDownPacketCarrier.flag = NBIOT_READABLE|NBIOT_WRITABLE|NBIOT_EXECUTABLE;
	ret = nbiot_resource_add(dev,3200,0,5501,&LinkLayerDownPacketCarrier);
	if(ret)
	{
		nbiot_device_destroy(dev);
#ifdef DEBUG_LOG
		printf("device add resource(LinkLayerDownPacketCarrier) failed, code = %d.\r\n", ret);
#endif
	}

	LinkLayerUpPacketCarrier.type = NBIOT_HEX_STRING;
	LinkLayerUpPacketCarrier.flag = NBIOT_READABLE|NBIOT_WRITABLE|NBIOT_EXECUTABLE;
	ret = nbiot_resource_add(dev,3200,0,5505,&LinkLayerUpPacketCarrier);
	if(ret)
	{
		nbiot_device_destroy(dev);
#ifdef DEBUG_LOG
		printf("device add resource(LinkLayerUpPacketCarrier) failed, code = %d.\r\n", ret);
#endif
	}

	nbiot_object_add(dev);

	return ret;
}

void unregister_all_things(void)
{
	nbiot_device_close(dev,0);
	nbiot_device_destroy(dev);
	nbiot_clear_environment();

	UART4_Init(9600);
}

//从指定的NTP服务器获取时间
u8 SyncDataTimeFormBcxxModule(time_t sync_cycle)
{
	u8 ret = 0;
	struct tm tm_time;
	static time_t time_c = 0;
	static time_t time_s = 0;
	char buf[32];

	if((GetSysTick1s() - time_c >= sync_cycle) || GetTimeOK != 1)
	{
		time_c = GetSysTick1s();

		memset(buf,0,32);

		if(m53xx_get_AT_CCLK(buf))
		{
			tm_time.tm_year = 2000 + (buf[0] - 0x30) * 10 + buf[1] - 0x30 - 1900;
			tm_time.tm_mon = (buf[3] - 0x30) * 10 + buf[4] - 0x30 - 1;
			tm_time.tm_mday = (buf[6] - 0x30) * 10 + buf[7] - 0x30;

			tm_time.tm_hour = (buf[9] - 0x30) * 10 + buf[10] - 0x30;
			tm_time.tm_min = (buf[12] - 0x30) * 10 + buf[13] - 0x30;
			tm_time.tm_sec = (buf[15] - 0x30) * 10 + buf[16] - 0x30;

			time_s = mktime(&tm_time);

			time_s += 28800;

			SyncTimeFromNet(time_s);

			GetTimeOK = 1;

			ret = 1;
		}
	}

	return ret;
}

void vTaskNET(void *pvParameters)
{
	int ret = 0;
	time_t time_s = 0;
	time_t sync_csq_time = nbiot_time();

	GetTimeOK = GetRTC_State();		//获取RTC实时时钟状态

	p_tSensorMsgNet = (SensorMsg_S *)mymalloc(sizeof(SensorMsg_S));

	if(UpCommPortPara.random_peak_staggering_time != 0)		//使用随机错峰时间
	{
		LoginStaggeredPeakInterval 		= rand() % UpCommPortPara.random_peak_staggering_time;
		HeartBeatStaggeredPeakInterval 	= UpCommPortPara.heart_beat_cycle * 60 	+ rand() % UpCommPortPara.random_peak_staggering_time;
	}
	else													//使用固定错峰时间
	{
		LoginStaggeredPeakInterval 		= UpCommPortPara.specify_peak_staggering_time;
		HeartBeatStaggeredPeakInterval 	= UpCommPortPara.heart_beat_cycle * 60 	+ UpCommPortPara.specify_peak_staggering_time;
	}

	RE_INIT_M53XX:
	nbiot_init_environment();

	ret = create_device();					//创建设备

	if(ret)
	{
		unregister_all_things();

		goto RE_INIT_M53XX;
	}

	ret = add_object_resource();			//添加资源

	if(ret)
	{
		unregister_all_things();

		goto RE_INIT_M53XX;
	}

	ret = nbiot_device_connect(dev,100);	//连接到OneNet

	if(ret)
	{
#ifdef DEBUG_LOG
		printf( "connect OneNET failed.\r\n" );
#endif

		unregister_all_things();

		goto RE_INIT_M53XX;
	}
	else
	{
#ifdef DEBUG_LOG
		 printf( "connect OneNET success.\r\n" );
#endif
	}
	
	time_s = GetSysTick1s();

	while(1)
	{
		if(dev->observes == NULL)
		{
			if(GetSysTick1s() - time_s >= 180)	//设备离线约3分钟内不回复则重启M5310-A模组
			{
				goto RE_INIT_M53XX;
			}
		}

		if(dev->observes != NULL)
		{
			if(nbiot_time() - sync_csq_time >= 30)
			{
				sync_csq_time = nbiot_time();

				m53xx_get_AT_CSQ(&BcxxCsq);		//获取通讯状态质量

				bcxx_get_AT_NUESTATS(&BcxxRsrp,
                                     &BcxxRssi,
                                     &BcxxSnr,
									 &BcxxPci,
                                     &BcxxRsrq);

				SyncDataTimeFormBcxxModule(3600);			//每隔3600秒自动对时
			}
		}

		ret = nbiot_device_step(dev, -1);					//轮训查看并处理LwM2M事件

		if ( ret )
		{
#ifdef DEBUG_LOG
			printf( "device step error, code = %d.\r\n", ret );
#endif
			unregister_all_things();

			goto RE_INIT_M53XX;
		}
		else
		{
			if(dev->observes != NULL)
			{
				res_update();	//向服务器发送数据包
			}
		}

		if(ReConnectToServer == 0x81)		//上行网络重连
		{
			ReConnectToServer = 0;

			goto RE_INIT_M53XX;
		}

		delay_ms(100);
	}
}
































