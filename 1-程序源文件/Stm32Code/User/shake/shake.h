#ifndef __shake_H__
#define __shake_H__

#include "sys.h"

#define      Shake_RCC          RCC_APB2Periph_GPIOA   // GPIOA时钟使能（APB2总线）
#define      Shake_GPIO_PORT    GPIOA                 // 传感器连接的GPIO端口
#define      Shake_GPIO_PIN     GPIO_Pin_4			 // 使用的GPIO引脚

void Shake_Init(void); // 震动传感器初始化
uint8_t Shake_GetValue(void); // 读取印脚状态

#endif
