#include "sys.h"

/********************************************************************************/
/* OLED显示模块基础配置宏定义 */
#define OLED_MODE 0     // OLED接口模式（0=I2C，1=SPI，需与硬件匹配）
#define SIZE 8          // 基础字体大小（8x8像素）
#define XLevelL 0x00    // 显存列地址低位起始值
#define XLevelH 0x10    // 显存列地址高位起始值（组合使用设置显存窗口）
#define Max_Column 128  // 屏幕最大列数（像素宽度）
#define Max_Row 64      // 屏幕最大行数（像素高度）
#define Brightness 0xFF // 初始亮度值（0x00-0xFF，FF为最亮）
#define X_WIDTH 128     // 物理屏幕宽度（像素）
#define Y_WIDTH 64      // 物理屏幕高度（像素）

#define OLED_CMD 0  // 操作类型：写命令（控制寄存器/参数）
#define OLED_DATA 1 // 操作类型：写数据（显存内容）

/********************************************************************************/
/* 基础硬件控制函数 */
void Delay_50ms(unsigned int Del_50ms); // 阻塞延时函数（单位：50ms周期数）
void Delay_1ms(unsigned int Del_1ms);   // 阻塞延时函数（单位：1ms周期数）

/* I2C协议层操作 */
void IIC_Start(void);                              // 生成I2C起始条件（SCL高时SDA下降沿）
void IIC_Stop(void);                               // 生成I2C停止条件（SCL高时SDA上升沿）
void Write_IIC_Command(unsigned char IIC_Command); // 发送单字节命令（自动设置DC线为CMD模式）
void Write_IIC_Data(unsigned char IIC_Data);       // 发送单字节数据（自动设置DC线为DATA模式）
void Write_IIC_Byte(unsigned char IIC_Byte);       // 原始I2C字节发送（需手动控制DC线状态）
void IIC_Wait_Ack(void);                           // 等待从设备应答（检测SDA线低电平）

/* OLED显示控制层 */
void OLED_WR_Byte(unsigned dat, unsigned cmd);                                                    // 通用字节写入（cmd=0写命令，cmd=1写数据）
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char Char_Size); // 指定坐标显示字符
void OLED_Set_Pos(unsigned char x, unsigned char y);                                              // 设置显存写入起始坐标（x:0-127, y:0-7对应8页）
void fill_picture(unsigned char fill_Data);                                                       // 全屏填充统一数据（用于清屏或测试）

/* 函数申明 -----------------------------------------------*/
extern void delay_us(unsigned int nus);
/* 变量定义 ----------------------------------------------*/

/*-----------------------------------------------------------
 * 函数名：I2C_delay
 * 功能：I2C总线时序控制短延时
 * 说明：
 *   - 通过空循环实现微妙级延时
 *   - 经实测i=5为最低有效值（24MHz主频下约0.5us）
 *   - 延时精度受编译器优化和主频影响，移植需重新校准
 *-----------------------------------------------------------*/
static void I2C_delay(void)
{
    unsigned char i = 5; // 这里可以优化速度，经测试最低到5还能写入
    while (i)
    {
        i--;
    }
}

/*-----------------------------------------------------------
 * 函数名：delay5ms
 * 功能：产生约5ms延时（精度较低）
 * 说明：
 *   - 空循环实现毫秒级延时
 *   - 实际延时时间与主频强相关（示例值基于特定时钟配置）
 *   - 建议改用定时器实现精确延时
 *-----------------------------------------------------------*/
static void delay5ms(void)
{
    int i = 50; // 循环次数经验值（24MHz下约5ms）

    while (i)
    {
        i--;
    }
}

/*
 * 函数名：I2C_Start
 * 功能：生成I2C起始条件并检测总线状态
 * 返回值：
 *   TRUE  - 起始条件生成成功
 *   FALSE - 总线忙（SDA被拉低）或总线错误（无法拉低SDA）
 * 时序说明：
 *   1. 初始状态：SCL和SDA均为高电平（总线空闲）
 *   2. 检测总线是否被占用（SDA=0表示总线忙）
 *   3. 产生起始条件（SDA在SCL高电平期间产生下降沿）
 *   4. 验证起始条件有效性（成功拉低SDA）
 */
static unsigned char I2C_Start(void)
{
    SDA_H;       // 释放SDA线（拉高）
    SCL_H;       // 释放SCL线（拉高）
    I2C_delay(); // 等待电平稳定

    /* 总线状态检测 */
    if (!SDA_read)    // 检测SDA线是否为低电平
        return FALSE; // 总线被其他设备占用（返回错误）

    /* 生成起始条件 */
    SDA_L; // 在SCL高电平期间拉低SDA（起始信号）
    I2C_delay();

    /* 验证起始信号 */
    if (SDA_read)     // 检测SDA线是否成功拉低
        return FALSE; // SDA无法被拉低（总线异常）

    SDA_L; // 保持SDA低电平（配合后续SCL操作）
    I2C_delay();
    return TRUE; // 起始条件生成成功
}

