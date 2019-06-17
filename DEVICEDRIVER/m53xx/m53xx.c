#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "delay.h"
#include "at_cmd.h"
#include "utils.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "m53xx.h"
#include "fifo.h"
#include "common.h"
#include "uart4.h"


char cmd_tx_buff[512];
unsigned char   bcxx_init_ok;
unsigned char	bcxx_busy = 0;
char *bcxx_imei = NULL;

u8 BcxxBand = 0;	//Ƶ��
s16 BcxxCsq = 0;	//ͨ��״̬����
s16 BcxxPci = 0;	//��վ
s16 BcxxRsrp = 0;	//reference signal received power
s16 BcxxRsrq = 0;	//reference signal received quality
s16 BcxxRssi = 0;	//received signal strength indicator
s16 BcxxSnr = 0;	//signal to noise ratio

void m53xx_hard_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	BCXX_PWREN_LOW;
	BCXX_RST_LOW;
}

void m53xx_hard_reset(void)
{
	BCXX_PWREN_LOW;						//�رյ�Դ
	delay_ms(50);						//��ʱ����5ms
	BCXX_RST_HIGH;

	delay_ms(300);

	BCXX_PWREN_HIGH;					//�򿪵�Դ
	delay_ms(10);						//��ʱ����535us
	BCXX_RST_LOW;

	bcxx_init_ok = 1;
}


u32 ip_SendData(int8_t *buf, uint32_t len)
{
     SentData((char *)buf,"OK",100);
     return len;
}

void netif_rx(uint8_t *buf,uint16_t *read)
{
	uint8_t ptr[1024] = {0};

	*read = fifo_get(dl_buf_id,ptr);

	if(*read != 0)
	{
		if(strstr((const char *)ptr, "+MIPL") != NULL)
		{
			memcpy(buf,ptr,*read);
		}
		else
		{
			*read = 0;
		}
	}
}

void netdev_init(void)
{
	RE_INIT:

	m53xx_hard_reset();

	nbiot_sleep(8000);

	if(m53xx_set_NATSPEED(115200) != 1)
		goto RE_INIT;

	if(m53xx_get_AT_CGSN() != 1)
		goto RE_INIT;

	if(m53xx_get_AT_NCCID() != 1)
		goto RE_INIT;

	if(m53xx_get_AT_CIMI() != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CFUN(0) != 1)
		goto RE_INIT;

	if(m53xx_set_AT_NBAND(DeviceInfo.imsi) != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CELL_RESELECTION() != 1)
		goto RE_INIT;

	if(m53xx_set_AT_NRB() != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CFUN(1) != 1)
		goto RE_INIT;

//	if(bcxx_set_AT_MIPLCONFIG("183.230.40.39","5683") != 1)
//		goto RE_INIT;

	if(m53xx_set_AT_CEDRXS(0) != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CPSMS(0) != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CSCON(1) != 1)
		goto RE_INIT;

	if(m53xx_set_AT_CEREG(2) != 1)
		goto RE_INIT;

//	if(m53xx_set_AT_MREGSWT(0) != 1)
//		goto RE_INIT;

//	if(bcxx_set_AT_NSONMI(3) != 1)
//		goto RE_INIT;

	if(m53xx_set_AT_CGATT(1) != 1)
		goto RE_INIT;

	nbiot_sleep(10000);

	if(!SendCmd("AT+CGATT?\r\n","+CGATT:1", 1000,5,TIMEOUT_5S))
	{
		goto RE_INIT;
	}

	printf("m5310-A init sucess\r\n");
}

//�趨ģ�鲨����
unsigned char m53xx_set_NATSPEED(u32 baud_rate)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NATSPEED?\r\n");

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

	if(ret == 1)				//������Ĭ��9600
	{
		memset(cmd_tx_buf,0,64);

		sprintf(cmd_tx_buf,"AT+NATSPEED=%d,30,1,2,1,0,0\r\n",baud_rate);

		ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

		if(ret == 1)
		{
			UART4_Init(baud_rate);

			memset(cmd_tx_buf,0,64);

			sprintf(cmd_tx_buf,"AT+NATSPEED?\r\n");

			ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);
		}
	}
	else if(ret == 0)
	{
		UART4_Init(baud_rate);

		memset(cmd_tx_buf,0,64);

		sprintf(cmd_tx_buf,"AT+NATSPEED?\r\n");

		ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);
	}

    return ret;
}

