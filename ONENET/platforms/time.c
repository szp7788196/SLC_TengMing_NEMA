/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
**/

#include <platform.h>
#include <time.h>
//#include <unistd.h>
//#include <sys/time.h>
#include "stm32f10x_rtc.h"
#include "stm32f10x.h"
#include "core_cm3.h"
static __IO time_t SystickTime=0; 
static u8  c_us=0;//us��ʱ������
static u16 c_ms=0;//ms��ʱ������


void delay_init()	 
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//ѡ���ⲿʱ��  HCLK/8
	c_us=SystemCoreClock/8000000;	//Ϊϵͳʱ�ӵ�1/8  
	c_ms=(u16)c_us*1000;//��ucos��,����ÿ��ms��Ҫ��systickʱ����   
}

void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*c_us; 
	SysTick->VAL=0x00;       
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;  
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;     
	SysTick->VAL =0X00;     
}
void uDelay(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*c_us; 
	SysTick->VAL=0x00;       
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;  
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;     
	SysTick->VAL =0X00;     
}
/*
************************************************************
*	�������ƣ�	delay_ms_sub
*
*	�������ܣ�	΢�뼶����ʱ
*
*	��ڲ�����	delay_ms_sub����72M������,delay_ms_sub<=1864 
*
*	���ز�����	��
************************************************************
*/
void delay_ms_sub(u16 sub_ms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)sub_ms*c_ms;
	SysTick->VAL =0x00;        
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;         
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;    
	SysTick->VAL =0X00;     
} 


/*
************************************************************
*	�������ƣ�	delay_ms
*
*	�������ܣ�	΢�뼶����ʱ
*
*	��ڲ�����	ms����ʱ��ʱ��
*
*	���ز�����	��
************************************************************
*/
void mDelay(unsigned short ms)
{

	unsigned char repeat = 0;
	unsigned short remain = 0;
	
	repeat = ms / 500;
	remain = ms % 500;
	
	while(repeat)
	{
		delay_ms_sub(500);
		repeat--;
	}
	
	if(remain)
	{
		delay_ms_sub(remain);
	}
}


void RTC_Init(void)
{
	unsigned char temp = 0;
	RCC->APB1ENR|=1<<28;

	RCC->APB1ENR|=1<<27;

	PWR->CR|=1<<8;

	RCC->BDCR|=1<<16;

	RCC->BDCR&=~(1<<16); 

		//internal 40k;
	RCC->CSR|=(1<<0);
	while((!(RCC->CSR&0X02))&&temp<250)//�ȴ��ⲿʱ�Ӿ��� 
	{ 
		mDelay(10);
		temp++;
	};
	
	if(temp>=250)
		return;
	RCC->BDCR&=~(0x3<<8);
	RCC->BDCR|=1<<9; 
	RCC->BDCR|=1<<15;
	while(!(RTC->CRL&(1<<5)));
	while(!(RTC->CRL&(1<<3)));
	RTC->CRH|=0X01;
	while(!(RTC->CRL&(1<<5)));
	RTC->CRL|=1<<4;
	
	RTC->PRLH=0X0000; 
	RTC->PRLL=39999; 
	RTC->CRL&=~(1<<4); 
	while(!(RTC->CRL&(1<<5)));
}

void nbiot_sleep( int milliseconds)
{
    mDelay(milliseconds);
	  return;
}


time_t nbiot_time( void )
{

   return (RTC->CNTL | RTC->CNTH << 16);
}

void nbiot_time_init(void)
{
   delay_init();
	 RTC_Init();
   return ;
}