/*
 * 函数名：I2C_Stop
 * 功能：生成I2C停止条件并释放总线
 * 时序说明：
 *   1. 确保SCL先置低（避免在SCL高时直接改变SDA形成起始条件）
 *   2. 将SDA置低建立稳定状态
 *   3. 先拉高SCL再拉高SDA，在SCL高电平期间SDA上升沿形成停止条件

 */
static void I2C_Stop(void)
{
    SCL_L;       // 先将SCL置低，为电平切换准备安全环境
    I2C_delay(); // 等待电平稳定
    SDA_L;       // SDA置低建立明确电平状态
    I2C_delay();
    SCL_H; // 拉高SCL（此时SCL和SDA均为高）
    I2C_delay();
    SDA_H;       // 在SCL高电平期间释放SDA（产生停止条件的上升沿）
    I2C_delay(); // 保证停止条件持续时间（>4us）
}

/*
 * 函数名：I2C_Ack
 * 功能：生成I2C应答信号（ACK）
 * 时序说明：
 *   1. SCL置低电平（数据线允许变化）
 *   2. SDA置低电平（ACK信号核心动作）
 *   3. SCL置高电平（主机检测ACK信号）
 *   4. SCL恢复低电平（为后续传输准备）
 * 协议规范：
 *   - 应答信号发生在第9个时钟周期（每个字节传输后的ACK周期）
 *   - SDA低电平表示成功接收（ACK），高电平表示无应答（NACK）
 */
static void I2C_Ack(void)
{
    SCL_L;       /* 时钟线置低，允许数据线变化 */
    I2C_delay(); /* 等待电平稳定（满足tLOW时序） */

    SDA_L;       /* 数据线置低（生成ACK信号） */
    I2C_delay(); /* 保持数据稳定（满足tSU:DAT时序） */

    SCL_H;       /* 时钟线置高（主机检测ACK信号） */
    I2C_delay(); /* 维持高电平（满足tHIGH时序） */

    SCL_L;       /* 时钟线恢复低电平（结束ACK周期） */
    I2C_delay(); /* 为下一数据传输周期准备 */
}

/*
 * 函数名：I2C_NoAck
 * 功能：生成I2C非应答信号（NACK）
 * 协议规范：
 *   1. 在I2C协议中，主机在第9个时钟周期保持SDA高电平表示NACK
 *   2. NACK用于以下场景：
 *      - 主机接收完最后一个字节数据
 *      - 从机无法响应时强制终止传输
 * 时序说明：
 *   1. SCL先置低电平（数据线变化安全期）
 *   2. SDA置高电平建立NACK状态
 *   3. SCL置高电平使从机锁存NACK信号
 *   4. SCL恢复低电平完成信号周期

 */
static void I2C_NoAck(void)
{
    SCL_L;       // 时钟线置低，允许数据线变化
    I2C_delay(); // 等待电平稳定（tLOW时钟低周期时间）

    SDA_H;       // 数据线置高（NACK信号核心动作）
    I2C_delay(); // 保持数据稳定（tSU:DAT数据建立时间）

    SCL_H;       // 时钟线置高（从机在此上升沿采样NACK）
    I2C_delay(); // 维持高电平（tHIGH时钟高周期时间）

    SCL_L;       // 时钟线恢复低电平（结束NACK周期）
    I2C_delay(); // 总线空闲准备（tBUF总线空闲时间）
}

/* 函数功能: 等待I2C从设备返回ACK应答信号
 * 返回值  : 返回TRUE(1)表示收到ACK，返回FALSE(0)表示无ACK
 * 执行流程:
 *   1. 拉低时钟线(SCL_L)启动ACK检测流程
 *   2. 释放数据线(SDA_H)使从设备可以控制SDA
 *   3. 拉高时钟线(SCL_H)生成ACK检测时钟脉冲
 *   4. 在时钟高电平期间检测SDA状态：
 *      - 高电平: 从设备无应答(NACK)
 *      - 低电平: 从设备有应答(ACK)
 *   5. 最终保持时钟线低电平(SCL_L)结束ACK检测

 */
