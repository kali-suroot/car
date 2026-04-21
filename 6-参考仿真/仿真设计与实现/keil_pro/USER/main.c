#include "stm32f10x.h"
#include "LQ12864.h"
#include "adc.h"
#include "dth11.h"
#include "delay.h"

#define	LED1 PCout(0) //数据端口	PA0 
#define	BEEP PCout(4)
#define	LED2 PCout(1)
#define	LED3 PCout(2)

#define KEY1  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)//读取按键1
#define KEY2  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_2)//读取按键2 
#define KEY3  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3)//读取按键3(WK_UP) 
#define KEY4  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)//读取按键3(WK_UP) 
#define KEY5  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)
#define KEY6  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_10)
#define  DEBUG_USARTx                   USART1
#define  DEBUG_USART_CLK                RCC_APB2Periph_USART1
#define  DEBUG_USART_APBxClkCmd         RCC_APB2PeriphClockCmd
#define  DEBUG_USART_BAUDRATE           9600

// USART GPIO 引脚宏定义
#define  DEBUG_USART_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define  DEBUG_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
    
#define  DEBUG_USART_TX_GPIO_PORT         GPIOA   
#define  DEBUG_USART_TX_GPIO_PIN          GPIO_Pin_9
#define  DEBUG_USART_RX_GPIO_PORT       GPIOA
#define  DEBUG_USART_RX_GPIO_PIN        GPIO_Pin_10

#define  DEBUG_USART_IRQ                USART1_IRQn
#define  DEBUG_USART_IRQHandler         USART1_IRQHandler

short tem,flag1,flag2,flag3,flag4,flag5,flag44;
short flag11,flag22,flag33,flag44,flag55;
void Beep_Init()
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_13|GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);		
	GPIO_SetBits(GPIOB,GPIO_Pin_13);	
	GPIO_SetBits(GPIOB,GPIO_Pin_14);	

}
void KEY_Init(void) //IO初始化
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使能PORTA,PORTE时钟

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_10;//KEY0-KEY2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOE2,3,4

}
void Delay_DS18B20(int num)
{
	while(num--) ;
}

void LED_Init1(void)
{
 
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	 //使能PB,PE端口时钟
		
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;				 //LED0-->PB.5 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOC, &GPIO_InitStructure);					 //根据设定参数初始化GPIOB.5
	 GPIO_SetBits(GPIOC,GPIO_Pin_0);	
	 GPIO_SetBits(GPIOC,GPIO_Pin_1);
   GPIO_SetBits(GPIOC,GPIO_Pin_2);	
	 GPIO_SetBits(GPIOC,GPIO_Pin_3);
	 GPIO_SetBits(GPIOC,GPIO_Pin_4);	
   GPIO_SetBits(GPIOC,GPIO_Pin_5);

}

void USART_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	// ????GPIO???
	DEBUG_USART_GPIO_APBxClkCmd(DEBUG_USART_GPIO_CLK, ENABLE);
	
	// ?????????
	DEBUG_USART_APBxClkCmd(DEBUG_USART_CLK, ENABLE);

	// ?USART Tx?GPIO?????????
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);

  // ?USART Rx?GPIO?????????
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);
	
	// ?????????
	// ?????
	USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
	// ?? ?????
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// ?????
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// ?????
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	// ???????
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	// ??????,????
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// ??????????
	USART_Init(DEBUG_USARTx, &USART_InitStructure);

	// ????
	USART_Cmd(DEBUG_USARTx, ENABLE);	    
}
void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure; 
	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	/* Enable the USARTy Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
typedef struct __FILE FILE;
///???c???printf???,???????printf??
int fputc(int ch, FILE *f)
{
		/* ??????????? */
		USART_SendData(DEBUG_USARTx, (uint8_t) ch);
		
		/* ?????? */
		while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_TXE) == RESET);		
	
		return (ch);
}

///???c???scanf???,???????scanf?getchar???
int fgetc(FILE *f)
{
		/* ???????? */
		while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_RXNE) == RESET);

		return (int)USART_ReceiveData(DEBUG_USARTx);
}

