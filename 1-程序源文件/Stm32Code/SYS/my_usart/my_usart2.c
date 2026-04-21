#include "sys.h"

/* USART2全局接收计数器(记录当前接收数据长度) */
u16 USART2_RxCounter = 0;

/* USART2接收缓冲区(1024字节环形缓冲区) */
char USART2_RxBuff[1024];

/* 函数功能: USART2初始化配置
 * 硬件连接:
 *   TX - PA2(复用推挽输出)
 *   RX - PA3(浮空输入)
 * 通信参数:
 *   - 波特率: 115200bps
 *   - 数据位: 8位
 *   - 停止位: 1位
 *   - 校验位: 无
 *   - 流控  : 无
 * 特殊配置:
 *   - 使能接收中断(USART_IT_RXNE)
 *   - 使用APB1总线时钟(36MHz典型值) */
void My_USART2(void)
{
    GPIO_InitTypeDef  GPIO_InitStrue;  // GPIO配置结构体
    USART_InitTypeDef USART2_InitStrue; // USART配置结构体

    /* 时钟使能配置 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // 使能USART2时钟

    /* TX引脚(PA2)配置 */
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出模式
    GPIO_InitStrue.GPIO_Pin = USART2_GPIO_PIN_TX;    // PA2引脚(需宏定义)
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_10MHz;    // 输出速度10MHz
    GPIO_Init(GPIOA, &GPIO_InitStrue);               // 应用TX配置

    /* RX引脚(PA3)配置 */
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入模式
    GPIO_InitStrue.GPIO_Pin = USART2_GPIO_PIN_RX;    // PA3引脚(需宏定义)
    GPIO_Init(GPIOA, &GPIO_InitStrue);               // 应用RX配置

    /* USART参数配置 */
    USART2_InitStrue.USART_BaudRate = 115200;        // 波特率115200
    USART2_InitStrue.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART2_InitStrue.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收发模式
    USART2_InitStrue.USART_Parity = USART_Parity_No; // 无校验位
    USART2_InitStrue.USART_StopBits = USART_StopBits_1; // 1位停止位
    USART2_InitStrue.USART_WordLength = USART_WordLength_8b; // 8位数据长度

    USART_Init(USART2, &USART2_InitStrue);           // 应用USART配置

    /* 中断与使能配置 */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);   // 使能接收中断(接收数据寄存器非空中断)
    USART_Cmd(USART2, ENABLE);                       // 使能USART2外设
}

/* 函数功能: USART2格式化输出函数(阻塞式发送)
 * 参数说明:
 *   fmt : 格式化字符串(支持%d,%f等标准格式符)
 *   ... : 可变参数列表(与fmt中的格式符匹配)
 * 实现流程:
 *   1. 使用va_list处理可变参数
 *   2. 通过vsprintf将格式化结果存入256字节发送缓冲区
 *   3. 轮询方式逐个字节发送数据
 *   4. 等待最后一个字节发送完成(TC标志)
 *     */
__align(8) char USART2_TxBuff[256];  // 发送缓冲区(8字节对齐)

void u2_printf(char* fmt, ...)
{
    unsigned int i = 0, length = 0;

    /* 可变参数处理 */
    va_list ap;
    va_start(ap, fmt);
    vsprintf(USART2_TxBuff, fmt, ap); // 格式化字符串到缓冲区
    va_end(ap);

    /* 计算有效数据长度 */
    length = strlen((const char*)USART2_TxBuff);

    /* 轮询发送数据 */
    while(i < length)
    {
        USART_SendByte(USART2, USART2_TxBuff[i]); // 阻塞发送单字节
        i++;
    }

    /* 等待发送完成(检测TC标志而非TXE) */
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}
//void USART2_IRQHandler(void)
//{

//	if(Connect_flag == 0)
//	{
//		if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
//		{
//
//			if(USART2->DR)
//			{
//				USART2_RxBuff[USART2_RxCounter]=USART_ReceiveData(USART2);
//				USART2_RxCounter++;
//			}
//
//		}
//	}
//	else
//	{
//		USART2_RxBuff[USART2_RxCounter] =USART_ReceiveData(USART2);
//		if(USART2_RxCounter == 0)
//		{
//			TIM_Cmd(TIM4,ENABLE);
//		}else
//		{
//			TIM_SetCounter(TIM4,0);
//		}
//		USART2_RxCounter++;
//	}
//};


/* 函数功能: USART2阻塞式数据包发送
 * 参数说明:
 *   data - 数据包指针，结构为：
 *          [0]   : 长度高字节
 *          [1]   : 长度低字节(总长度= data[0]<<8 | data[1])
 *          [2~n+1]: 实际数据(n=数据长度)
 * 执行流程:
 *   1. 等待上一次传输完成(TC标志置位)
 *   2. 解析数据包前两字节获取数据长度
 *   3. 按长度值遍历发送后续字节
 *   4. 每个字节发送后等待硬件传输完成

 *    */
void u2_TxData(unsigned char *data)
{
    int	i;

    /* 等待历史传输完成 */
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);

    /* 解析长度并遍历发送 */
    for(i = 1; i <= (data[0] * 256 + data[1]); i ++) // 长度= data[0]<<8 | data[1]
    {
        USART_SendData(USART2, data[i + 1]); // 发送data[2]~data[length+1]

        /* 阻塞等待单字节发送完成 */
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    }
}