static unsigned char I2C_WaitAck(void)
{
    SCL_L;       // 拉低时钟线开始ACK检测
    I2C_delay(); // 保持时序
    SDA_H;       // 释放数据线(切换为输入模式)
    I2C_delay(); // 等待总线稳定
    SCL_H;       // 时钟线上升沿(从设备在此阶段拉低SDA)
    I2C_delay(); // 保持时钟高电平时间

    if (SDA_read) // 检测SDA电平状态
    {             // 高电平 -> 无ACK
        SCL_L;    // 结束ACK检测前拉低时钟
        I2C_delay();
        return FALSE; // 返回无应答状态
    }

    SCL_L; // 收到ACK后保持时钟低电平
    I2C_delay();
    return TRUE; // 返回成功收到ACK
}

/* 函数功能: 通过I2C总线发送单字节数据(MSB优先)
 * 参数     : SendByte - 要发送的字节数据
 * 执行流程:
 *   1. 循环8次依次发送每个bit(从最高位到最低位)
 *   2. 每个bit发送步骤:
 *      a. 拉低SCL进入数据准备阶段
 *      b. 根据当前bit值设置SDA电平(高电平=1,低电平=0)
 *      c. 左移数据准备下一个bit
 *      d. 拉高SCL生成时钟上升沿(数据采样点)
 *      e. 保持时钟高电平满足时序要求
 *   3. 最终保持SCL低电平结束传输

 */
static void I2C_SendByte(unsigned char SendByte)
{
    unsigned char i = 8;
    while (i--) // 循环发送8个bit
    {
        SCL_L;       // 拉低时钟开始数据准备
        I2C_delay(); // 保持低电平时间

        /* 设置数据线电平 */
        if (SendByte & 0x80) // 检测最高位(0x80=10000000b)
            SDA_H;           // 发送逻辑1
        else
            SDA_L; // 发送逻辑0

        SendByte <<= 1; // 左移准备下一个bit(MSB优先)
        I2C_delay();    // 保证数据建立时间

        SCL_H;       // 时钟上升沿(从机在此阶段采样)
        I2C_delay(); // 保持高电平时间
    }
    SCL_L; // 最终保持时钟低电平(总线空闲)
}

/* 函数功能: 通过I2C总线接收单字节数据(MSB优先)
 * 返回值  : 接收到的完整字节数据
 * 执行流程:
 *   1. 初始化数据线为高电平(SDA_H)进入输入模式
 *   2. 循环8次依次接收每个bit(从最高位到最低位)
 *   3. 每个bit接收步骤:
 *      a. 左移接收字节准备存储新bit
 *      b. 拉低SCL_L开始数据采样周期
 *      c. 拉高SCL_H生成时钟上升沿(从机在此阶段输出数据)
 *      d. 在SCL高电平期间读取SDA状态
 *      e. 根据SDA电平设置接收字节最低位
 *   4. 最终保持SCL_L低电平结束传输

 */
static unsigned char I2C_RadeByte(void)
{
    unsigned char i = 8;
    unsigned char ReceiveByte = 0;

    SDA_H;      // 释放数据线(切换为输入模式)
    while (i--) // 循环接收8个bit
    {
        ReceiveByte <<= 1; // 左移腾出最低位(MSB First)

        SCL_L;       // 拉低时钟开始采样周期
        I2C_delay(); // 保持低电平时间
        SCL_H;       // 时钟上升沿(从机输出数据稳定)
        I2C_delay(); // 保持高电平时间

        if (SDA_read) // 检测SDA电平状态
        {
            ReceiveByte |= 0x01; // 最低位置1(小端存储)
        }
    }
    SCL_L;              // 最终拉低时钟结束传输
    return ReceiveByte; // 返回完整接收字节
}

/* 函数功能: 向I2C从设备执行单字节数据写入操作
 * 参数     :
 *   SlaveAddress - 从设备地址(7位地址，不含读写位)
 *   REG_Address  - 目标寄存器地址
 *   REG_data     - 待写入数据
 * 返回值  : TRUE(1)操作成功，FALSE(0)操作失败
 * 执行流程:
 *   1. 发起I2C起始信号
 *   2. 发送从设备地址+写标志位(地址自动左移1位，最低位置0)
 *   3. 发送目标寄存器地址
 *   4. 发送待写入数据
 *   5. 每次发送后检测从设备应答
 *   6. 发送停止信号结束通信
 *   7. 延时5ms确保写入完成

 */
unsigned char Single_Write(unsigned char SlaveAddress,
                           unsigned char REG_Address,
                           unsigned char REG_data)
{
    if (!I2C_Start()) // 发起起始信号
    {
        return FALSE; // 总线占用检测失败
    }

    // 发送设备地址(自动添加写标志位)
    I2C_SendByte(SlaveAddress);
    if (!I2C_WaitAck())
    { // 检测地址ACK
        I2C_Stop();
        return FALSE; // 地址无应答
    }

    I2C_SendByte(REG_Address); // 发送寄存器地址
    I2C_WaitAck();             // 未处理ACK失败(需改进)

    I2C_SendByte(REG_data); // 发送写入数据
    I2C_WaitAck();          // 未处理ACK失败(需改进)

    I2C_Stop();  // 终止通信
    delay5ms();  // 等待数据持久化(EEPROM等需要)
    return TRUE; // 返回操作成功状态
}

