#include "sys.h"

LED_CONTROLLER Led_Controller; // 定义LED控制结构体

/**
  * @brief  LED硬件初始化函数
  * @note   配置PB5/PB6/PB7为推挽输出模式，默认关闭所有LED
  * @param  无
  * @retval 无
  */
void LED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;  // GPIO配置结构体
    
    /* 使能GPIOB时钟（APB2总线）*/
    RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE);  
    
    /* 配置PB5/PB6/PB7引脚 */
    GPIO_InitStruct.GPIO_Pin = LED_GPIO_PIN1 | LED_GPIO_PIN2 | LED_GPIO_PIN3;  // 同时初始化3个引脚
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出模式
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;  // 50MHz高速模式（适合LED控制）
    
    /* 应用GPIO配置 */
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);     
    
    /* 初始化关闭所有LED */
    LED1_OFF;  // 关闭LED1
    LED2_OFF;  // 关闭LED2
    LED3_OFF;  // 关闭LED3
}

// 设置闪烁模式
void LED_SetBlinkMode(LED_STATE_Typedef mode)
{
    Led_Controller.CurrentMode = mode;
    Led_Controller.LastTick = HAL_GetTick();
    
    switch(mode) {
        case STEADY_OFF:
            LED3_OFF;
            Led_Controller.LED_State = 0;
            break;
		
        case STEADY_ON:
            LED3_ON;
            Led_Controller.LED_State = 1;
            break;
		
        case SLOW_BLINK:
            Led_Controller.Duration_ON = 500;
            Led_Controller.Duration_OFF = 1500;
            LED3_ON; // 开始亮灯
            Led_Controller.LED_State = 1;
            break;
		
        case FAST_BLINK:
            Led_Controller.Duration_ON = 100;
            Led_Controller.Duration_OFF = 100;
            LED3_ON; // 开始亮灯
            Led_Controller.LED_State = 1;
            break;
    }
}

// 更新LED状态
void LED_Update(void)
{
    if (Led_Controller.CurrentMode == STEADY_OFF || Led_Controller.CurrentMode == STEADY_ON) {
        return; // 常亮或常灭模式不需要更新
    }
    
    uint32_t Current_Tick = HAL_GetTick(); // 获取当前时钟
	
	// 根据LED开关状态，切换闪烁间隔时间
    uint32_t required_duration = Led_Controller.LED_State ? Led_Controller.Duration_ON : Led_Controller.Duration_OFF;
    
    if (Current_Tick - Led_Controller.LastTick >= required_duration) {
        // 时间到，切换状态
        if (Led_Controller.LED_State) {
            LED3_OFF;
            Led_Controller.LED_State = 0;
        } else {
            LED3_ON;
            Led_Controller.LED_State = 1;
        }
        Led_Controller.LastTick = Current_Tick; // 重置计时器
    }
}

