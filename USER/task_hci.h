#ifndef __TASK_HCI_H
#define __TASK_HCI_H

#include "sys.h"
#include "rtos_task.h"

extern TaskHandle_t xHandleTaskHCI;

void vTaskHCI(void *pvParameters);
u16 HCI_DataAnalysis(u8 *inbuf,u16 inbuf_len,u8 *outbuf);






































#endif
