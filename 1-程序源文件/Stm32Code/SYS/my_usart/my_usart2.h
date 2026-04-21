#ifndef __MY_USART2_H__
#define __MY_USART2_H__

/*
 * USART2 驱动头文件
 * 功能: 实现STM32 USART2串口通信基础功能
 * 特性:
 *   - 支持阻塞式发送
 *   - 1024字节接收缓冲区
 *   - 自定义协议格式数据包发送
 * 硬件配置:
 *   - TX: PA2 (GPIO_Pin_2)
 *   - RX: PA3 (GPIO_Pin_3)
 *   - 波特率: 115200 (在My_USART2()中配置)
 */

#include "sys.h"  // 系统级头文件，包含寄存器定义和基础宏

/* 硬件引脚定义 */
#define USART2_GPIO_PIN_TX   GPIO_Pin_2  // 发送引脚PA2
#define USART2_GPIO_PIN_RX   GPIO_Pin_3  // 接收引脚PA3

/* 全局变量声明 */
extern u16 USART2_RxCounter;      // 接收数据计数器(当前接收字节数)
extern char USART2_RxBuff[1024];  // 接收缓冲区(1024字节环形队列)

/* 函数声明 */
void My_USART2(void);  // USART2初始化函数(配置GPIO和串口参数)

// 格式化打印函数(类printf实现)
// 注意: 使用内部256字节缓冲区，长数据可能被截断
void u2_printf(char* fmt, ...);

// 自定义协议数据包发送函数
// 数据包格式: [长度高字节][长度低字节][数据1][数据2]...
// 参数data: 需包含完整包头的数据缓冲区指针
void u2_TxData(unsigned char *data);

#endif



