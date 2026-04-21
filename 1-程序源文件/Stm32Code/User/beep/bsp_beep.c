#include "sys.h" 

void Beep_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;           // 定义io结构体
    RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE); // 使能蜂鸣器所在GPIO端口的时钟

    GPIO_InitStructure.GPIO_Pin = BEEP_GPIO_PIN;      // 设置要初始化的GPIO引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 配置GPIO模式为推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 配置GPIO输出速度为50MHz

    GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStructure); // 初始化蜂鸣器所在GPIO端口

    BEEP_OFF; // 蜂鸣器关
}

/* 蜂鸣器嘀一声 */
void Short_Beep(void)
{
	BEEP_ON;
	delay_ms(100);
	BEEP_OFF;
}
