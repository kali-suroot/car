#ifndef __MY_USART1_H__
#define __MY_USART1_H__

#include "sys.h"

#define USART1_GPIO_PIN_TX   GPIO_Pin_9
#define USART1_GPIO_PIN_RX   GPIO_Pin_10
#define USART1_MAX_RECV_LEN		400					//最大接收缓存字节数

void My_USART1(void);
void u1_printf(char* fmt,...);
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data);

extern vu16 USART1_RX_STA; 
extern u8 USART1_RX_BUF[USART1_MAX_RECV_LEN]; 				//接收缓冲,最大USART3_MAX_RECV_LEN个字节

#endif

