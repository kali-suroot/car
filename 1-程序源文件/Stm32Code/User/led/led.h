#ifndef __LED_H__
#define __LED_H__

#include "sys.h"

/* LED引脚定义（对应GPIOB）*/
#define LED_GPIO_PIN1   GPIO_Pin_5  // LED1连接PB5
#define LED_GPIO_PIN2   GPIO_Pin_6  // LED2连接PB6
#define LED_GPIO_PIN3   GPIO_Pin_7  // LED3连接PB7

#define LED_GPIO_PORT  GPIOB        // LED所用GPIO端口
#define LED_GPIO_CLK   RCC_APB2Periph_GPIOB  // GPIOB时钟使能宏

#define LED1_OFF GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN1) // PB5输出低电平，LED1灭
#define LED1_ON GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN1)    // PB5输出高电平，LED1亮
                                                             
#define LED2_OFF GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN2) // PB6输出低电平，LED2灭
#define LED2_ON GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN2)    // PB6输出高电平，LED2亮
                                                             
#define LED3_OFF GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN3) // PB7输出低电平，LED3灭
#define LED3_ON GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN3)    // PB7输出高电平，LED3亮

/* LED状态枚举类型 */
typedef enum {
	STEADY_OFF = 0, // 保持熄灭
	STEADY_ON,		// 保持开启
	SLOW_BLINK,		// 慢速闪烁
	FAST_BLINK		// 快速闪烁
} LED_STATE_Typedef;

/* LED控制结构体类型 */
typedef struct {
    LED_STATE_Typedef CurrentMode;	// 当前模式
    uint8_t LED_State;				// 当前状态: 0=OFF, 1=ON
    uint32_t LastTick;				// 上次状态改变的时间戳
    uint32_t Duration_ON;			// 亮灯持续时间
    uint32_t Duration_OFF;			// 灭灯持续时间
} LED_CONTROLLER;

extern LED_CONTROLLER Led_Controller; // 定义LED控制结构体

/* 函数声明 */
void LED_GPIO_Config(void); // LED硬件初始化函数
void LED_SetBlinkMode(LED_STATE_Typedef mode);
void LED_Update(void);

#endif