/* 函数功能: 从I2C从设备执行单字节数据读取操作
 * 参数     :
 *   SlaveAddress - 从设备地址(7位地址，不含读写位)
 *   REG_Address  - 要读取的目标寄存器地址
 * 返回值  :
 *   成功时返回读取的字节数据(0x00-0xFF)
 *   失败时返回FALSE(0x00)(与数据0冲突，需业务层判断)
 * 执行流程:
 *   1. 发起起始信号 -> 发送设备地址(写模式)
 *   2. 发送寄存器地址 -> 重复起始信号
 *   3. 发送设备地址(读模式) -> 接收数据
 *   4. 发送NACK终止传输 -> 发送停止信号

 */
unsigned char Single_Read(unsigned char SlaveAddress,
                          unsigned char REG_Address)
{
    unsigned char REG_data;
    // 第一阶段：写寄存器地址
    if (!I2C_Start())
        return FALSE;           // 启动总线失败
    I2C_SendByte(SlaveAddress); // 发送设备地址+写标志
    if (!I2C_WaitAck())
    { // 检测地址ACK
        I2C_Stop();
        return FALSE; // 从机无应答
    }
    I2C_SendByte(REG_Address); // 发送目标寄存器地址
    I2C_WaitAck();             // 未处理ACK失败(建议改进)

    // 第二阶段：读取数据
    I2C_Start();                    // 重复起始信号
    I2C_SendByte(SlaveAddress + 1); // 设备地址+读标志(LSB=1)
    I2C_WaitAck();

    REG_data = I2C_RadeByte(); // 读取数据字节
    I2C_NoAck();               // 发送NACK终止读取
    I2C_Stop();                // 释放总线

    return REG_data; // 返回读取结果
}

/* 函数功能: 向I2C从设备执行多字节连续写入操作
 * 参数     :
 *   SlaveAddress - 从设备地址(7位地址，不含读写位)
 *   REG_Address  - 起始寄存器地址
 *   REG_data     - 待写入数据缓冲区指针
 *   num          - 要写入的字节数
 * 返回值  : TRUE(1)操作成功，FALSE(0)操作失败
 * 执行流程:
 *   1. 发起起始信号 -> 发送设备地址(写模式)
 *   2. 发送寄存器起始地址
 *   3. 循环发送数据缓冲区内容
 *   4. 每个字节发送后检测ACK
 *   5. 发送停止信号并延时5ms

 */
unsigned char IIC_Mult_Write(unsigned char SlaveAddress,
                             unsigned char REG_Address,
                             unsigned char *REG_data,
                             unsigned char num)
{
    unsigned char i = 0;
    // 启动传输序列
    if (!I2C_Start())
        return FALSE;           // 总线启动检测
    I2C_SendByte(SlaveAddress); // 发送设备地址+写标志
    if (!I2C_WaitAck())
    { // 检测地址ACK
        I2C_Stop();
        return FALSE;
    }
    I2C_SendByte(REG_Address); // 发送寄存器起始地址
    I2C_WaitAck();             // 未处理ACK失败(需改进)

    // 循环发送数据
    do
    {
        I2C_SendByte(REG_data[i]); // 发送数据字节
        I2C_WaitAck();             // 未处理ACK失败(数据可能丢失)
        i++;
        num--;
    } while (num); // 直到发送完指定字节数

    I2C_Stop();  // 终止通信
    delay5ms();  // 等待存储完成(EEPROM等需要)
    return TRUE; // 返回操作状态
}

/* 函数功能: 从I2C从设备连续读取多个字节数据
 * 参数     :
 *   SlaveAddress - 7位从设备地址(不含读写位)
 *   REG_Address  - 目标起始寄存器地址
 *   REG_data     - 数据接收缓冲区指针
 *   num          - 要读取的字节数(≥1)
 * 返回值  : TRUE(1)操作成功，FALSE(0)操作失败
 * 执行流程:
 *   1. 写相位阶段:
 *     a. 发送起始信号
 *     b. 发送设备地址+写标志
 *     c. 发送目标寄存器地址
 *   2. 读相位阶段:
 *     a. 重复起始信号
 *     b. 发送设备地址+读标志
 *     c. 循环接收数据:
 *       - 前N-1个字节发送ACK继续读取
 *       - 最后1个字节发送NACK终止读取
 *   3. 发送停止信号释放总线

 */
