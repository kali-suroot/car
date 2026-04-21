#include "sys.h"

void NVIC_Config(void)
{
    /* 定义NVIC初始化结构体 */
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* 设置NVIC优先级分组为第2组
     * 优先级分组规则：
     * - 第2组：2位抢占优先级，2位子优先级
     * - 可配置范围：抢占优先级0-3，子优先级0-3
     */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 配置TIM2中断 */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;           // 设置TIM2中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
    NVIC_Init(&NVIC_InitStructure);                           // 应用配置
    
	/* 配置USART1中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         // 设置USART1中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
    NVIC_Init(&NVIC_InitStructure); 
	
	/* 配置USART2中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         // 设置USART2中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
    NVIC_Init(&NVIC_InitStructure); 
}