unsigned char m53xx_set_AT_CFUN(char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CFUN=%d\r\n", cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_90S);

    return ret;
}

//����Ƶ��
unsigned char m53xx_set_AT_NBAND(unsigned char *imsi)
{
	unsigned char ret = 0;
	unsigned char operators_code = 0;
	char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	if(imsi == NULL)
	{
		return ret;
	}

	operators_code = (*(imsi + 3) - 0x30) * 10 + *(imsi + 4) - 0x30;

	if(operators_code == 0 ||
	   operators_code == 2 ||
	   operators_code == 4 ||
	   operators_code == 7 ||
	   operators_code == 1 ||
	   operators_code == 6 ||
	   operators_code == 9)
	{
		BcxxBand = 8;
	}
	else if(operators_code == 3 ||
	        operators_code == 5 ||
	        operators_code == 11)
	{
		BcxxBand = 5;
	}
	else
	{
		BcxxBand = 8;
	}

	sprintf(cmd_tx_buf,"AT+NBAND=%d\r\n",BcxxBand);

	ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_90S);

    return ret;
}

//��ȡIMEI��
unsigned char m53xx_get_AT_CGSN(void)
{
	unsigned char ret = 0;
	char buf[32];

    if(SendCmd("AT+CGSN=1\r\n", "+CGSN", 100,0,TIMEOUT_1S) == 1)
    {
		memset(buf,0,32);

		get_str1((unsigned char *)result_ptr->data, "CGSN:", 1, "\r\n", 2, (unsigned char *)buf);

		if(strlen(buf) == 15)
		{
			memcpy(DeviceInfo.imei,buf,15);

			ret = 1;
		}
    }

    return ret;
}

//��ȡICCID
unsigned char m53xx_get_AT_NCCID(void)
{
	unsigned char ret = 0;
	char buf[32];

    if(SendCmd("AT+NCCID\r\n", "OK", 100,0,TIMEOUT_5S) == 1)
    {
		memset(buf,0,32);

		get_str1((unsigned char *)result_ptr->data, "NCCID:", 1, "\r\n", 2, (unsigned char *)buf);

		if(strlen(buf) == 20)
		{
			memcpy(DeviceInfo.iccid,buf,20);

			ret = 1;
		}
    }

    return ret;
}

//��ȡIMSI
unsigned char m53xx_get_AT_CIMI(void)
{
	unsigned char ret = 0;
	char buf[32];

    if(SendCmd("AT+CIMI\r\n", "OK", 100,0,TIMEOUT_1S) == 1)
    {
		memset(buf,0,32);

		get_str1((unsigned char *)result_ptr->data, "\r\n", 1, "\r\n", 2, (unsigned char *)buf);

		if(strlen(buf) == 15)
		{
			memcpy(DeviceInfo.imsi,buf,15);

			ret = 1;
		}
    }

    return ret;
}