unsigned char IIC_Mult_Read(unsigned char SlaveAddress,
                            unsigned char REG_Address,
                            unsigned char *REG_data,
                            unsigned char num)
{
    unsigned char i = 0;
    // 第一阶段：写寄存器地址
    if (!I2C_Start())
        return FALSE;           // 起始信号失败
    I2C_SendByte(SlaveAddress); // 设备地址+写标志(LSB=0)
    if (!I2C_WaitAck())
    { // 地址ACK检测
        I2C_Stop();
        return FALSE;
    }
    I2C_SendByte(REG_Address); // 发送寄存器起始地址
    I2C_WaitAck();             // 未处理ACK失败(建议改进)

    // 第二阶段：读取数据流
    I2C_Start();                    // 重复起始信号
    I2C_SendByte(SlaveAddress + 1); // 设备地址+读标志(LSB=1)
    I2C_WaitAck();                  // 未处理ACK失败(建议改进)

    // 循环接收数据字节
    num--; // 调整计数器(N-1个ACK)
    while (num--)
    {
        REG_data[i] = I2C_RadeByte(); // 读取字节存入缓冲区
        i++;
        I2C_Ack(); // 发送ACK继续读取
    }
    REG_data[i] = I2C_RadeByte(); // 读取最后一个字节
    I2C_NoAck();                  // 发送NACK终止读取
    I2C_Stop();                   // 释放总线

    return TRUE; // 返回操作成功
}

/* 1ms延时函数(近似值，具体时长需根据主频校准) */
void Delay_1ms(unsigned int Del_1ms)
{
    unsigned char j;
    while (Del_1ms--) // 外层循环控制总延时毫秒数
    {
        for (j = 0; j < 123; j++)
            ; // 内层循环实现约1ms延时（11.0592MHz典型值）
    }
}

/* OLED I2C操作状态标志
 * 0:失败 1:成功 (由Single_Write返回值更新) */
unsigned char OLED_IIC_stu = 0;

/* OLED单字节写入函数
 * dat : 要写入的数据/命令
 * cmd : 数据类型选择
 *       1=数据模式(0X40)
 *       0=命令模式(0X00) */
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
    if (cmd) // 数据写入模式
    {
        OLED_IIC_stu = Single_Write(0x78, 0X40, dat); // 0x78为OLED器件地址
    }
    else // 命令写入模式
    {
        OLED_IIC_stu = Single_Write(0x78, 0X00, dat); // 0x00为命令寄存器地址
    }
}

/* 全屏填充函数
 * fill_Data : 填充模式(0x00全灭/0xFF全亮/0xAA棋盘格等) */
void fill_picture(unsigned char fill_Data)
{
    unsigned char m, n;
    for (m = 0; m < 8; m++) // 遍历8个页（OLED每页对应8行）
    {
        OLED_WR_Byte(0xb0 + m, 0); // 设置页地址：0xB0~0xB7
        OLED_WR_Byte(0x00, 0);     // 列地址低4位：0x00
        OLED_WR_Byte(0x10, 0);     // 列地址高4位：0x10（组合后从0列开始）
        for (n = 0; n < 128; n++)  // 遍历128列
        {
            OLED_WR_Byte(fill_Data, 1); // 持续写入填充数据
        }
    }
}

/* 坐标定位函数
 * x : 列地址(0-127)
 * y : 页地址(0-7)
 * 注意：OLED纵向以页为单位，每页8个像素行 */
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);                 // 设置页地址（0xB0+y）
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD); // 列地址高4位（0x10|高半字节）
    OLED_WR_Byte((x & 0x0f), OLED_CMD);               // 列地址低4位（0x00|低半字节）
}

/////
// 开启OLED显示
void oled_Display_On(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X14, OLED_CMD); // DCDC ON
    OLED_WR_Byte(0XAF, OLED_CMD); // DISPLAY ON
}

// 关闭OLED显示
void oled_Display_Off(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X10, OLED_CMD); // DCDC OFF
    OLED_WR_Byte(0XAE, OLED_CMD); // DISPLAY OFF
}

// 清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void oled_Clear(void)
{
    unsigned char i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD); // 设置页地址（0~7）
        OLED_WR_Byte(0x00, OLED_CMD);     // 设置显示位置—列低地址
        OLED_WR_Byte(0x10, OLED_CMD);     // 设置显示位置—列高地址
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(0, OLED_DATA);
    } // 更新显示
}

void oled_On(void)
{
    unsigned char i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD); // 设置页地址（0~7）
        OLED_WR_Byte(0x00, OLED_CMD);     // 设置显示位置—列低地址
        OLED_WR_Byte(0x10, OLED_CMD);     // 设置显示位置—列高地址
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(1, OLED_DATA);
    } // 更新显示
}

