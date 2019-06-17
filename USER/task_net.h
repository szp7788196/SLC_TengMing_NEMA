#ifndef __TASK_NET_H
#define __TASK_NET_H

#include "sys.h"
#include "common.h"
#include "rtos_task.h"
#include "m53xx.h"
#include "task_sensor.h"



extern TaskHandle_t xHandleTaskNET;
extern SensorMsg_S *p_tSensorMsgNet;

extern nbiot_device_t *dev;

extern CONNECT_STATE_E ConnectState;	//Á¬½Ó×´Ì¬


void vTaskNET(void *pvParameters);































#endif
