#include "sys.h"

/*-----------------------------------------------------------
 * 函数名：Shake_Init
 * 功能：初始震动传感器接口（GPIO配置为浮空输入模式）
 * 硬件要求：
 *-----------------------------------------------------------*/
void Shake_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;  // GPIO配置结构体
	
	/* 使能GPIO时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	/* 配置GPIO参数 */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入模式（高阻态）;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;  // 设置连接的GPIO引脚
	
	/* 应用GPIO配置 */
	GPIO_Init(Shake_GPIO_PORT, &GPIO_InitStruct);
}

/**
*读取引脚状态
*/
uint8_t Shake_GetValue(void)
{
	return 	GPIO_ReadInputDataBit(Shake_GPIO_PORT, Shake_GPIO_PIN);	
}