/* 函数功能: 在OLED指定位置显示单个字符
 * 参数说明:
 *   x,y       : 显示起始坐标(x:0-127列, y:0-7页)
 *   chr       : 要显示的ASCII字符(实际支持范围视字库而定)
 *   Char_Size : 字体大小选择(16=16x8点阵, 其他值=6x8点阵)
 * 实现细节:
 *   - 字符显示基于预存字模数据(F8X16/F6x8数组)
 *   - 16px字体分两次写入，上半页和下半页各8行数据
 *   - 自动换行处理: x超界时复位到0列并下移2页(16px字体适用)
 * 注意事项:
 *   - 字库数组需包含从空格字符(ASCII 32)开始的字形数据
 *   - 页地址y参数实际对应OLED的8像素行区块 */
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' '; // 计算字库偏移量(空格字符为字库起点)

    /* 坐标超界处理(16px字体自动换行) */
    if (x > Max_Column - 1)
    {
        x = 0;
        y = y + 2; // 16px字体占2页高度，换行时增加2页
    }

    if (Char_Size == 16) // 16x8点阵字体处理
    {
        /* 写入上半页数据(前8行) */
        OLED_Set_Pos(x, y);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA); // 索引计算: 每个字符16字节

        /* 写入下半页数据(后8行) */
        OLED_Set_Pos(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
    }
    else // 6x8点阵字体处理
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA); // 每个字符6字节
    }
}

/* 函数功能: 简易整数幂计算(m的n次方)
 * 参数说明:
 *   m : 底数(建议范围0-65535)
 *   n : 指数(建议范围0-16, 防止结果溢出)
 * 返回值  : m^n计算结果(无符号整型)
 * 注意事项:
 *   - 仅适用于非负整数运算
 *   - 无溢出保护，需确保结果在unsigned int范围内
 *   - 典型应用场景: OLED显示数值时的位权计算 */
unsigned int oled_pow(unsigned char m, unsigned char n)
{
    unsigned int result = 1;
    while (n--)
        result *= m; // 通过累乘实现幂运算
    return result;
}

/* 函数功能: 多位数字显示(支持自动去前导零)
 * 参数说明:
 *   x,y    : 起始坐标(x:列,y:页)
 *   num    : 待显示数值(支持0~4294967295)
 *   len    : 显示位数(固定位数显示)
 *   size2  : 字体大小(16/8 对应不同字库)
 * 实现特性:
 *   - 自动跳过前导零显示(保留最后一位零)
 *   - 数字间距自动计算(size2/2)
 *   - 支持超过显示位数时截断高位 */
void oled_ShowNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char size2)
{
    unsigned char t, temp;
    unsigned char enshow = 0; // 前导零标志(0:未遇到有效数字)
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10; // 提取指定位的数字
        /* 前导零处理逻辑 */
        if (enshow == 0 && t < (len - 1)) // 允许最后一位显示零
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2); // 用空格替代前导零
                continue;
            }
            else
                enshow = 1; // 遇到非零数字后开始显示
        }
        OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2); // 数字转ASCII码显示
    }
}

/* 函数功能: 字符串显示(支持自动换行)
 * 参数说明:
 *   chr        : 字符串指针(需以'\0'结尾)
 *   Char_Size  : 字体大小(16/8)
 * 显示特性:
 *   - 字符间距固定8像素(适用于8px宽字体)
 *   - x>120时换行到下一行首列
 *   - 换行时y增加2页(适配16px高度字体) */
void oled_ShowString(unsigned char x, unsigned char y, unsigned char *chr, unsigned char Char_Size)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8; // 步进8列(标准ASCII字符宽度)
        /* 边界换行处理 */
        if (x > 120)
        {
            x = 0;
            y += 2;
        } // 预留8列余量防止溢出
        j++;
    }
}

/* 函数功能: 16x16点阵汉字显示(双页模式)
 * 参数说明:
 *   x : 起始列坐标(0-127)
 *   y : 起始页坐标(0-6) 每个汉字占2页高度
 *   no: 汉字在字库中的索引号(0~n)
 * 字库要求:
 *   Hzk[][16]二维数组，每个汉字占32字节：
 *   - [2*no]   为汉字上半部分数据(前16字节)
 *   - [2*no+1] 为汉字下半部分数据(后16字节)
 * 执行流程:
 *   1. 定位到指定页的起始列
 *   2. 写入前16字节到上半页(y)
 *   3. 定位到下一页(y+1)
 *   4. 写入后16字节到下半页

 *     */
