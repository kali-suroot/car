#ifndef __OLEDIIC_H
#define __OLEDIIC_H	

/* OLED I2C 驱动头文件
 * 适用硬件: SSD1306控制器 + 7位地址0x3C(写地址0x78)
 * 屏幕分辨率: 128x64
 * 通信方式: GPIO模拟I2C协议
 * 字体支持: 6x8 ASCII / 8x16 ASCII / 16x16汉字
 * 坐标系说明:
 *   - X轴范围: 0-127 (列地址)
 *   - Y轴范围: 0-7   (页地址，每页8个像素行) */

#include "sys.h"  // 依赖系统级头文件(包含STM32寄存器定义)

/* 状态宏定义 */
#define TRUE        1  // 逻辑真值
#define FALSE       0  // 逻辑假值

/* 硬件引脚配置 (以GPIOA为例) */
#define SCL_1_PORT    GPIOA  // SCL时钟线端口
#define SCL_1_PIN     GPIO_Pin_12  // PA12作为SCL引脚
#define SDA_1_PORT    GPIOA  // SDA数据线端口
#define SDA_1_PIN     GPIO_Pin_11  // PA11作为SDA引脚

/* GPIO电平操作宏 
 * 使用BRR/BSRR寄存器实现原子操作
 * BSRR: 低16位设置引脚，高16位复位引脚
 * BRR : 复位引脚寄存器 */
#define SCL_H         SCL_1_PORT->BSRR = SCL_1_PIN  // SCL置高电平
#define SCL_L         SCL_1_PORT->BRR  = SCL_1_PIN  // SCL置低电平
#define SDA_H         SDA_1_PORT->BSRR = SDA_1_PIN  // SDA置高电平
#define SDA_L         SDA_1_PORT->BRR  = SDA_1_PIN  // SDA置低电平

/* GPIO输入状态读取宏 
 * IDR寄存器存储引脚输入状态 */
#define SCL_read      SCL_1_PORT->IDR  & SCL_1_PIN  // 读取SCL电平状态
#define SDA_read      SDA_1_PORT->IDR  & SDA_1_PIN  // 读取SDA电平状态

/**************************** 功能函数声明 ******************************/

/* 初始化SSD1306控制器 
 * 包含GPIO初始化和配置命令序列 */
void oled_Init(void);

/* 颜色反相显示控制 
 * 参数: i=0正常模式, i=1反色模式 */
void oled_ColorTurn(uint8_t i);

/* 屏幕显示方向控制 
 * 参数: i=0正常方向, i=1旋转180度 */
void oled_DisplayTurn(uint8_t i);

/* 清屏函数 
 * 将GDDRAM全部写0x00 */
void oled_Clear(void);

/* 显示开关控制 
 * 关闭显示可降低功耗 */
void oled_Display_On(void);  // 开启显示
void oled_Display_Off(void); // 关闭显示

/* 字符串显示函数 
 * 参数:
 *   x,y: 起始坐标 
 *   p: 字符串指针(需以'\0'结尾)
 *   Char_Size: 字体大小(8/16) */
void oled_ShowString(unsigned char x,unsigned char y, unsigned char *p,unsigned char Char_Size);

/* 16x16点阵汉字显示 
 * 参数:
 *   no: 字库数组索引(每个汉字占32字节) */
void oled_ShowCHinese(unsigned char x,unsigned char y,unsigned char no);

/* 位图显示函数 
 * 参数:
 *   x0,y0: 起始坐标 
 *   x1,y1: 结束坐标(需大于起始坐标)
 *   BMP: 位图数据数组(纵向分页存储) */
void oled_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);

/* 数字显示系列函数 
 * 参数:
 *   num: 要显示的数值 
 *   len: 显示位数 
 *   size: 字体大小 */
void oled_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char Char_Size); // 字符显示
void OLED_ShowDNum(unsigned char x,unsigned char y,int Dnum,unsigned char size1); // 十进制整型显示
void OLED_ShowFNum(unsigned char x,unsigned char y,float Fnum,unsigned char size1); // 浮点数显示

/* I2C操作状态标志 
 * 0:失败 1:成功 (由底层函数更新) */
extern unsigned char OLED_IIC_stu; 

#endif
