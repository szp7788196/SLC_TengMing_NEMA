#ifndef __TASK_MAIN_H
#define __TASK_MAIN_H

#include "sys.h"
#include "rtos_task.h"
#include "net_protocol.h"
#include "common.h"



extern TaskHandle_t xHandleTaskMAIN;

extern RemoteControl_S ContrastControl;

void vTaskMAIN(void *pvParameters);

void CheckSwitchStatus(RemoteControl_S *ctrl);

u8 LookUpStrategyList(pControlStrategy strategy_head,RemoteControl_S *ctrl,u8 *update);




































#endif
