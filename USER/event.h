#ifndef __EVENT_H
#define __EVENT_H

#include "sys.h"
#include "common.h"

void RecordEventsECx(u8 ecx,u8 len,u8 *msg);
void CheckEventsEC15(RemoteControl_S ctrl);
void CheckEventsEC16(RemoteControl_S ctrl);
void CheckEventsEC17(RemoteControl_S ctrl);
void CheckEventsEC18(RemoteControl_S ctrl);
void CheckEventsEC19(RemoteControl_S ctrl);
void CheckEventsEC20(RemoteControl_S ctrl);
void CheckEventsEC28(u8 *cal1,u8 *cal2);
void CheckEventsEC51(u8 result,u8 *version);
void CheckEventsEC52(RemoteControl_S ctrl);





































#endif
