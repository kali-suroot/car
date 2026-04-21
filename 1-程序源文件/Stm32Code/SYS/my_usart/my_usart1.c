#include "sys.h"

vu16 USART1_RX_STA=0; 
u8 USART1_RX_BUF[USART1_MAX_RECV_LEN];//接收缓冲,最大USART1_MAX_RECV_LEN个字节.

void My_USART1(void)
{
    GPIO_InitTypeDef  GPIO_InitStrue;      // GPIO初始化结构体
    USART_InitTypeDef USART1_InitStrue;   // USART初始化结构体
    
    // 使能GPIOA和USART1的时钟（APB2总线）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    /* 配置USART1_TX引脚(PA9) */
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出模式
    GPIO_InitStrue.GPIO_Pin = USART1_GPIO_PIN_TX;    // PA9
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_10MHz;    // 10MHz速率
    GPIO_Init(GPIOA, &GPIO_InitStrue);
    
    /* 配置USART1_RX引脚(PA10) */
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入模式
    GPIO_InitStrue.GPIO_Pin = USART1_GPIO_PIN_RX;     // PA10
    GPIO_Init(GPIOA, &GPIO_InitStrue);
    
    /* USART1参数配置 */
    USART1_InitStrue.USART_BaudRate = 9600;                          // 波特率9600
    USART1_InitStrue.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART1_InitStrue.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;     // 收发模式
    USART1_InitStrue.USART_Parity = USART_Parity_No;                 // 无校验位
    USART1_InitStrue.USART_StopBits = USART_StopBits_1;              // 1位停止位
    USART1_InitStrue.USART_WordLength = USART_WordLength_8b;         // 8位数据长度
    
    USART_Init(USART1, &USART1_InitStrue);    // 初始化USART1
    
    // 使能USART1接收中断（RXNE: 接收缓冲区非空中断）
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 使能USART1外设
    USART_Cmd(USART1, ENABLE);    
}

/**
  * @brief  发送单个字节数据
  * @param  USARTx: 指定USART外设
  * @param  Data: 要发送的数据(0-0x1FF)
  * @retval 无
  */
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data)
{
	/* 参数检查 */
	assert_param(IS_USART_ALL_PERIPH(USARTx));
	assert_param(IS_USART_DATA(Data)); 
		
	/* 发送数据 */
	USARTx->DR = (Data & (uint16_t)0x01FF); // 写入数据寄存器
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); // 等待发送完成
}

__align(8) char USART1_TxBuff[256];  

void u1_printf(char* fmt,...) 
{  
	unsigned int i =0,length=0;
	
	va_list ap;
	va_start(ap,fmt);
	vsprintf(USART1_TxBuff,fmt,ap);
	va_end(ap);
	
	length=strlen((const char*)USART1_TxBuff);
	while(i<length)
	{
		USART_SendByte(USART1,USART1_TxBuff[i]);		
		i++;		
	}
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET);
	
}
//串口中断
void USART1_IRQHandler(void)
{
    uint8_t res;
   if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)  //判断是否发生中断
    {
		res =USART_ReceiveData(USART1);		 
		if((USART1_RX_STA&(1<<15))==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
		{ 
			if(USART1_RX_STA<USART1_MAX_RECV_LEN)	//还可以接收数据
			{
				USART1_RX_BUF[USART1_RX_STA++]=res;	//记录接收到的值	 
			}else 
			{
				USART1_RX_STA|=1<<15;				//强制标记接收完成
			} 
		}
       USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