//��С����ѡ����
unsigned char m53xx_set_AT_CELL_RESELECTION(void)
{
	unsigned char ret = 0;

    ret = SendCmd("AT+NCONFIG=CELL_RESELECTION,TRUE\r\n", "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//����ģ��
unsigned char m53xx_set_AT_NRB(void)
{
	unsigned char ret = 0;

	SendCmd("AT+NRB\r\n", "OK", 1000,0,TIMEOUT_10S);

	ret = SendCmd("AT\r\n", "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//����IOTƽ̨IP�Ͷ˿� HUAWEI IOT
unsigned char m53xx_set_AT_NCDP(char *addr, char *port)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NCDP=%s,%s\r\n",addr,port);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//��������״̬�Զ�����
unsigned char m53xx_set_AT_CSCON(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CSCON=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//EPS Network Registration Status
unsigned char m53xx_set_AT_CEREG(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CEREG=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//�����������ݽ���ģʽHUAWEI IOT
unsigned char m53xx_set_AT_NNMI(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NNMI=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//�����������ݽ���ģʽ TCP/IP
unsigned char m53xx_set_AT_NSONMI(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NSONMI=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

unsigned char m53xx_set_AT_CGATT(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CGATT=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_2S);

    return ret;
}

unsigned char m53xx_set_AT_MREGSWT(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+MREGSWT=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_2S);

    return ret;
}

//����eDRX����
unsigned char m53xx_set_AT_CEDRXS(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CEDRXS=%d,5\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//����PMS����
unsigned char m53xx_set_AT_CPSMS(unsigned char cmd)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+CPSMS=%d\r\n",cmd);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//��IOTƽ̨�������� HUAWEI IOT
unsigned char m53xx_set_AT_NMGS(unsigned int len,char *buf)
{
	unsigned char ret = 0;
    char cmd_tx_buf[256];

	memset(cmd_tx_buf,0,256);

	sprintf(cmd_tx_buf,"AT+NMGS=%d,%s\r\n",len,buf);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//��ȡ����IP��ַ
unsigned char m53xx_get_AT_CGPADDR(char **ip)
{
	unsigned char ret = 0;
	unsigned char len = 0;
	unsigned char new_len = 0;
	unsigned char msg[20];

    if(SendCmd("AT+CGPADDR\r\n", ",", 100,20,TIMEOUT_1S) == 1)
    {
		memset(msg,0,20);

		get_str1((unsigned char *)result_ptr->data, "+CGPADDR:0,", 1, "\r\nOK", 1, (unsigned char *)msg);

		new_len = strlen((char *)msg);

		if(new_len != 0)
		{
			if(*ip == NULL)
			{
				*ip = (char *)mymalloc(sizeof(u8) * len + 1);
			}

			if(*ip != NULL)
			{
				len = strlen((char *)*ip);

				if(len == new_len)
				{
					memset(*ip,0,new_len + 1);
					memcpy(*ip,msg,new_len);
					ret = 1;
				}
				else
				{
					myfree(*ip);
					*ip = (char *)mymalloc(sizeof(u8) * new_len + 1);
					if(ip != NULL)
					{
						memset(*ip,0,new_len + 1);
						memcpy(*ip,msg,new_len);
						len = new_len;
						new_len = 0;
						ret = 1;
					}
				}
			}
		}
    }

    return ret;
}

//�½�һ��SOCKET
unsigned char m53xx_set_AT_NSOCR(char *type, char *protocol,char *port)
{
	unsigned char ret = 255;
	char cmd_tx_buf[64];
	unsigned char buf[3] = {0,0,0};

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NSOCR=%s,%s,%s,1\r\n",type,protocol,port);

    if(SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S) == 1)
    {
		get_str1((unsigned char *)result_ptr->data, "\r\n", 1, "\r\n", 2, (unsigned char *)buf);
		if(strlen((char *)buf) == 1)
		{
			ret = buf[0] - 0x30;
		}
    }

    return ret;
}

//�ر�һ��SOCKET
unsigned char m53xx_set_AT_NSOCL(unsigned char socket)
{
	unsigned char ret = 0;
	char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NSOCL=%d\r\n",socket);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//��UDP�������������ݲ��ȴ���Ӧ����
unsigned char m53xx_set_AT_NSOFT(unsigned char socket, char *ip,char *port,unsigned int len,char *inbuf)
{
	unsigned char ret = 0;
    char cmd_tx_buf[256];

	memset(cmd_tx_buf,0,256);

	sprintf(cmd_tx_buf,"AT+NSOST=%d,%s,%s,%d,%s\r\n",socket,ip,port,len,inbuf);

    ret = SendCmd(cmd_tx_buf, "+NSOSTR:", 100,0,TIMEOUT_60S);

    return ret;
}

//����һ��TCP����
unsigned char m53xx_set_AT_NSOCO(unsigned char socket, char *ip,char *port)
{
	unsigned char ret = 0;
    char cmd_tx_buf[64];

	memset(cmd_tx_buf,0,64);

	sprintf(cmd_tx_buf,"AT+NSOCO=%d,%s,%s\r\n",socket,ip,port);

    ret = SendCmd(cmd_tx_buf, "OK", 100,0,TIMEOUT_1S);

    return ret;
}

//ͨ��TCP���ӷ�������
unsigned char m53xx_set_AT_NSOSD(unsigned char socket, unsigned int len,char *inbuf)
{
	unsigned char ret = 0;
	char cmd_tx_buf[512];

	memset(cmd_tx_buf,0,512);

	sprintf(cmd_tx_buf,"AT+NSOSD=%d,%d,%s,0x100,100\r\n",socket,len,inbuf);

    ret = SendCmd(cmd_tx_buf, "+NSOSTR:", 100,0,TIMEOUT_60S);

    return ret;
}

//��ȡ�ź�ǿ��
unsigned char m53xx_get_AT_CSQ(signed short *csq)
{
	u8 ret = 0;
	u8 i = 0;
	char *msg = NULL;
	char tmp[10];

	if(SendCmd("AT+CSQ\r\n", "OK", 100,0,TIMEOUT_1S) == 1)
	{
		msg = strstr((char *)result_ptr->data,"+CSQ:");

		if(msg == NULL)
			return 0;

		memset(tmp,0,10);

		msg = msg + 5;

		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';

		*csq = nbiot_atoi(tmp,strlen(tmp));

		if(*csq == 0 || *csq >= 99)
		{
			*csq = 0;
			ret = 0;
		}
	}

	return ret;
}

//��ȡUEģ��״̬
/*
	rsrp	�ο��źŽ��չ���
	rssi	�����ź�ǿ�ȵȼ�
	snr		�����
	pci		����С����ʶ
	rsrq	�ο��źŽ�������
	ecl		�źŸ��ǵȼ�
*/
unsigned char bcxx_get_AT_NUESTATS(signed short *rsrp,
                                   signed short *rssi,
                                   signed short *snr,
                                   signed short *pci,
                                   signed short *rsrq)
{
	u8 ret = 0;
	u8 i = 0;
	char *msg = NULL;
	char tmp[10];

	if(SendCmd("AT+NUESTATS=CELL\r\n", "OK", 100,0,TIMEOUT_1S) == 1)
	{
		msg = strstr((char *)result_ptr->data,":CELL,");

		if(msg == NULL)
			return 0;

		memset(tmp,0,10);

		msg = msg + 6;

		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		*pci = nbiot_atoi(tmp,strlen(tmp));
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		*rsrp = nbiot_atoi(tmp,strlen(tmp));
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		*rsrq = nbiot_atoi(tmp,strlen(tmp));
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != ',')
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		*rssi = nbiot_atoi(tmp,strlen(tmp));
		
		i = 0;
		msg = msg + 1;
		memset(tmp,0,10);
		while(*msg != 0x0D)
		tmp[i ++] = *(msg ++);
		tmp[i] = '\0';
		*snr = nbiot_atoi(tmp,strlen(tmp));
	}

	return ret;
}

//��ģ���ȡʱ��
unsigned char m53xx_get_AT_CCLK(char *buf)
{
	unsigned char ret = 0;

    if(SendCmd("AT+CCLK?\r\n", "OK", 100,0,TIMEOUT_1S) == 1)
    {
        if(search_str((unsigned char *)result_ptr->data, "+CCLK:") != -1)
		{
			get_str1((unsigned char *)result_ptr->data, "+CCLK:", 1, "\r\n\r\nOK", 1, (unsigned char *)buf);

			ret = 1;
		}
    }

    return ret;
}

void mipl_generate(char *buf, size_t buflen, MIPL_T *mipl)
{
	strcpy(buf,"AT+MIPLCREATE=49,130031F10003F2002304001100000000000000123137322E31382E32342E3134323A35363833000131F300087100000000,0,49,0\r\n"); //���ĵ������ƽ̨
	SendCmd(buf,"+MIPLCREATE:0",300,0,TIMEOUT_2S);
	
//	strcpy(buf,"AT+MIPLCREATE=52,130034F10003F2002504001100000000000000143138332E3230372E3231352E3134333A35363833000131F30009710001000131,0,52,0\r\n"); //���ĵ������ƽ̨
//	SendCmd(buf,"+MIPLCREATE:0",300,0,TIMEOUT_2S);
	
//	SendCmd("AT+MIPLCREATEEX=\"nbiotbt.heclouds.com:5683\",1\r\n","+MIPLCREATEEX:0",300,0,TIMEOUT_2S);
}

void init_miplconf(u32 lifetime,const char *uri,const char *ep)
{
	MIPL_T mipl;
	char buffer[512];

	mipl.boot = MIPL_BOOT;
	mipl.encrypt = MIPL_ENCRYPT;
	mipl.debug = MIPL_DEBUG;
	mipl.port = MIPL_PORT;
	mipl.keep_alive = lifetime;
	mipl.uri = uri;
	mipl.uri_len = strlen(uri);
	mipl.ep = ep;
	mipl.ep_len = strlen(ep);
	mipl.block1 = MIPL_BLOCK1,	//COAP option BLOCK1(PUT or POST),0-6. 2^(4+n)  bytes
	mipl.block2 = MIPL_BLOCK2,	//COAP option BLOCK2(GET),0-6. 2^(4+n)  bytes
	mipl.block2th = MIPL_BLOCK2TH,
	mipl_generate(buffer,sizeof(buffer),&mipl);
}

void m53xx_addobj(uint16_t	   objid,
  	              uint8_t	   instcount,
                  uint8_t     *bitmap,
                  uint8_t      attrs,
                  uint8_t      acts)
{
	char tmp[10];

	memset(cmd_tx_buff,0,50);
	memcpy(cmd_tx_buff,"AT+MIPLADDOBJ=0,",sizeof("AT+MIPLADDOBJ=0,"));
	nbiot_itoa(objid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(instcount,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,(char *)bitmap);
	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,",");

	nbiot_itoa(attrs,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(acts,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,"\r\n");

	SendCmd(cmd_tx_buff,"OK",300,0,TIMEOUT_2S);
}

void m53xx_delobj(uint16_t  objid)
{
	 char tmp[10];

	 memset(cmd_tx_buff,0,50);
	 memcpy(cmd_tx_buff,"AT+MIPLDELOBJ=0,",sizeof("AT+MIPLDELOBJ=0,"));
	 nbiot_itoa(objid,tmp,10);
	 strcat(cmd_tx_buff,tmp);
	 strcat(cmd_tx_buff,"\r\n");

	 SendCmd(cmd_tx_buff,"OK",300,0,TIMEOUT_2S);
 }


 size_t m53xx_register_request( uint8_t  *buffer,
                                size_t    buffer_len,
                                uint16_t  lifetime,
                                uint8_t   waittime)
{
	char status = 0;
	char ative[6];

	nbiot_itoa(lifetime,ative,6);
	memcpy(buffer,"AT+MIPLOPEN=0,",sizeof("AT+MIPLOPEN=0,"));
	strcat((char *)buffer,ative);
	strcat((char *)buffer,",");
	nbiot_itoa(waittime,ative,6);
	strcat((char *)buffer,ative);
	strcat((char *)buffer,"\r\n");
	
	status=SendCmd((char *)buffer,"OK",300,0,TIMEOUT_2S);

	if(status==2)
		SendCmd((char *)buffer,"OK",300,0,TIMEOUT_2S);

	return buffer_len;
}

 size_t m53xx_register_update (uint16_t lifttime,
                               uint8_t  withobj,
                               uint8_t *buffer,
                               size_t  buffer_len)
{
	size_t  len = 0;
	char ative[6];

	nbiot_itoa(lifttime,ative,6);
	memcpy(buffer,"AT+MIPLUPDATE=0,",sizeof("AT+MIPLUPDATE=0,"));
	strcat((char *)buffer,ative);
	strcat((char *)buffer,",");
	nbiot_itoa(withobj,ative,1);
	strcat((char *)buffer,ative);
	strcat((char *)buffer,"\r\n");
	len=strlen((char *)buffer)+1;

	if(len<buffer_len)
	{
		SendCmd((char *)buffer,"OK",300,0,TIMEOUT_2S);

		return len;
	}

	return 0;
}

size_t m53xx_close_request( uint8_t  *buffer,
                             size_t    buffer_len)
{
	size_t  len = 0;

	len = strlen("AT+MIPLCLOSE=0\r\n")+1;

	if(len < buffer_len)
	{
		memcpy(buffer,"AT+MIPLCLOSE=0\r\n",len);
		SendCmd("AT+MIPLCLOSE=0\r\n","OK",300,0,TIMEOUT_2S);

		return len;
	}

	return 0;
}


void m53xx_notify_upload(const nbiot_uri_t *uri,uint8_t type,char *data,uint8_t flag,uint8_t index,uint16_t ackid)
{
	char tmp[10];

	memset(cmd_tx_buff,0,sizeof(cmd_tx_buff));
	memcpy(cmd_tx_buff,"AT+MIPLNOTIFY=0,0,",sizeof("AT+MIPLNOTIFY=0,0,"));
	nbiot_itoa(uri->objid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(uri->instid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(uri->resid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(type,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(strlen(data),tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,data);
	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,",");

	nbiot_itoa(index,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(flag,tmp,1);
	strcat(cmd_tx_buff,tmp);

	strcat(cmd_tx_buff,"\r\n");

	printf("send data:");
	printf("%s\r\n",cmd_tx_buff);

	SentData(cmd_tx_buff,"OK",100);
}

void m53xx_read_upload(const nbiot_uri_t *uri,uint8_t type,char *data,uint16_t msgid,uint8_t result,uint8_t index,uint8_t flag)
{

	char tmp[10];

	memset(cmd_tx_buff,0,sizeof(cmd_tx_buff));
	memcpy(cmd_tx_buff,"AT+MIPLREADRSP=0,",sizeof ("AT+MIPLREADRSP=0,"));
	nbiot_itoa(msgid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(result,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(uri->objid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(uri->instid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(uri->resid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(type,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(strlen(data),tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,data);
	strcat(cmd_tx_buff,"\"");
	strcat(cmd_tx_buff,",");

	nbiot_itoa(index,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	nbiot_itoa(flag,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,"\r\n");

	printf("read rsp:");
	printf("%s\r\n",cmd_tx_buff);

	SentData(cmd_tx_buff,"OK",100);
}

void m53xx_write_rsp(int suc,uint16_t msgid)
{
	char tmp[10];

	memset(cmd_tx_buff,0,50);
	memcpy(cmd_tx_buff,"AT+MIPLWRITERSP=0,",sizeof("AT+MIPLWRITERSP=0,"));
	nbiot_itoa(msgid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(suc,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,"\r\n");

	printf("write rsp:");
	printf("%s\r\n",cmd_tx_buff);

	SentData(cmd_tx_buff,"OK",100);

}

void m53xx_execute_rsp(int suc,uint16_t msgid)
{
	char tmp[10];

	memset(cmd_tx_buff,0,50);
	memcpy(cmd_tx_buff,"AT+MIPLEXECUTERSP=0,",sizeof("AT+MIPLEXECUTERSP=0,"));
	nbiot_itoa(msgid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");
	nbiot_itoa(suc,tmp,1);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,"\r\n");

	printf("execute rsp:");
	printf("%s\r\n",cmd_tx_buff);

	SentData(cmd_tx_buff,"OK",100);

}

void m53xx_discover_rsp(uint16_t objid,char *resid)
{
	char tmp[10];

	memset(cmd_tx_buff,0,50);
	memcpy(cmd_tx_buff,"AT+MIPLDISCOVERRSP=0,",sizeof("AT+MIPLDISCOVERRSP=0,"));
	nbiot_itoa(objid,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",1,");

	nbiot_itoa(strlen(resid)-2,tmp,10);
	strcat(cmd_tx_buff,tmp);
	strcat(cmd_tx_buff,",");

	strcat(cmd_tx_buff,resid);
	strcat(cmd_tx_buff,"\r\n");
	printf("discover rsp:");

	printf("%s\r\n",cmd_tx_buff);

	SentData(cmd_tx_buff,"OK",100);

}

size_t m53xx_delete_request( uint8_t  *buffer,
                             size_t    buffer_len)
{
	size_t  len=0;

	len=strlen("AT+MIPLDEL=0\r\n")+1;

	if(len<buffer_len)
	{
		memcpy(buffer,"AT+MIPLDEL=0\r\n",len);

		SendCmd("AT+MIPLDEL=0\r\n","OK",300,0,TIMEOUT_2S);

		return len;
	}

	return 0;
}











