void oled_ShowCHinese(unsigned char x, unsigned char y, unsigned char no)
{
    unsigned char t, adder = 0; // adder为无效变量(历史遗留)

    /* 写入汉字上半部分(8像素行) */
    OLED_Set_Pos(x, y);      // 设置起始位置(列x,页y)
    for (t = 0; t < 16; t++) // 遍历16列数据
    {
        OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA); // 发送字库上半部数据
        adder += 1;                              // 无效操作(可删除)
    }

    /* 写入汉字下半部分(8像素行) */
    OLED_Set_Pos(x, y + 1);  // 跳转到下一页(y+1)
    for (t = 0; t < 16; t++) // 遍历16列数据
    {
        OLED_WR_Byte(Hzk[2 * no + 1][t], OLED_DATA); // 发送字库下半部数据
        adder += 1;                                  // 无效操作(可删除)
    }
}

/* 函数功能: 显示十进制整型数值(带简单格式化)
 * 参数说明:
 *   x,y    : 显示起始坐标
 *   Dnum   : 要显示的整型数值(-999~9999)
 *   size1  : 字体大小(8/16)
 * 实现特性:
 *   - 使用%3d格式保证至少显示3位数字(不足补空格)
 *   - Data[5]=0强制截断为5字符(含结束符)

 *    */
void OLED_ShowDNum(unsigned char x, unsigned char y, int Dnum, unsigned char size1)
{
    uint8_t Data[20] = " ";             // 格式化缓冲区(实际有效长度被限制为4字符+结束符)
    sprintf((char *)Data, "%3d", Dnum); // 格式化为至少3位十进制数
    Data[5] = 0;                        // 强制终止字符串(防止长数值溢出)
    oled_ShowString(x, y, Data, size1);
}

/* 函数功能: 显示浮点数值(保留1位小数)
 * 参数说明:
 *   Fnum   : 要显示的浮点数值(-999.9~9999.9)
 * 实现特性:
 *   - 使用%.1f格式保留1位小数
 *   - Data[5]=0强制截断为5字符(含结束符)

 *  */
void OLED_ShowFNum(unsigned char x, unsigned char y, float Fnum, unsigned char size1)
{
    uint8_t Data[20] = " ";              // 格式化缓冲区(实际有效长度被限制为5字符+结束符)
    sprintf((char *)Data, "%10.6f", Fnum); // 保留6位小数格式化
    Data[10] = 0;                         // 强制终止字符串(示例"123.4"占5字节)
    oled_ShowString(x, y, Data, size1);
}

/* 函数功能: OLED位图绘制(存在潜在坐标计算问题)
 * 参数说明:
 *   x0,y0 : 起始列坐标(0-127)和起始页坐标(0-7)
 *   x1,y1 : 结束列坐标(应大于x0)和结束像素行坐标(0-63)
 *   BMP[] : 位图数据数组(按纵向分页存储，每页包含x1-x0个列数据)
 * 实现流程:
 *   1. 计算结束像素行对应的总页数：y = y1/8 + 余数处理
 *   2. 纵向逐页遍历(y0到计算得到的y值)
 *   3. 每页内横向逐列写入数据(x0到x1)
 *   4. 数据按列顺序连续存储，每列1字节(8像素行)

 *    */
void oled_DrawBMP(unsigned char x0, unsigned char y0,
                  unsigned char x1, unsigned char y1,
                  unsigned char BMP[])
{
    unsigned int j = 0; // 数据缓冲区索引
    unsigned char x, y; // 列/页循环变量

    /* 页数计算 */
    if (y1 % 8 == 0)
        y = y1 / 8; // 完美对齐时直接除
    else
        y = y1 / 8 + 1; // 未对齐时增加1页

    /* 纵向页遍历  */
    for (y = y0; y < y1; y++) // y0作为起始页，y1作为结束页
    {
        OLED_Set_Pos(x0, y); // 设置当前页起始列
        /* 横向列遍历 */
        for (x = x0; x < x1; x++)
        {
            OLED_WR_Byte(BMP[j++], OLED_DATA); // 写入列数据
        }
    }
}
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  //
/* 函数功能: SSD1306 OLED显示屏初始化
 * 执行流程:
 *   1. GPIO初始化: 配置PA11(SDA),PA12(SCL)为推挽输出
 *   2. 发送初始化命令序列配置显示参数
 *   3. 清屏并开启显示

 *    */
