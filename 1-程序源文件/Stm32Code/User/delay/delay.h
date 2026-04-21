#ifndef __DELAY_H
#define __DELAY_H
#include "sys.h"
extern  volatile unsigned int       sysTickUptime;	
void delay_init(void);
void delay_ms(u16 nms);
void delay_us(u32 nus);

void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
unsigned int GetSysTime_us(void) ;

#endif
