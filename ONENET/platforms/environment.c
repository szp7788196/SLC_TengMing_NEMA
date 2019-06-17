/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
**/
#include "platform.h"
#include "m53xx.h"
#include "usart.h"
#include "uart4.h"
#include "delay.h"
#include "net_protocol.h"
#include "led.h"
#include "task_net.h"

static bool _nbiot_init_state = false;

void nbiot_init_environment(void)
{
	if ( !_nbiot_init_state )
	{
		m53xx_hard_init();
		
		delay_ms(500);

		netdev_init();

		_nbiot_init_state = true;
	}
}

void nbiot_clear_environment(void)
{
	if( _nbiot_init_state)
	{
//		Registered_Flag = 0;
		ConnectState = UNKNOW_STATE;
		LogInOutState = 0x00;
		_nbiot_init_state = false;
	}
}

void nbiot_reset(void)
{
//	NVIC_GenerateSystemReset();
}