int main(void)
{	 
	
	uint16_t val,aa,va22,aa1,va11,aa22,flagqp=0,wds=31,tds=60,phs=40,zds=40,flagqq=0;
	u8 tem = 0, hum = 0;
	char buf[100] = { 0 };

	LCD_Init() ; 	
	Adc_Init();

	LED_Init1();
	KEY_Init();
	Beep_Init();

  DHT11_Init();
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	USART_Config();

	while(1)
	{
			
				
		     DHT11_Read_Data(&tem, &hum);
	
							
		    if(flag2==0)
				{
							if(flagqp==0)
							{
									flagqp=1;
									LCD_Init() ;						
							}
							sprintf(buf, "汽车防盗系统");		
							LCD_P14x16Str(20, 0, buf);
							sprintf(buf, "温度：");		
							LCD_P14x16Str(0, 2, buf);
							sprintf(buf, "%d C", tem);		
							LCD_P8x16Str(45, 2, buf);									
							
								
							va22 = Get_Adc(1);  //???????adc?
							aa22=va22*0.02;		   	
							sprintf(buf, "E:");		
							LCD_P8x16Str(0, 4, buf);
							sprintf(buf, "%02d", aa22);		
							LCD_P8x16Str(20, 4, buf);
							
							val = Get_Adc(0);  //???????adc?
							aa1=val*0.02;
							sprintf(buf, "N:");		
							LCD_P8x16Str(65, 4, buf);
							sprintf(buf, "%02d", aa1);		
							LCD_P8x16Str(85, 4, buf);
							if(KEY2==0)
							{
									if(KEY2==0)
									{ 
												flag1=1;
										
									}
									while(!KEY2);
							}
							if((flag1%2)==1)
							{
										sprintf(buf, "无人");		
										LCD_P14x16Str(65, 6, buf);
							      LED1=1;LED2=0;
								    if(KEY5==0)
										{
												if(KEY5==0)
												{ 
													flag4++;
													
												}
												while(!KEY5);
										}
										if(KEY6==0)
										{
												if(KEY6==0)
												{ 
													flag5++;
													
												}
												while(!KEY6);
										}
									if((tem>wds)||((flag4%2)==1)||((flag5%2)==1))
									{
											 LED3=0;
											 BEEP=0;
									}
									else
									{
											 LED3=1;
											 BEEP=1;
									}
									if(((flag5%2)==1))
									{
											  sprintf(buf, "发生震动");		
												LCD_P14x16Str(0, 6, buf);
									}
									else
									{
											 sprintf(buf, "        ");		
										    LCD_P8x16Str(0, 6, buf);
									}
									
							}	
							if((flag1%2)==0)
							{
										sprintf(buf, "    ");		
										LCD_P8x16Str(65, 6, buf);
										LED1=0;LED2=1;
							}								
							
							

				}
				if(flag2==1)
				{		
							if(flagqp==0)
							{
									flagqp=1;BEEP=1;LED1=1;LED2=1;LED3=1;
									LCD_Init() ;						
							}
							 sprintf(buf, "阈值设置");		
							 LCD_P14x16Str(30, 0, buf);
							 sprintf(buf, "温度上限：");		
							 LCD_P14x16Str(10, 2, buf);
							 sprintf(buf, "%d C", wds);		
							 LCD_P8x16Str(80, 2, buf);
						
							 sprintf(buf, "*");		
							 LCD_P8x16Str(0, 2, buf);
							
							 if(KEY3==0)
							 {
									if(KEY3==0)
									{ 
											wds++; 
											
									}
									while(!KEY3);
							 }
							 if(KEY4==0)
							 {
									if(KEY4==0)
									{ 
											wds--; 
											
									}
									while(!KEY4);
							 }
				}	
		  
			 if(KEY1==0)
			 {
					if(KEY1==0)
					{ 
							flag2++;  if(flag2>1)flag2=0;	
						  flagqp=0;
						  flag1=0;
					}
					while(!KEY1);
			 }
			
		 
			
	}
}