void oled_Init(void)
{
    /* GPIO初始化阶段 */
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 使能GPIOA时钟

    /* SCL(PA12)配置 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 高速模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;        // SCL引脚
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* SDA(PA11)配置 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // SDA引脚
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 总线初始状态置高 */
    GPIO_SetBits(GPIOA, GPIO_Pin_12); // SCL高电平
    GPIO_SetBits(GPIOA, GPIO_Pin_11); // SDA高电平

    /* 设备初始化阶段 */
    while (!Single_Write(0x78, 0X00, 0xae))
        ;           // 持续尝试关闭显示直到成功
    Delay_1ms(400); // 关键延时确保硬件稳定

    /* SSD1306配置命令序列 */
    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭显示
    OLED_WR_Byte(0x00, OLED_CMD); // 设置列地址低4位起始为0
    OLED_WR_Byte(0x10, OLED_CMD); // 设置列地址高4位起始为0
    OLED_WR_Byte(0x40, OLED_CMD); // 设置显示起始行地址(行0)
    OLED_WR_Byte(0xB0, OLED_CMD); // 设置页地址起始页(页0)
    OLED_WR_Byte(0x81, OLED_CMD); // 对比度控制模式
    OLED_WR_Byte(0xFF, OLED_CMD); // 对比度最大值255
    OLED_WR_Byte(0xA1, OLED_CMD); // 段重映射(水平翻转)
    OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示模式(非反色)
    OLED_WR_Byte(0xA8, OLED_CMD); // 设置复用比率
    OLED_WR_Byte(0x3F, OLED_CMD); // 1/64占空比(适用于128x64屏)
    OLED_WR_Byte(0xC8, OLED_CMD); // COM输出扫描方向(逆向)
    OLED_WR_Byte(0xD3, OLED_CMD); // 显示偏移设置
    OLED_WR_Byte(0x00, OLED_CMD); // 无偏移
    OLED_WR_Byte(0xD5, OLED_CMD); // 显示时钟分频
    OLED_WR_Byte(0x80, OLED_CMD); // 建议默认值(分频比1)
    OLED_WR_Byte(0xD8, OLED_CMD); // 区域颜色模式
    OLED_WR_Byte(0x05, OLED_CMD); // 禁用区域颜色模式
    OLED_WR_Byte(0xD9, OLED_CMD); // 预充电周期设置
    OLED_WR_Byte(0xF1, OLED_CMD); // Phase1=15 DCLK, Phase2=1 DCLK
    OLED_WR_Byte(0xDA, OLED_CMD); // COM引脚硬件配置
    OLED_WR_Byte(0x12, OLED_CMD); // 顺序COM引脚，禁用左右COM偏移
    OLED_WR_Byte(0xDB, OLED_CMD); // VCOMH电压等级
    OLED_WR_Byte(0x30, OLED_CMD); // 约0.83*VCC
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵设置
    OLED_WR_Byte(0x14, OLED_CMD); // 使能电荷泵(必须开启)
    OLED_WR_Byte(0xAF, OLED_CMD); // 开启显示

    oled_Clear(); // 清屏初始化显示缓存
}

/* 函数功能: 显示颜色反相控制
 * 参数说明:
 *   i - 显示模式选择
 *       0: 正常显示(黑底白字)
 *       1: 反色显示(白底黑字)
 * 实现原理:
 *   通过发送SSD1306的A6/A7命令切换全局颜色映射

 *    */
void oled_ColorTurn(uint8_t i)
{
    if (i == 0)
    {
        OLED_WR_Byte(0xA6, OLED_CMD); // 0xA6: 正常像素模式(GDDRAM数据1表示亮)
    }
    if (i == 1)
    {
        OLED_WR_Byte(0xA7, OLED_CMD); // 0xA7: 反色像素模式(GDDRAM数据0表示亮)
    }
}

/* 函数功能: 屏幕显示方向旋转控制
 * 参数说明:
 *   i - 旋转模式选择
 *       0: 正常方向显示
 *       1: 180度旋转显示
 * 实现原理:
 *   组合使用段重映射(A0/A1)和COM扫描方向(C0/C8)命令实现硬件级旋转
 * 硬件配置:
 *   - 0xC8: COM输出逆向扫描(从COM63到COM0)
 *   - 0xA1: 段重映射(列地址127映射到SEG0)
 *   - 0xC0: COM输出正常扫描(从COM0到COM63)
 *   - 0xA0: 段正常映射(列地址0映射到SEG0)

 *   */
void oled_DisplayTurn(uint8_t i)
{
    if (i == 0) // 标准方向
    {
        OLED_WR_Byte(0xC8, OLED_CMD); // COM逆向扫描(配合物理布局)
        OLED_WR_Byte(0xA1, OLED_CMD); // 列地址反向(SEG127->SEG0)
    }
    if (i == 1) // 旋转180度
    {
        OLED_WR_Byte(0xC0, OLED_CMD); // COM正向扫描
        OLED_WR_Byte(0xA0, OLED_CMD); // 列地址正常映射
    }
}
