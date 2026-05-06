[详细链接](https://github.com/kali-suroot/car)

# STM32 汽车防盗监测系统 —— 超详细代码说明书

> **版本**：V1.0  
> **适用对象**：嵌入式初学者、单片机爱好者、课程设计/毕业设计参考  
> **阅读建议**：按章节顺序阅读，每个章节都包含完整代码和逐行注释  
> **目标**：让高中生都能看懂每一行代码的作用

---

## 目录

- [第一章：项目概述与系统架构](#第一章项目概述与系统架构)
- [第二章：STM32 核心系统代码详解](#第二章stm32-核心系统代码详解)
- [第三章：通信模块代码详解（串口WiFiMQTT）](#第三章通信模块代码详解)
- [第四章：外设驱动代码详解（OLEDGPS蜂鸣器等）](#第四章外设驱动代码详解)
- [第五章：Android APP 代码详解](#第五章android-app-代码详解)
- [第六章：配置文件与中断处理详解](#第六章配置文件与中断处理详解)
- [第七章：项目使用指南](#第七章项目使用指南)
- [附录：文件清单与学习路线](#附录)

---

## 第一章：项目概述与系统架构

### 1.1 这是个什么项目？

这是一个基于 **STM32F103C8T6** 单片机的**智能汽车防盗监测系统**。你可以把它理解为给汽车装了一个"智能保镖"——它能感知危险、发出警报，还能通过手机APP远程告诉你"你的车有状况"。

#### 核心功能一览

| 功能模块 | 通俗说明 | 技术实现 |
|---------|---------|---------|
| **震动检测** | 有人拍打车窗或碰撞车身，系统立刻感知 | SW-18010P 振动传感器 |
| **GPS定位** | 随时知道车在哪里，精确到米级 | ATGM336H 北斗/GPS双模模块 |
| **OLED显示** | 一个小屏幕显示当前状态、坐标等信息 | 0.96寸 I2C接口OLED屏 |
| **蜂鸣器报警** | 检测到非法入侵时"嘀嘀嘀"响 | 有源蜂鸣器 |
| **LED指示灯** | 用不同闪烁频率表示系统状态 | 3个LED灯（红/绿/蓝） |
| **按键控制** | 4个按钮切换显示页面和操作模式 | 机械按键+软件消抖 |
| **WiFi通信** | 通过无线把数据传到云端 | ESP-01S WiFi模块 |
| **MQTT远程通信** | 手机和车之间通过互联网对话 | MQTT协议+云服务器 |
| **手机APP监控** | 在手机上看到车的状态和位置 | Android APP |
| **Flash存储** | 断电后记住配置信息 | STM32内部Flash |

### 1.2 系统架构图（整体框图）

想象一下这个系统的组成部分就像人的身体：

```
                    +------------------------------------------+
                    |           云平台（MQTT服务器）              |
                    |         地址: 47.109.89.8:1883            |
                    +---------------------+--------------------+
                                          |
                        WiFi/互联网       |      WiFi/互联网
                                          |
    +------------------------+   MQTT    +------------------------+
    |    Android 手机 APP     |<-------->|   STM32F103C8T6 单片机   |
    |  - 显示GPS坐标          |  协议    |   （系统大脑/CPU）        |
    |  - 开关防盗模式          |          |                        |
    |  - 接收报警通知          |          |  +------------------+   |
    |  - 远程监控             |          |  |   各个外设模块    |   |
    +------------------------+          |  |                  |   |
                                        |  | - 震动传感器(PA0)|   |
                                        |  | - GPS模块(USART1)|   |
                                        |  | - OLED屏(I2C)    |   |
                                        |  | - 蜂鸣器(PB8)    |   |
                                        |  | - LED灯(PA1/4/5) |   |
                                        |  | - 按键(PA6/7/8/9)|   |
                                        |  | - WiFi(USART2)   |   |
                                        |  +------------------+   |
                                        +------------------------+
                                                      |
                                            5V电源供电 (1A~2A)
```

### 1.3 硬件实物图说明

```
    [STM32F103C8T6 最小系统板]
           │
    ┌──────┴──────────────────────────┐
    │  ┌─────────┐  ┌────┐  ┌───────┐ │
    │  │ OLED屏  │  │LED │  │蜂鸣器 │ │
    │  │ 128x64  │  │x3  │  │       │ │
    │  └─────────┘  └────┘  └───────┘ │
    │  ┌────┐ ┌────┐ ┌──────┐        │
    │  │按键│ │GPS │ │WiFi  │        │
    │  │x4  │ │模块│ │ESP826│        │
    │  └────┘ └────┘ └──────┘        │
    │  ┌──────┐                       │
    │  │振动  │  + 各类电阻电容        │
    │  │传感器│                       │
    │  └──────┘                       │
    └─────────────────────────────────┘
```

### 1.4 数据流图（信息怎么流动的？）

```
    振动传感器 ──→ STM32 ──→ MQTT服务器 ──→ 手机APP
    GPS模块    ──→         │
    按键       ──→         │
                           ↓
                      OLED显示（本地显示）
                           ↓
                      蜂鸣器+LED（本地报警）
```

### 1.5 核心工作原理

整个系统的工作流程就像一个"智能保安值班室"：

1. **开机自检**（main.c）：STM32启动后，先初始化所有外设（屏幕、传感器、WiFi等），就像保安上班先检查设备
2. **正常工作模式**（menu.c）：系统循环检测按键状态，更新屏幕显示，等待用户操作
3. **防盗模式**（menu.c）：用户开启防盗后，STM32持续监控振动传感器。一旦检测到震动，立即触发报警
4. **报警流程**：
   - 蜂鸣器响（"嘀嘀嘀"）
   - LED灯快速闪烁（红色）
   - 通过WiFi发送MQTT消息到云端
   - 云端转发到用户手机APP
   - APP弹出报警通知，手机自身也震动
5. **GPS定位**：定时读取GPS模块的经纬度数据，显示在OLED屏幕上，同时通过MQTT上传到APP

### 1.6 技术栈一览

| 层级 | 技术/工具 | 说明 |
|------|----------|------|
| **硬件平台** | STM32F103C8T6 | ARM Cortex-M3 内核，72MHz |
| **开发环境** | Keil MDK-ARM | 嵌入式C语言开发IDE |
| **固件库** | STM32F10x_StdPeriph_Driver V3.5.0 | ST官方标准外设库 |
| **编程语言** | C语言 | 嵌入式开发主流语言 |
| **通信协议** | USART / I2C / MQTT | 串口、I2C总线、MQTT物联网协议 |
| **无线网络** | ESP8266 WiFi模块 | AT指令控制 |
| **手机端** | Android Java | Android Studio开发 |
| **云服务** | EMQ X MQTT Broker | 开源MQTT服务器 |

### 1.7 完整文件清单

本项目共包含以下模块（按功能分类）：

```
STM32 单片机端代码（C语言）
│
├── 系统核心层 (SYS/)
│   ├── main.c              ← 程序入口，主循环
│   ├── sys.c / sys.h       ← 系统配置、全局变量、宏定义
│   ├── menu.c / menu.h     ← 菜单系统、防盗逻辑核心
│   ├── time2.c / time2.h   ← 定时器2驱动
│   ├── my_usart1.c/.h      ← 串口1驱动（连接GPS）
│   ├── my_usart2.c/.h      ← 串口2驱动（连接WiFi）
│   └── BridgeFlash.c/.h    ← Flash存储管理
│
├── 外设驱动层 (User/)
│   ├── delay.c / delay.h   ← 延时函数
│   ├── led.c / led.h       ← LED指示灯驱动
│   ├── key.c / key.h       ← 按键驱动
│   ├── bsp_beep.c/.h       ← 蜂鸣器驱动
│   ├── bsp_oled_iic.c/.h   ← OLED显示屏驱动
│   ├── oledFont.h          ← OLED字体数据
│   ├── shake.c / shake.h   ← 振动传感器驱动
│   ├── gps.c / gps.h       ← GPS模块驱动
│   ├── esp8266_drv.c/.h    ← ESP8266 WiFi驱动
│   ├── mqtt_drv.c / .h     ← MQTT协议实现
│   ├── stm32f10x_conf.h    ← 外设配置文件
│   ├── stm32f10x_it.c/.h   ← 中断处理
│   └── stm32f10x.h 等库文件  ← ST官方库
│
└── Android APP端代码（Java）
    ├── MainActivity.java     ← APP主界面和逻辑
    ├── activity_main.xml     ← 主界面布局
    ├── dialog_login.xml      ← 登录对话框布局
    ├── AndroidManifest.xml   ← APP清单文件
    └── build.gradle          ← 构建配置
```

---



---

## 第二章：STM32 核心系统代码详解

> 本章是整个项目的"心脏"部分。你将学习到：STM32如何启动、系统时钟如何配置、菜单系统如何工作、定时器如何实现精准定时、Flash如何保存配置数据。每一行代码都有详尽的中文注释。

---

## 项目整体架构概述

这是一个基于STM32F103的汽车防盗系统，主要功能包括：
- **GPS定位**：通过USART1获取GPS经纬度数据
- **震动检测**：使用震动传感器检测车辆异常震动
- **OLED显示**：显示防盗状态、经纬度、MQTT连接状态等信息
- **MQTT通信**：通过ESP8266 WiFi模块连接MQTT服务器，实现远程监控
- **按键控制**：4个按键用于切换显示页面和操作模式
- **LED指示灯**：显示系统运行状态（常亮/慢闪/快闪）
- **蜂鸣器报警**：检测到非法入侵时发出警报
- **Flash存储**：保存MQTT设备名称等配置参数

---
### 文件1：sys.h（系统核心头文件）

#### 一、文件整体功能说明

`sys.h`是整个项目的"大脑"，它负责：
1. 引入所有需要的标准库和驱动头文件
2. 定义直接操作GPIO寄存器的快捷宏
3. 实现位带操作（Bit-Banding）技术，让单个GPIO的控制像控制变量一样简单
4. 定义两个核心数据结构：`SENSOR`（传感器数据）和`SYSTEM`（系统状态）
5. 声明全局变量和NVIC配置函数

可以把`sys.h`理解为一份"全局地图"，告诉编译器项目中有哪些模块、哪些资源可用。

---

#### 二、头文件保护（Header Guard）

```c
#ifndef __SYS_H      // 如果没有定义 __SYS_H 这个宏
#define __SYS_H      // 那就定义它
// ... 所有内容 ...
#endif               // 结束条件编译
```

**通俗解释**：这是C语言头文件的"守门员"。想象一下，多个.c文件都可能包含`#include "sys.h"`，如果没有这个保护，编译器就会重复处理同一个文件，导致报错。这就像食堂打饭，没有这个保护，同一个人可能被重复打饭多次。

---

#### 三、标准库头文件引入

```c
#include "math.h"      // 数学函数库，提供sin、cos、sqrt等数学运算
#include "stdio.h"     // 标准输入输出库，提供printf、sprintf等函数
#include "string.h"    // 字符串处理库，提供memcpy、strcpy、memset等函数
```

**逐行解释**：
- `math.h`：当你的程序需要做数学计算（比如三角函数、开平方）时就需要它。虽然本项目可能用得不多，但留着备用。
- `stdio.h`：这是C语言最基础的库，printf函数就是从这里来的。STM32中通常通过串口重定向来实现printf输出到串口助手。
- `string.h`：处理内存和字符串的"工具箱"。比如`memcpy`可以把一块内存的数据复制到另一块，`memset`可以把一块内存全部清零。

---

#### 四、STM32固件库头文件

```c
#include "stm32f10x.h"   // STM32F10x系列的标准外设库头文件
```

**通俗解释**：这是STM32的"操作手册"。里面定义了所有寄存器的地址、结构体、常量等。没有这个头文件，你就没法使用STM32的任何硬件功能（GPIO、定时器、串口等）。它相当于STM32芯片的"字典"，告诉程序每个硬件寄存器在哪里、怎么用。

---

#### 五、各模块驱动头文件引入

```c
#include "./delay/delay.h"           // 延时函数驱动（基于SysTick系统滴答定时器）
#include "stdarg.h"                  // 可变参数列表支持（用于printf-like函数）
#include "./time/time2.h"            // 定时器TIM2驱动
#include "./led/led.h"               // LED灯驱动
#include "./key/key.h"               // 按键扫描驱动
#include "./shake/shake.h"           // 震动传感器驱动
#include "./flash/BridgeFlash.h"     // Flash存储驱动
#include "./beep/bsp_beep.h"         // 蜂鸣器驱动
#include "./my_usart/my_usart1.h"    // 串口1驱动（用于GPS模块）
#include "./my_usart/my_usart2.h"    // 串口2驱动（用于ESP8266 WiFi模块）
#include "./GPS/gps.h"               // GPS数据解析驱动
#include "./mqttwifi/esp8266_drv.h"  // ESP8266 WiFi模块驱动
#include "./mqttwifi/mqtt_drv.h"     // MQTT协议驱动
#include "./oled/bsp_oled_iic.h"     // OLED显示屏IIC驱动
#include "./oled/oledFont.h"         // OLED中文字体库
#include "./menu/menu.h"             // 菜单和显示逻辑
```

**通俗解释**：这些就像组装电脑时需要准备的各种配件的说明书。每个`.h`文件对应一个硬件模块的"使用说明"：
- `delay.h`：延时函数，比如"等100毫秒"
- `time2.h`：定时器2，像是一个"闹钟"，每隔一段时间触发一次
- `led.h`：LED灯的控制，开灯、关灯、闪烁
- `key.h`：按键检测，判断哪个键被按下
- `shake.h`：震动传感器，检测车辆是否被碰撞或移动
- `BridgeFlash.h`：Flash存储，断电后数据不丢失的"硬盘"
- `bsp_beep.h`：蜂鸣器，"滴滴"报警
- `my_usart1.h` / `my_usart2.h`：串口通信，GPS和WiFi都用这个
- `gps.h`：解析GPS返回的经纬度数据
- `esp8266_drv.h` / `mqtt_drv.h`：WiFi和MQTT，让设备连上互联网
- `bsp_oled_iic.h` / `oledFont.h`：OLED显示屏，显示文字和数字
- `menu.h`：菜单逻辑，决定屏幕上显示什么内容

---

#### 六、GPIO快捷操作宏定义

```c
/* 直接操作寄存器的方法控制IO */
#define digitalHi(p, i) \     // 将引脚p的第i位设为高电平（1）
    {                   \
        p->BSRR = i;    \     // BSRR = Bit Set Reset Register（位设置/复位寄存器）
    } // 输出为高电平

#define digitalLo(p, i) \     // 将引脚p的第i位设为低电平（0）
    {                   \
        p->BRR = i;     \     // BRR = Bit Reset Register（位复位寄存器）
    } // 输出低电平

#define digitalToggle(p, i) \  // 将引脚p的第i位翻转（1变0，0变1）
    {                       \
        p->ODR ^= i;        \  // ODR = Output Data Register（输出数据寄存器），^=是按位异或
    } // 输出反转状态
```

**逐行解释**：

这三个宏是操作GPIO引脚的"快捷键"。STM32的每个GPIO端口（A、B、C等）都有多个寄存器来控制它们。这里用了三个特殊的寄存器：

- `BSRR`（Bit Set Reset Register）：**置位寄存器**。往这里面写1，对应的引脚就变成高电平（3.3V）。这是"开灯"操作。为什么用它而不是直接写ODR？因为BSRR是"只写"的，不会误改其他位，更安全。
- `BRR`（Bit Reset Register）：**复位寄存器**。往这里面写1，对应的引脚就变成低电平（0V）。这是"关灯"操作。
- `ODR`（Output Data Register）：**输出数据寄存器**。记录当前所有引脚的输出状态。`^=`是按位异或操作，`1^1=0, 0^1=1`，所以和1异或就会翻转状态。

**参数说明**：
- `p`：GPIO端口指针，比如`GPIOA`、`GPIOB`、`GPIOC`
- `i`：引脚号，比如`GPIO_Pin_5`（就是第5号引脚）

**使用示例**：
```c
digitalHi(GPIOA, GPIO_Pin_5);    // PA5输出高电平（LED亮）
digitalLo(GPIOA, GPIO_Pin_5);    // PA5输出低电平（LED灭）
digitalToggle(GPIOA, GPIO_Pin_5); // PA5状态翻转（亮变灭，灭变亮）
```

---

#### 七、位带操作（Bit-Banding）宏定义

```c
#define BITBAND(addr, bitnum) ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
#define MEM_ADDR(addr) *((volatile unsigned long *)(addr))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))
```

**通俗解释**：位带操作是STM32的"黑科技"。

正常来说，STM32的寄存器是按32位（4字节）操作的。如果你想改某个寄存器的第3位，需要先读出整个32位，修改第3位，再写回去。这样很麻烦，而且不是"原子操作"（可能被中断打断）。

位带技术的神奇之处在于：**它把每一个比特位都映射到了一个独立的32位地址**。你只需要往那个地址写0或1，就能直接控制那一个比特位！

计算公式（上面的BITBAND宏）：
1. `addr & 0xF0000000`：提取地址的最高4位，判断是SRAM区域（0x20000000）还是外设区域（0x40000000）
2. `+ 0x2000000`：加上位带别名区的偏移量
3. `(addr & 0xFFFFF) << 5`：取出地址的低20位，左移5位（因为一个32位字有32个位，每个位占用4字节，所以是2^5=32倍）
4. `(bitnum << 2)`：位号左移2位（每个位的别名地址占4字节）

`MEM_ADDR`宏把计算出来的地址转换成可以被C语言读写的方式（`volatile`告诉编译器这个地址的值可能随时改变，不要优化掉）。

**可以把位带理解成"每个比特位都有了独立的开关"**。

---

#### 八、GPIO端口地址映射

```c
// IO口地址映射（输出数据寄存器ODR的地址）
#define GPIOA_ODR_Addr (GPIOA_BASE + 12)  // 0x4001080C
#define GPIOB_ODR_Addr (GPIOB_BASE + 12)  // 0x40010C0C
#define GPIOC_ODR_Addr (GPIOC_BASE + 12)  // 0x4001100C

// IO口地址映射（输入数据寄存器IDR的地址）
#define GPIOA_IDR_Addr (GPIOA_BASE + 8)   // 0x40010808
#define GPIOB_IDR_Addr (GPIOB_BASE + 8)   // 0x40010C08
#define GPIOC_IDR_Addr (GPIOC_BASE + 8)   // 0x40011008
```

**逐行解释**：

STM32的GPIO外设在内存中有固定的"住址"。`GPIOA_BASE`是`0x40010800`，然后每个寄存器按4字节排列：
- `CRL`（端口配置低寄存器）：偏移+0x00
- `CRH`（端口配置高寄存器）：偏移+0x04
- `IDR`（输入数据寄存器）：偏移+0x08 ← 地址+8
- `ODR`（输出数据寄存器）：偏移+0x0C ← 地址+12
- `BSRR`（位设置/复位寄存器）：偏移+0x10
- `BRR`（位复位寄存器）：偏移+0x14

所以`GPIOA_BASE + 12`就是GPIOA的输出数据寄存器的地址，也就是`0x4001080C`。

这些地址配合位带操作，就能实现单个引脚的直接控制。

---

#### 九、单引脚快捷操作宏

```c
// IO口操作，只对单一的IO口！
#define PAout(n) BIT_ADDR(GPIOA_ODR_Addr, n)  // 控制PA.n输出（写这个变量就是设置输出）
#define PAin(n)  BIT_ADDR(GPIOA_IDR_Addr, n)  // 读取PA.n输入（读这个变量就是读取输入）

#define PBout(n) BIT_ADDR(GPIOB_ODR_Addr, n)  // 控制PB.n输出
#define PBin(n)  BIT_ADDR(GPIOB_IDR_Addr, n)  // 读取PB.n输入

#define PCout(n) BIT_ADDR(GPIOC_ODR_Addr, n)  // 控制PC.n输出
#define PCin(n)  BIT_ADDR(GPIOC_IDR_Addr, n)  // 读取PC.n输入
```

**通俗解释**：

这些宏是最最常用的GPIO操作方式！有了它们，操作GPIO就像操作普通变量一样简单。

**使用示例**：
```c
PAout(5) = 1;    // PA5输出高电平（等效于 digitalHi(GPIOA, GPIO_Pin_5)）
PAout(5) = 0;    // PA5输出低电平

if (PAin(0) == 1) {   // 如果PA0输入是高电平（比如按键被按下）
    // 做某事
}
```

**技术原理**：这些宏通过`BIT_ADDR`和`BITBAND`，把PA.n引脚映射到一个独立的32位地址上。当你写`PAout(5) = 1`时，实际上是在往一个特殊的地址写1，STM32硬件会自动把这个1反映到GPIOA的第5号引脚上。

**注意事项**：这些宏只对单个引脚有效！如果要同时操作多个引脚，还是得用`digitalHi`、`digitalLo`或者直接操作寄存器。

---

#### 十、传感器数据结构体（SENSOR）

```c
// 传感器数据结构体类型
typedef struct
{
    uint8_t VibrationVal;     // 震动状态值（0=无震动，1=有震动）
    float   longitudeVal;     // 经度值（GPS获取）
    float   latitudeVal;      // 纬度值（GPS获取）
} SENSOR;
```

**逐行解释**：

`struct`是C语言的"结构体"，可以把多个不同类型的数据打包在一起。`typedef`是给这个结构体起了一个别名`SENSOR`。

- `uint8_t VibrationVal`：`uint8_t`是"无符号8位整数"（0~255），存储震动状态。`0`表示没检测到震动，`1`表示检测到震动。
- `float longitudeVal`：`float`是浮点数（带小数），存储经度。比如`116.397428`就是北京天安门附近的经度。
- `float latitudeVal`：存储纬度。比如`39.90923`就是北京天安门附近的纬度。

**为什么用结构体**：把所有传感器数据打包在一起，管理起来更方便。比如可以一次性把整个结构体传给显示函数。

---

#### 十一、系统标志位结构体（SYSTEM）

```c
// 系统标志位结构体类型
typedef struct
{
    uint8_t mqttflag;      // MQTT消息发布标志
    uint8_t Switch1;       // 灯光1开关标志位（0=关，1=开）
    uint8_t Switch2;       // 灯光2开关标志位（0=关，1=开）
    uint8_t Switch3;       // 报警开关标志位（0=关，1=开）
} SYSTEM;
```

**逐行解释**：

这个结构体存储系统的"控制信号"。它们不是直接控制硬件，而是作为一种"中间变量"，由主程序根据逻辑设置，再由硬件控制函数读取执行。

- `mqttflag`：MQTT消息发布标志。当这个值为1时，表示有新的数据需要通过MQTT发送到服务器。
- `Switch1`：灯光1的开关标志。在自动模式下，由程序自动控制。
- `Switch2`：灯光2的开关标志。在防盗模式下常亮。
- `Switch3`：报警（蜂鸣器）开关标志。当检测到非法入侵时置1，触发蜂鸣器。

**使用方式**：这种"标志位+执行函数"的架构让程序逻辑更清晰。比如`Automatic()`函数负责决定哪些开关该打开，`Action()`函数负责真正去开关硬件。

---

#### 十二、全局变量和函数声明

```c
extern SENSOR SensorData;   // 传感器数据实例（在main.c中定义）
extern SYSTEM System;       // 系统状态实例（在main.c中定义）
void NVIC_Config(void);     // 中断优先级配置函数（在sys.c中实现）
```

**通俗解释**：

- `extern`表示"这个变量在其他地方定义了，这里只是声明一下它的存在"。`SensorData`和`System`实际上是在`main.c`中用`SENSOR SensorData;`和`SYSTEM System;`定义的全局变量。用`extern`可以让其他.c文件也能访问它们。
- `void NVIC_Config(void)`：这是函数声明，告诉编译器"有个叫NVIC_Config的函数，没有参数，也没有返回值"。实际实现在`sys.c`中。

---

### 文件2：sys.c（中断优先级配置）

#### 一、文件整体功能说明

`sys.c`只做一件事：**配置STM32的中断优先级**。中断是嵌入式系统中最重要的概念之一——它让单片机能够"同时处理多件事"。

STM32有很多中断源（定时器中断、串口中断、外部中断等），当多个中断同时发生时，需要有一套规则来决定"谁先执行"。`NVIC_Config()`函数就是来设定这套规则的。

---

#### 二、NVIC_Config函数完整注释

```c
#include "sys.h"   // 包含自己的头文件

void NVIC_Config(void)
{
    /* 定义NVIC初始化结构体 */
    NVIC_InitTypeDef NVIC_InitStructure;
```

`NVIC_InitTypeDef`是STM32标准库定义的结构体类型，用于配置中断的各种参数。

```c
    /* 设置NVIC优先级分组为第2组
     * 优先级分组规则：
     * - 第2组：2位抢占优先级，2位子优先级
     * - 可配置范围：抢占优先级0-3，子优先级0-3
     */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
```

**通俗解释优先级分组**：

STM32的每个中断有两个优先级属性：
1. **抢占优先级**（Preemption Priority）：决定谁可以"打断"谁。如果一个中断的抢占优先级更高（数字更小），它可以打断正在执行的低优先级中断。就像急诊科医生可以打断普通门诊。
2. **子优先级**（Sub Priority）：当两个中断的抢占优先级相同时，子优先级决定谁先执行。就像两个同级别的病人，谁先来先看。

`NVIC_PriorityGroup_2`表示用4位优先级中的**2位做抢占优先级，2位做子优先级**。这意味着：
- 抢占优先级可以设0~3（共4个级别）
- 子优先级也可以设0~3（共4个级别）

**优先级数字越小，优先级越高**！0是最高优先级。

```c
    /* 配置TIM2中断 */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;           // 设置TIM2中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断通道
    NVIC_Init(&NVIC_InitStructure);                           // 应用配置
```

**逐行解释**：

这四行配置了TIM2（定时器2）的中断：
1. `NVIC_IRQChannel = TIM2_IRQn`：指定我们要配置的是"定时器2中断"。STM32给每个中断源都编了号，TIM2_IRQn就是那个编号。
2. `NVIC_IRQChannelPreemptionPriority = 2`：抢占优先级设为2（中等优先级）。
3. `NVIC_IRQChannelSubPriority = 2`：子优先级也设为2。
4. `NVIC_IRQChannelCmd = ENABLE`：**使能**这个中断（Enable = 打开）。如果不使能，定时器2溢出了也不会触发中断。
5. `NVIC_Init(&NVIC_InitStructure)`：把所有配置写入NVIC硬件寄存器，让设置生效。`&NVIC_InitStructure`表示传入结构体的地址。

```c
    /* 配置USART1中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         // 串口1中断（GPS用）
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断
    NVIC_Init(&NVIC_InitStructure); 
```

USART1中断用于接收GPS数据。GPS模块会不断地发送数据，每收到一个字节就可能触发一次中断，程序在中断里读取数据。

```c
    /* 配置USART2中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         // 串口2中断（ESP8266 WiFi用）
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        // 子优先级为2
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 使能中断
    NVIC_Init(&NVIC_InitStructure); 
}
```

USART2中断用于接收ESP8266 WiFi模块的数据。和串口1一样，数据来了就会触发中断。

**总结**：三个中断的优先级都一样（抢占=2，子=2），这意味着它们不会互相打断，而是按照发生顺序依次执行。

---

### 文件3：main.c（主程序入口）

#### 一、文件整体功能说明

`main.c`是整个程序的**入口点**。就像一栋大楼的大门，所有程序都从这里开始执行。它的主要工作：
1. **定义全局变量**：`SensorData`和`System`两个结构体实例
2. **初始化所有硬件**：在主循环开始前，把所有需要用的硬件都配置好
3. **主循环（while(1)）**：不停地执行核心任务（MQTT通信、GPS获取、模式选择）

---

#### 二、全局变量定义

```c
#include "sys.h"         // 包含系统头文件

SENSOR SensorData;       // 传感器结构体定义（全局变量，所有文件都能访问）
SYSTEM System;           // 系统标志位结构体定义（全局变量）
```

**通俗解释**：

这两行在内存中创建了两个"容器"：
- `SensorData`：一个`SENSOR`类型的变量，包含震动值、经度、纬度
- `System`：一个`SYSTEM`类型的变量，包含MQTT标志和三个开关标志

因为定义在main.c的最外层（不在任何函数内），它们是**全局变量**。用`extern`声明后，其他.c文件也能读写它们。这就像公司的公告板，所有人都能看到和修改。

---

#### 三、main函数——初始化阶段

```c
int main(void)           // main函数，程序从这里开始执行，void表示没有参数
{
```

**C语言规定**：每个C程序都必须有一个`main`函数，程序一上电就从这里开始跑。

```c
    delay_init();           // 延时函数初始化
```

初始化SysTick定时器，让后面的`delay_ms()`、`delay_us()`函数能正常工作。这是最早初始化的模块，因为后面的初始化可能需要用到延时。

```c
    BdgFlashInit();         // Flash初始化，包含阈值参数初始化与开关
```

初始化Flash存储系统。这个函数（在BridgeFlash.c中）会：
1. 解锁Flash控制器
2. 清除状态标志
3. 从Flash读取之前保存的配置
4. 初始化MQTT参数
5. 初始化传感器阈值

Flash是断电不丢失的存储器，用来保存设备名称等配置。

```c
    NVIC_Config();          // 中断优先级配置
```

调用sys.c中的函数，配置TIM2、USART1、USART2三个中断的优先级。

```c
    KEY_Init();             // 按键初始化
```

配置按键对应的GPIO引脚为输入模式，使能内部上拉/下拉电阻。

```c
    Beep_Init();            // 蜂鸣器初始化
```

配置蜂鸣器对应的GPIO引脚为输出模式。

```c
    oled_Init();            // OLED初始化
```

初始化OLED显示屏，包括IIC总线初始化、显示屏复位、亮度设置等。

```c
    LED_GPIO_Config();      // LED灯初始化
```

配置LED对应的GPIO引脚为输出模式。

```c
    Shake_Init();           // 震动传感器初始化
```

配置震动传感器对应的GPIO引脚为输入模式。

```c
    My_USART1();            // 串口1初始化，用于GPS模块
```

配置USART1：设置波特率（通常是9600）、数据位8位、停止位1位、无校验。GPS模块通过串口1发送经纬度数据。

```c
    My_USART2();            // 串口2初始化，与ESP8266通信，波特率115200
```

配置USART2，波特率115200（ESP8266默认用这个波特率）。WiFi模块通过串口2与STM32通信。

```c
    WIFI_RESET_init();      // ESP8266复位引脚初始化
```

配置ESP8266的复位引脚，用于在需要时复位WiFi模块。

```c
    Ali_MsessageInit();     // 服务器连接相关的变量初始化
```

初始化MQTT连接相关的变量，比如服务器地址、端口、设备名称等。

```c
    TIM2_Init(499, 7199);   // 定时器2初始化，定时扫描按键，获取震动状态并计数
```

配置定时器2的参数。传入的两个参数：
- `arr = 499`：自动重装载值，计数器从0数到499后溢出
- `psc = 7199`：预分频系数，把72MHz的主频除以7200（7199+1）

定时频率 = 72MHz / 7200 / 500 = 20Hz，即每50毫秒中断一次。

```c
    oled_Clear();           // 清屏，把OLED显示内容全部清除
```

初始化完成后清屏，为后续显示做准备。

---

#### 四、main函数——主循环（while(1)）

```c
    /*******************************************/
    while (1) {                          // 无限循环，嵌入式程序的核心
        ESP8266_run_handle();            // 连接MQTT服务器并与之通信
```

`while(1)`是嵌入式程序的"心脏"。因为单片机不像电脑有操作系统，程序不能结束，必须永远运行下去。

`ESP8266_run_handle()`：这个函数处理ESP8266 WiFi模块的状态机，包括：
- 初始化WiFi模块
- 连接WiFi路由器
- 连接MQTT服务器
- 发布（上报）数据到服务器
- 订阅（接收）服务器的控制命令

```c
        // 获取并传递经纬度数据
        get_gps();                       // 从GPS模块获取最新数据
        SensorData.longitudeVal = GPS_Data.longitude;  // 保存经度
        SensorData.latitudeVal = GPS_Data.latitude;    // 保存纬度
```

`get_gps()`解析USART1接收到的GPS数据（NMEA格式），提取出经度和纬度。然后保存到全局变量`SensorData`中。

```c
        Mode_selection();                // 模式判定（按键1选择模式）
```

调用模式选择函数，处理自动/防盗模式切换、按键响应、数据显示等逻辑。

```c
    }  // while(1)结束
}  // main函数结束
```

**整体逻辑流程**：
1. 开机 → 依次初始化所有硬件
2. 进入主循环 → 每轮循环做三件事：
   a. 处理WiFi和MQTT通信
   b. 更新GPS数据
   c. 执行模式控制和显示
3. 永远循环下去...

---

### 文件4：menu.h（菜单模块头文件）

#### 一、文件整体功能说明

`menu.h`是菜单和显示控制模块的声明文件。它定义了：
- 两个全局标志变量：`mode_selection`和`displayFlag`
- 五个函数的声明

---

#### 二、宏保护与头文件包含

```c
#ifndef __MENU_H        // 头文件保护
#define __MENU_H

#include "sys.h"        // 包含系统核心头文件
```

---

#### 三、外部变量声明

```c
extern uint8_t mode_selection;   // 模式切换标志位（0=正常模式，1=防盗模式）
extern uint8_t displayFlag;      // 页面显示标志位（0~3对应4个页面）
```

**详细说明**：

- `mode_selection`：模式选择标志。虽然名字里有"selection"，但从代码中看这个变量定义了但实际代码中主要用`displayFlag`来区分不同界面。

- `displayFlag`：显示页面标志，控制OLED当前显示哪个页面：
  - `0`：主页面——显示"汽车防盗"标题 + 经度 + 纬度
  - `1`：防盗模式页面——显示防盗状态，处理震动检测逻辑
  - `2`：MQTT连接状态页面——显示WiFi和MQTT的连接信息
  - `3`：MQTT订阅参数页面——显示订阅的主题和接收到的参数

---

#### 四、函数声明

```c
void Mode_selection(void);       // 模式选择总控函数（在main循环中被调用）
void Display(void);               // OLED显示函数（处理页面切换和内容显示）
void Automatic(void);             // 自动控制逻辑（决定LED和蜂鸣器的开关）
void Anti_theft_mode(void);       // 防盗模式处理（震动检测和报警逻辑）
void Action(void);                // 执行设备控制（根据标志位真正操作硬件）
```

---

### 文件5：menu.c（菜单和模式控制核心）

#### 一、文件整体功能说明

`menu.c`是整个系统的"业务逻辑中心"，负责：
1. **模式切换**：响应按键，切换显示页面和工作模式
2. **自动控制**：根据当前模式决定LED灯的开关
3. **防盗检测**：检测震动，判断是否非法入侵，触发报警
4. **数据显示**：在OLED上显示各种信息（经纬度、连接状态等）
5. **设备执行**：根据控制标志位真正操作LED和蜂鸣器

---

#### 二、变量定义

```c
#include "sys.h"                 // 包含系统头文件

uint8_t mode_selection = 0;      // 模式切换标志位，初始为0（正常模式）
uint8_t displayFlag = 0;         // 页面显示标志位，初始为0（主页面）
```

这两个变量在`menu.c`中定义（而不是在.h中定义），其他文件通过`extern`声明来访问。

---

#### 三、Mode_selection函数（模式选择总控）

```c
/*****************贯穿整个系统的函数 在主函数只需要调用这一个函数*****************/

void Mode_selection(void)
{
    Automatic();    // 自动控制——根据模式决定LED开关状态
    Action();       // 调用执行设备函数——真正操作LED和蜂鸣器
    Display();      // 数据显示——更新OLED屏幕内容
}
```

**通俗解释**：

这个函数是menu.c的"总指挥"。在`main()`的`while(1)`循环中调用。每次循环只做三件事：
1. `Automatic()`：先根据当前模式，设置好应该开哪个灯、关哪个灯的"计划"
2. `Action()`：按照计划，真正去操作LED和蜂鸣器
3. `Display()`：把最新的信息显示在OLED上

这种"先决定、再执行"的分层设计让逻辑更清晰。

---

#### 四、Automatic函数（自动控制逻辑）

```c
/*
 * 自动控制逻辑判断
 * 正常模式：灯光1常亮，灯光2熄灭
 * 防盗模式：灯光1熄灭，灯光2常亮
 */
void Automatic(void)
{
    if(displayFlag != 1){           // 如果当前不是防盗模式页面（displayFlag=1）
        System.Switch1 = 1;         // 灯光1开关标志位置1（开）
        System.Switch2 = 0;         // 灯光2开关标志位置0（关）
    }else{                          // 如果是防盗模式
        System.Switch1 = 0;         // 灯光1关
        System.Switch2 = 1;         // 灯光2开
    }
}
```

**逻辑说明**：

这个函数实现了两种模式的LED指示：
- **正常模式**（`displayFlag != 1`，即显示主页面、MQTT页面等）：LED1亮，LED2灭
- **防盗模式**（`displayFlag == 1`）：LED1灭，LED2亮

注意：这里只是设置了**标志位**（`System.Switch1`、`System.Switch2`），真正控制LED是在`Action()`函数中。这种设计实现了"决策"和"执行"的分离。

---

#### 五、Action函数（执行设备控制）

```c
/****
 * 根据执行设备的标志位来开关执行设备
 ****/
void Action(void)
{
    if (System.Switch1 == 1) {     // 如果灯光1的开关标志位为1
        LED1_ON;                    // 打开灯光1（宏定义，在led.h中定义）
    } else {
        LED1_OFF;                   // 关闭灯光1
    }

    if (System.Switch2 == 1) {     // 如果灯光2的开关标志位为1
        LED2_ON;                    // 打开灯光2
    } else {
        LED2_OFF;                   // 关闭灯光2
    }

    if (System.Switch3 == 1) {     // 如果报警标志位为1
        BEEP_ON;                    // 打开蜂鸣器（报警！）
    } else {
        BEEP_OFF;                   // 关闭蜂鸣器
    }

    LED_Update();                   // 更新LED闪烁模式（处理慢闪、快闪等效果）
}
```

**通俗解释**：

这个函数是"执行者"。它不看传感器，不做决策，只负责根据`System`结构体中的三个开关标志位来操作硬件：
- `Switch1` → LED1（主灯）
- `Switch2` → LED2（防盗指示灯）
- `Switch3` → 蜂鸣器（报警器）

`LED_Update()`是一个额外的函数，负责处理LED的闪烁效果（慢闪、快闪等），让LED不只是简单开关，还能有视觉提示效果。

---

#### 六、Display函数（OLED显示控制）

```c
/****
 * 传感器数据显示——处理页面切换和OLED内容显示
 ****/
void Display(void)
{
```

#### 6.1 按键响应——页面切换

```c
    // 按下按键1，回到数据显示界面（主页面）
    if (isKey1) {                   // isKey1在key.c中定义，按键1按下时为1
        isKey1 = 0;                 // 清除按键标志（处理过了就清零）
        oled_Clear();               // OLED清屏
        displayFlag = 0;            // 设置显示标志为0（主页面）
        LED_SetBlinkMode(STEADY_OFF);  // 第三个指示灯保持熄灭
        System.Switch3 = 0;         // 报警开关标志位置0（关闭报警）
    }
```

按键1的作用：**回到主页面，关闭报警**。这相当于"复位"操作。

```c
    // 按下按键2，进入防盗模式界面
    if (isKey2) {                   // 按键2被按下
        isKey2 = 0;                 // 清除标志
        oled_Clear();               // 清屏
        displayFlag = 1;            // 设置显示标志为1（防盗模式页面）
    }
```

按键2的作用：**进入防盗模式**。

```c
    // 按下按键3，显示MQTT连接状态
    if (isKey3) {                   // 按键3被按下
        isKey3 = 0;                 // 清除标志
        oled_Clear();               // 清屏
        displayFlag = 2;            // 设置显示标志为2（MQTT状态页面）
    }
```

按键3的作用：**查看MQTT连接状态**。

```c
    // 按下按键4，显示订阅主题的参数
    if (isKey4) {                   // 按键4被按下
        isKey4 = 0;                 // 清除标志
        oled_Clear();               // 清屏
        displayFlag = 3;            // 设置显示标志为3（订阅参数页面）
    }
```

按键4的作用：**查看MQTT订阅的主题参数**。

#### 6.2 页面内容显示

```c
    // 显示第一页（主页面）
    if(displayFlag == 0)
    {
        // 显示"汽车防盗"标题（4个汉字，每个16像素宽）
        oled_ShowCHinese(16 * 2, 2 * 0, 0);   // 第0个汉字"汽"，x=32, y=0
        oled_ShowCHinese(16 * 3, 2 * 0, 1);   // 第1个汉字"车"，x=48, y=0
        oled_ShowCHinese(16 * 4, 2 * 0, 2);   // 第2个汉字"防"，x=64, y=0
        oled_ShowCHinese(16 * 5, 2 * 0, 3);   // 第3个汉字"盗"，x=80, y=0

        // 显示经度
        oled_ShowCHinese(16 * 0, 2 * 1, 49);  // "经"字
        oled_ShowCHinese(16 * 1, 2 * 1, 51);  // "度"字
        oled_ShowString(16 * 2, 2 * 1, ":", 16);  // 冒号
        OLED_ShowFNum(16 * 3, 2 * 1, SensorData.longitudeVal, 16);  // 经度数值

        // 显示纬度
        oled_ShowCHinese(16 * 0, 2 * 2, 50);  // "纬"字
        oled_ShowCHinese(16 * 1, 2 * 2, 51);  // "度"字
        oled_ShowString(16 * 2, 2 * 2, ":", 16);  // 冒号
        OLED_ShowFNum(16 * 3, 2 * 2, SensorData.latitudeVal, 16);   // 纬度数值
    }
```

**OLED显示坐标说明**：
- OLED分辨率通常是128×64像素
- `oled_ShowCHinese(x, y, index)`：在(x,y)位置显示第index个汉字，每个汉字16×16像素
- `oled_ShowString(x, y, str, size)`：在(x,y)位置显示字符串
- `OLED_ShowFNum(x, y, num, size)`：在(x,y)位置显示浮点数
- x坐标每次加16（一个汉字的宽度），y坐标每次加2（以行为单位）

```c
    // 显示第二页（防盗模式页面）
    if (displayFlag == 1) {
        Anti_theft_mode();          // 调用防盗模式处理函数
    }

    // 显示第三页（MQTT连接状态）
    if (displayFlag == 2) {
        MqttCon_Display();          // 显示MQTT连接状态（在mqtt_drv.c中实现）
    }

    // 显示第四页（MQTT订阅参数）
    if (displayFlag == 3) {
        Topic_Display();            // 显示订阅的主题参数（在mqtt_drv.c中实现）
    }
}
```

---

#### 七、Anti_theft_mode函数（防盗核心逻辑）

这是整个系统最关键、最复杂的函数！

```c
void Anti_theft_mode(void)
{
    uint32_t CurrentTime = HAL_GetTick();      // 获取当前系统运行时间（毫秒）
    static uint32_t First_ShakeCount_Time;     // 第一次震动的时刻（static保持值不变）
    static uint32_t IntervalTime = 2000;       // 判断间隔时间：2秒
    static uint32_t Execution_Time = 10000;    // 报警持续时间：10秒
    static uint8_t Alarm_ShakeCount = 3;       // 触发报警需要的震动次数：3次
    static uint8_t Current_ShakeCount = 0;     // 当前已检测到的震动次数
```

**变量详解**：
- `CurrentTime`：当前系统时间，单位毫秒，从开机开始计时
- `First_ShakeCount_Time`：**静态变量**，记录第一次震动的时间点。`static`表示这个变量只初始化一次，函数退出后值仍然保持。
- `IntervalTime = 2000`：判断窗口，2秒内统计震动次数
- `Execution_Time = 10000`：报警持续10秒后自动解除
- `Alarm_ShakeCount = 3`：2秒内震动3次以上才认为是非法入侵
- `Current_ShakeCount`：当前震动计数

```c
    // 显示"防盗模式"标题
    oled_ShowCHinese(16 * 2, 2 * 0, 32);   // "防"
    oled_ShowCHinese(16 * 3, 2 * 0, 33);   // "盗"
    oled_ShowCHinese(16 * 4, 2 * 0, 6);    // "模"
    oled_ShowCHinese(16 * 5, 2 * 0, 7);    // "式"
```

#### 阶段1：超时重置

```c
    // 如果已经有震动记录，且距离第一次震动超过了2秒
    if(Current_ShakeCount != 0 && CurrentTime - First_ShakeCount_Time > IntervalTime){
        ShakeCount = 0;              // 清除定时器中断中累计的震动计数
        Current_ShakeCount = 0;      // 清除当前震动计数
    }
```

**逻辑**：如果2秒内没有达到3次震动，说明可能是误触（比如路过的人碰了一下），重置计数。

#### 阶段2：检测到第一次震动

```c
    // 如果检测到1次震动
    else if(ShakeCount == 1){
        if(Current_ShakeCount == 0){           // 如果是第一次震动
            First_ShakeCount_Time = CurrentTime; // 记录第一次震动的时间
            Current_ShakeCount = 1;              // 当前震动计数设为1
        }

        // 如果当前没有在闪烁（不是慢闪也不是快闪状态）
        if(Led_Controller.CurrentMode != SLOW_BLINK && 
           Led_Controller.CurrentMode != FAST_BLINK){
            LED_SetBlinkMode(SLOW_BLINK);        // 设置LED慢速闪烁（警示）
            Short_Beep();                        // 蜂鸣器短"嘀"一声

            // 显示"发生震动"
            oled_ShowCHinese(16 * 2, 2 * 2, 36);
            oled_ShowCHinese(16 * 3, 2 * 2, 37);
            oled_ShowCHinese(16 * 4, 2 * 2, 38);
            oled_ShowCHinese(16 * 5, 2 * 2, 39);
        }
    }
```

**逻辑**：
1. 第一次震动来了，记录时间点
2. LED开始慢闪，蜂鸣器嘀一声
3. OLED显示"发生震动"

这是**预警状态**——提醒有异常情况，但还不确定是入侵。

#### 阶段3：确认非法入侵

```c
    // 如果在2秒内震动次数≥3次
    else if(((ShakeCount >= Alarm_ShakeCount && 
              CurrentTime - First_ShakeCount_Time <= IntervalTime))
            && Led_Controller.CurrentMode != FAST_BLINK){

        Current_ShakeCount = 3;             // 当前震动计数设为3
        LED_SetBlinkMode(FAST_BLINK);        // LED快速闪烁（紧急）
        System.Switch3 = 1;                  // 报警开关标志位置1（蜂鸣器响！）

        // 显示"非法入侵"
        oled_ShowCHinese(16 * 2, 2 * 2, 12);
        oled_ShowCHinese(16 * 3, 2 * 2, 13);
        oled_ShowCHinese(16 * 4, 2 * 2, 14);
        oled_ShowCHinese(16 * 5, 2 * 2, 15);
    }
```

**逻辑**：
如果在2秒内检测到3次或以上震动，判定为**非法入侵**：
1. LED快速闪烁
2. 蜂鸣器持续报警（`System.Switch3 = 1`）
3. OLED显示"非法入侵"

#### 阶段4：自动解除报警

```c
    // 检测到震动后，执行10秒后自动结束
    if(CurrentTime - First_ShakeCount_Time > Execution_Time){
        LED_SetBlinkMode(STEADY_OFF);       // LED保持熄灭
        System.Switch3 = 0;                  // 关闭报警
        oled_ShowString(16 * 2, 2 * 2, "          ", 16);  // 清除第二行文字
    }
}
```

**逻辑**：报警10秒后自动停止，避免一直响影响周围。需要重新触发才会再次报警。

**整个防盗逻辑的流程图**：
```
没有震动 → 正常显示"防盗模式"
   ↓
1次震动 → LED慢闪 + 蜂鸣器短嘀 + 显示"发生震动"，开始2秒计时
   ↓
2秒内<3次震动 → 超时重置，恢复正常
   ↓
2秒内≥3次震动 → LED快闪 + 蜂鸣器持续响 + 显示"非法入侵"
   ↓
10秒后 → 自动解除报警
```

---

### 文件6：time2.h（定时器2头文件）

#### 一、文件整体功能说明

`time2.h`是定时器2模块的头文件，声明了震动计数变量和定时器初始化函数。

```c
#ifndef __TIME2_H       // 头文件保护
#define __TIME2_H

#include "sys.h"        // 包含系统头文件

extern uint8_t ShakeCount;   // 震动计数（在time2.c中定义，在time2中断中累加）

void TIM2_Init(u32 arr, u32 psc);   // 定时器2初始化函数声明

#endif
```

**变量说明**：
- `ShakeCount`：震动计数器。在TIM2中断服务函数中，每次检测到震动就加1。这个值在`Anti_theft_mode()`函数中被读取和判断。

**函数说明**：
- `TIM2_Init(u32 arr, u32 psc)`：初始化定时器2
  - `arr`：自动重装载值（Auto-Reload Register）
  - `psc`：预分频系数（Prescaler）

---

### 文件7：time2.c（定时器2驱动与中断服务）

#### 一、文件整体功能说明

`time2.c`负责：
1. 初始化定时器2，配置为每隔一定时间产生一次中断
2. 编写中断服务函数，在中断中执行按键扫描和震动检测

定时器就像STM32内置的"闹钟"，你可以设定它每隔多久"响一次"（产生中断），在中断里执行需要定期执行的任务。

---

#### 二、变量定义

```c
#include "sys.h"

uint8_t ShakeCount = 0;    // 震动计数，初始为0
```

---

#### 三、TIM2_Init函数（定时器初始化）

```c
/***************************************
 * 函数名：TIM2_Init
 * 功能：初始化定时器TIM2
 * 参数：
 *   - arr: 自动重装载值
 *   - psc: 预分频系数
 * 定时时间计算公式：T = (arr+1) * (psc+1) / 72MHz
 * 
 * 调用示例：TIM2_Init(499, 7199)
 * 定时时间 = (499+1) * (7199+1) / 72,000,000 = 500 * 7200 / 72M = 50ms
 **************************************/
void TIM2_Init(u32 arr, u32 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;   // 定义定时器配置结构体
```

```c
    /* 使能TIM2时钟（APB1总线） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
```

STM32的所有外设默认时钟都是关闭的（为了省电），使用前必须先"打开开关"。TIM2在APB1总线上，所以用`RCC_APB1PeriphClockCmd`函数使能。

```c
    /* 配置定时器基础参数 */
    TIM_TimeBaseStructure.TIM_Period = arr;          // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;       // 预分频系数
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;     // 时钟分频因子（不分频）
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);  // 应用配置
```

**参数详解**：
- `TIM_Period = arr`：计数器从0数到arr后溢出，产生中断。arr=499表示数500下。
- `TIM_Prescaler = psc`：把72MHz的时钟除以(psc+1)。psc=7199表示除以7200，得到10kHz。
- `TIM_ClockDivision = 0`：输入捕获用的时钟分频，这里不用，设为0。
- `TIM_CounterMode_Up`：向上计数模式（从0数到arr，然后归零重新数）。

**定时时间计算**：
```
输入时钟 = 72MHz / (7199+1) = 72MHz / 7200 = 10kHz
定时周期 = (499+1) / 10kHz = 500 / 10000 = 0.05秒 = 50毫秒
```

所以TIM2每50毫秒产生一次中断。

```c
    /* 使能TIM2更新中断 */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
```

使能"更新中断"（Update Interrupt），也就是计数器溢出时产生的中断。

```c
    /* 启动定时器TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}
```

最后启动定时器，开始计数。

---

#### 四、TIM2_IRQHandler函数（中断服务函数）

```c
/***************************************
 * 函数名：TIM2_IRQHandler
 * 功能：TIM2中断服务函数——每50ms执行一次
 * 注意：中断函数名必须是这个！在启动文件中有定义
 **************************************/
void TIM2_IRQHandler(void)
{
    /* 检查是否是TIM2更新中断 */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        KeyScan();   // 执行按键扫描（检查是否有按键被按下）
```

`TIM_GetITStatus`检查是不是"更新中断"触发的。因为TIM2可能有多种中断源，需要确认。

`KeyScan()`：扫描按键状态。如果有按键按下，就设置对应的`isKeyX`标志。

```c
        SensorData.VibrationVal = Shake_GetValue();   // 读取震动传感器状态
```

读取震动传感器的当前状态。返回值通常是0（有震动）或1（无震动），具体取决于传感器的类型和电路设计。

```c
        // 如果检测到震动（VibrationVal==0表示低电平触发），且当前在防盗模式
        if(SensorData.VibrationVal == 0 && displayFlag == 1)
            ShakeCount ++;    // 震动计数加1
```

这里有两个条件：
1. `SensorData.VibrationVal == 0`：震动传感器检测到震动（低电平有效）
2. `displayFlag == 1`：当前在防盗模式页面

只有在防盗模式下才累计震动次数。这是合理的——你在正常模式下不需要检测震动。

```c
        /* 清除中断标志位（必须！否则中断会持续触发） */
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
```

**非常重要**：中断处理完成后必须清除中断标志位！如果不清除，STM32会认为中断还没处理完，会一直重复进入中断，导致程序卡死。

**中断执行流程**：
```
定时器计数到499 → 溢出 → 触发中断 → 程序暂停主循环 → 执行TIM2_IRQHandler → 
KeyScan() + 震动检测 → 清除中断标志 → 返回主循环继续执行
```

整个过程大约几微秒到几十微秒，对主循环几乎没有影响。

---

### 文件8：BridgeFlash.h（Flash存储头文件）

#### 一、文件整体功能说明

`BridgeFlash.h`定义了STM32内部Flash存储的操作参数和数据结构。Flash是STM32内部的"硬盘"，断电后数据不丢失，适合保存配置参数。

---

#### 二、Flash物理参数定义

```c
#define FLASH_PAGE_SIZE 0x400U      // 单页容量1KB（1024字节，0x400=1024）
#define FLASH_ADDR_BASE 0x08000000  // STM32F103内部Flash起始地址
```

**说明**：
- STM32F103C8T6的Flash从地址`0x08000000`开始，总容量64KB
- Flash按"页"（Page）组织，每页1KB
- 共有64页（64KB / 1KB = 64）

```c
#define FLASH_ADDR_START (FLASH_ADDR_BASE + 58 * FLASH_PAGE_SIZE)  // 第58页起始
#define FLASH_ADDR_END (FLASH_ADDR_BASE + 64 * FLASH_PAGE_SIZE)    // 第64页结束
```

自定义使用第58~63页（共6页，6KB）来存储配置数据。前58页留给程序代码。

---

#### 三、存储操作参数

```c
#define SIZE_OF_HALF_WORD 2                  // 半字（Half Word）= 16位 = 2字节
#define UINT_OF_WRITE SIZE_OF_HALF_WORD      // Flash写入最小单位是半字（16位）
#define PAGE_BUF_SIZE (FLASH_PAGE_SIZE / UINT_OF_WRITE)  // 每页可写入的半字数 = 512
```

**说明**：STM32F103的Flash必须以**半字（16位）**为单位写入，不能单独写8位。

---

#### 四、实际使用地址

```c
#define FLASH_START_ADD 0x0800F000   // 配置数据存储起始地址
```

`0x0800F000` = `0x08000000 + 0xF000` = `0x08000000 + 15 * 4096`，也就是第60页的起始地址。

这个地址在程序Flash区域之外（程序通常从0x08000000开始，大小不超过60KB），所以不会覆盖程序代码。

---

#### 五、配置状态标志

```c
#define FLASH_FLAG_READY 0X5555   // "魔数"，表示Flash已写入有效配置
```

这是一个**魔数（Magic Number）**，用来判断Flash里是否已经写过有效数据。
- 如果`is_config == 0x5555`：说明已经配置过，数据有效
- 如果不是：说明是首次使用（Flash刚擦除后全是0xFF）

---

#### 六、配置数据结构体

```c
#pragma pack(1)   // 告诉编译器：按1字节对齐，不要填充空隙
typedef struct
{
    unsigned short is_config;          // 配置状态标志（2字节）
    unsigned char config_buff[500];    // 核心配置数据（500字节）
    unsigned char reservl[10];         // 保留字段（10字节）
} STR_CONFIG_DATA;
#pragma pack(4)   // 恢复默认4字节对齐
```

**通俗解释**：

`#pragma pack(1)`是个重要的指令。正常情况下，C编译器为了让CPU访问速度更快，会在结构体成员之间"填充"一些空隙，让每个成员都从4的倍数地址开始。但Flash写入需要精确控制每个字节的位置，所以用`pack(1)`禁止填充。

结构体总大小 = 2 + 500 + 10 = 512字节，正好是半字节（2字节）的整数倍，方便Flash写入。

---

#### 七、全局变量和函数声明

```c
extern STR_CONFIG_DATA strConfig_info;   // 配置数据实例

void BdgFlashInit(void);     // Flash系统初始化
void write_config(void);     // 写入配置
void read_config(void);      // 读取配置
void save_Threshold(void);   // 保存阈值
void Threshold_Init(void);   // 阈值初始化
```

---

### 文件9：BridgeFlash.c（Flash存储实现）

#### 一、文件整体功能说明

`BridgeFlash.c`实现了配置数据的Flash读写功能，包括：
1. 初始化Flash系统
2. 擦除Flash页
3. 写入数据到Flash
4. 从Flash读取数据
5. 配置参数的初始化和读取

---

#### 二、变量定义

```c
STR_CONFIG_DATA strConfig_info;   // 配置数据结构体实例
```

这个结构体在内存中占据512字节，是Flash读写的"中转站"。要保存的数据先放到这里，再写入Flash；从Flash读出的数据也先放到这里，再使用。

---

#### 三、BdgFlashInit函数（Flash系统初始化）

```c
void BdgFlashInit(void)
{
    FLASH_Unlock();      // 解锁Flash控制器
```

STM32的Flash默认是"上锁"的，防止程序意外写入。写操作前必须先解锁。

```c
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
```

清除Flash的状态标志位：
- `FLASH_FLAG_EOP`：操作完成标志
- `FLASH_FLAG_PGERR`：编程错误标志
- `FLASH_FLAG_WRPRTERR`：写保护错误标志

```c
    FLASH_Lock();        // 重新上锁（初始化阶段不需要写，先锁上）
```

```c
    read_config();       // 从Flash读取之前保存的配置
```

```c
    Mqtt_Parameter_init();  // 初始化MQTT参数（在mqtt_drv.c中实现）
    Threshold_Init();       // 初始化阈值
}
```

---

#### 四、FLASH_ErasePage_user函数（擦除Flash页）

```c
unsigned char FLASH_ErasePage_user(uint32_t page_address)
{
    FLASH_Status status;

    FLASH_Unlock();    // 解锁Flash
```

擦除前必须解锁。

```c
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | 
                    FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
```

清除各种状态标志。

```c
    status = FLASH_ErasePage(page_address);   // 执行页擦除
```

调用标准库函数擦除指定地址所在的页。**Flash的特性是：写入前必须先擦除！** 擦除后所有位变成1（0xFF）。

```c
    if (status != FLASH_COMPLETE) {   // 如果擦除失败
        FLASH_Lock();
        return 0;                     // 返回0表示失败
    }

    FLASH_Lock();      // 重新上锁
    return 1;          // 返回1表示成功
}
```

---

#### 五、FLASH_WriteData函数（写入数据）

```c
unsigned char FLASH_WriteData(uint32_t address, uint16_t *data, uint32_t length)
{
    FLASH_Status status;
    uint32_t i;

    if (address % 2 != 0) return 0;    // 地址必须是偶数（半字对齐）
```

Flash写入要求地址必须是2的倍数（半字对齐）。

```c
    FLASH_ErasePage_user(address);     // 先擦除目标页
```

**Flash写入三步曲**：解锁 → 擦除 → 写入 → 上锁

```c
    FLASH_Unlock();    // 解锁

    for (i = 0; i < length; i++) {
        status = FLASH_ProgramHalfWord(address + i * 2, data[i]);
```

循环写入，每次写2字节（一个半字）。地址每次增加2。

```c
        if (status != FLASH_COMPLETE) {   // 如果写入失败
            FLASH_Lock();
            return 0;
        }
    }

    FLASH_Lock();       // 上锁
    return 1;           // 成功
}
```

---

#### 六、FLASH_ReadData函数（读取数据）

```c
void FLASH_ReadData(uint32_t address, uint16_t *data, uint32_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        data[i] = *((uint16_t *)(address + i * 2));
    }
}
```

**通俗解释**：

Flash读取比写入简单得多！Flash在STM32的地址空间中是"内存映射"的，读Flash和读RAM一样简单。

`*(uint16_t *)(address)`的意思是：
1. `address`是一个32位整数（地址值）
2. `(uint16_t *)`把它强制转换成"指向16位无符号整数的指针"
3. `*`解引用，读取该地址的16位数据

---

#### 七、write_config和read_config函数（配置读写）

```c
void write_config(void)
{
    FLASH_WriteData(FLASH_START_ADD, 
                    (unsigned short *)&strConfig_info, 
                    sizeof(STR_CONFIG_DATA) / 2);
}
```

把`strConfig_info`结构体写入Flash。`sizeof(STR_CONFIG_DATA) / 2`计算半字数（总字节数除以2）。

```c
void read_config(void)
{
    FLASH_ReadData(FLASH_START_ADD, 
                   (unsigned short *)&strConfig_info, 
                   sizeof(STR_CONFIG_DATA) / 2);
}
```

从Flash读取配置到`strConfig_info`结构体。

---

#### 八、Threshold_Init函数（阈值初始化）

```c
void Threshold_Init(void)
{
    // 检查是否首次使用（Flash里的标志不是0x5555）
    if (strConfig_info.is_config != FLASH_FLAG_READY) {
```

如果是首次使用（Flash刚擦除过，is_config = 0xFFFF），需要设置默认值。

```c
        strConfig_info.is_config = FLASH_FLAG_READY;   // 标记为已配置
```

写入魔数`0x5555`，下次启动就知道已经配置过了。

```c
        // 把设备名称复制到配置缓冲区
        memcpy((unsigned char *)&strConfig_info.config_buff[100], 
               strMqtt_Inof.device_name, 100);
```

从`config_buff[100]`开始，保存100字节的MQTT设备名称。

```c
        write_config();    // 写入Flash
    }
    else {
        // 不是首次使用，从Flash加载配置
        memcpy(strMqtt_Inof.device_name, 
               (unsigned char *)&strConfig_info.config_buff[100], 100);
    }
}
```

从Flash读取设备名称到MQTT配置结构体中。

---

### 附录：整体代码逻辑流程图

#### 系统启动流程

```
上电复位
  │
  ▼
main() 入口
  │
  ├─ delay_init()           初始化延时函数
  ├─ BdgFlashInit()         初始化Flash存储
  │   ├─ FLASH_Unlock()     解锁Flash
  │   ├─ FLASH_ClearFlag()  清除标志
  │   ├─ FLASH_Lock()       上锁
  │   ├─ read_config()      从Flash读取配置
  │   ├─ Mqtt_Parameter_init() 初始化MQTT参数
  │   └─ Threshold_Init()   初始化阈值
  │       ├─ 首次使用? → 设默认值 + 写入Flash
  │       └─ 已配置?   → 从Flash加载
  ├─ NVIC_Config()          配置中断优先级
  ├─ KEY_Init()             按键初始化
  ├─ Beep_Init()            蜂鸣器初始化
  ├─ oled_Init()            OLED初始化
  ├─ LED_GPIO_Config()      LED初始化
  ├─ Shake_Init()           震动传感器初始化
  ├─ My_USART1()            串口1初始化（GPS，9600bps）
  ├─ My_USART2()            串口2初始化（ESP8266，115200bps）
  ├─ WIFI_RESET_init()      WiFi复位引脚初始化
  ├─ Ali_MsessageInit()     MQTT变量初始化
  ├─ TIM2_Init(499, 7199)   定时器2初始化（50ms中断）
  └─ oled_Clear()           OLED清屏
  │
  ▼
while(1) 主循环
  │
  ├─ ESP8266_run_handle()   处理WiFi/MQTT通信
  ├─ get_gps()              获取GPS数据
  ├─ 保存经纬度到SensorData
  └─ Mode_selection()       模式选择
      ├─ Automatic()         自动控制LED
      ├─ Action()            执行LED/蜂鸣器控制
      └─ Display()           OLED显示
          ├─ 按键1 → 主页面 (displayFlag=0)
          ├─ 按键2 → 防盗模式 (displayFlag=1)
          ├─ 按键3 → MQTT状态 (displayFlag=2)
          ├─ 按键4 → 订阅参数 (displayFlag=3)
          └─ 根据displayFlag显示对应内容
```

#### 定时器中断流程（每50ms执行一次）

```
TIM2计数溢出
  │
  ▼
TIM2_IRQHandler()
  │
  ├─ 检查中断标志
  ├─ KeyScan()              扫描按键
  ├─ Shake_GetValue()       读取震动传感器
  ├─ 防盗模式? → ShakeCount++
  └─ 清除中断标志
  │
  ▼
返回主循环
```

#### 防盗模式检测流程

```
进入防盗模式（按键2 或 displayFlag=1）
  │
  ▼
检测震动
  │
  ├─ 无震动 → 正常显示"防盗模式"
  │
  ├─ 1次震动 ───────────────────┐
  │   ├─ 记录第一次震动时间      │
  │   ├─ LED慢速闪烁            │
  │   ├─ 蜂鸣器短嘀一声          │
  │   └─ 显示"发生震动"         │
  │                              │
  ├─ 2秒内<3次震动 ──────────────┤─ 2秒判断窗口
  │   └─ 超时重置，恢复正常      │
  │                              │
  └─ 2秒内≥3次震动 ──────────────┘
      ├─ LED快速闪烁
      ├─ 蜂鸣器持续报警
      └─ 显示"非法入侵"
          │
          ▼
      10秒后自动解除
```

---

### 各模块关系图

```
                    ┌──────────────┐
                    │   main.c     │
                    │  (主控中心)   │
                    └──────┬───────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
   ┌─────────┐      ┌──────────┐      ┌──────────┐
   │  sys.c  │      │ menu.c   │      │ time2.c  │
   │(中断配置)│      │(模式控制) │      │(定时中断) │
   └─────────┘      └────┬─────┘      └────┬─────┘
                         │                  │
              ┌─────────┼─────────┐        │
              ▼         ▼         ▼        ▼
         ┌───────┐ ┌───────┐ ┌───────┐ ┌────────┐
         │ LED   │ │ BEEP  │ │ OLED  │ │ Shake  │
         │ 驱动  │ │ 驱动  │ │ 驱动  │ │ 传感器 │
         └───────┘ └───────┘ └───────┘ └────────┘
                                              │
              ┌───────────────────────────────┼───────┐
              │                               │       │
              ▼                               ▼       ▼
         ┌──────────┐                  ┌─────────┐ ┌────────┐
         │BridgeFlash│                  │   KEY   │ │  GPS   │
         │  (存储)  │                  │  (按键) │ │ (定位) │
         └──────────┘                  └─────────┘ └────────┘
                                              │
                                              ▼
                                        ┌──────────┐
                                        │ ESP8266  │
                                        │ (WiFi)   │
                                        └──────────┘
                                              │
                                              ▼
                                        ┌──────────┐
                                        │  MQTT    │
                                        │ (服务器) │
                                        └──────────┘
```

---

*本文档由AI代码分析助手生成，旨在帮助初学者理解STM32嵌入式系统的代码结构和逻辑。*




---

## 第三章：通信模块代码详解（串口、WiFi、MQTT）

> 本章讲解系统如何与"外界"通信。你将学习到：串口通信（USART）是什么、如何用代码控制ESP8266连接WiFi、MQTT物联网协议如何工作、手机APP如何远程控制小车。通信是物联网项目的灵魂，务必仔细阅读。

---


### 一、基础概念科普

#### 1.1 什么是串口通信（USART）？

**串口通信**就像两个人用莫尔斯电码发电报一样。想象你和一个朋友，每个人手里有一根电线，你们约定好规则：
- 电压高表示"1"，电压低表示"0"
- 每隔固定的时间检查一次电线的电压
- 按照约定的顺序，把一连串的0和1组合起来，就能表示字母、数字

**USART**（Universal Synchronous Asynchronous Receiver Transmitter，通用同步异步收发器）是STM32单片机内置的一个硬件模块，专门负责这种串行通信。它的作用就是把单片机内部的数据（比如传感器读数）转换成一串0和1发送出去，或者反过来把接收到的0和1转换成单片机能理解的数据。

**关键参数：**
- **波特率（Baud Rate）**：每秒传输多少位数据。就像两个人发报的速度要一致，通信双方必须使用相同的波特率。常见的有9600、115200等。
- **数据位**：每个数据帧包含多少位数据，通常是8位（1个字节）。
- **停止位**：表示一帧数据结束的位数，通常是1位。
- **校验位**：用于检查数据传输是否正确，可选无校验、奇校验、偶校验。

#### 1.2 什么是ESP8266？

**ESP8266**是一款低成本的WiFi模块，相当于给单片机装上了一个"无线网卡"。单片机本身没有WiFi功能，通过串口（USART）连接ESP8266模块后，就可以：
- 连接到家里的WiFi路由器
- 通过WiFi连接到互联网上的服务器
- 实现远程控制和数据传输

ESP8266使用**AT指令**进行控制。AT指令是一种标准化的命令格式，就像你跟模块说：
- `AT` -> "你在吗？" -> 模块回答 `OK`
- `AT+CWJAP="WiFi名称","密码"` -> "连接到指定的WiFi"
- `AT+CIPSTART="TCP","服务器IP",端口` -> "连接到指定的TCP服务器"

#### 1.3 什么是MQTT？

**MQTT**（Message Queuing Telemetry Transport，消息队列遥测传输）是一种专门用于物联网设备通信的协议。想象它是一个"微博订阅系统"：

- **发布者（Publisher）**：发微博的人，发布消息到某个话题（Topic）
- **订阅者（Subscriber）**：关注话题的人，能收到该话题下的所有新消息
- **Broker（代理服务器）**：微博平台，负责把消息从发布者转发给所有订阅者

**MQTT的关键概念：**
- **Topic（主题）**：消息的分类标签，比如 `"sensor/temperature"`、`"car/control"`
- **QoS（服务质量等级）**：
  - QoS 0：最多一次（发出去就不管了，像UDP）
  - QoS 1：至少一次（一定会送达，但可能重复）
  - QoS 2：恰好一次（保证送达且不重复，最可靠但最慢）
- **心跳包（Keep Alive）**：设备定期发送一个小消息告诉服务器"我还活着"，防止被断开连接

#### 1.4 本项目通信架构

```
+-------------+    USART1(串口1)    +-------------+
|             |  <---9600bps--->    |             |
|   STM32     |                     |   其他设备   |
|  单片机      |                     |  (调试/传感器)|
|             |                     |             |
|             |    USART2(串口2)    |  +--------+ |
|             |  <---115200bps-->   |  |ESP8266 | |
|             |                     |  |WiFi模块| |
|             |                     |  +--------+ |
|             |         TCP连接     |      |      |
|             |  <================> |   Internet  |
|             |                     |      |      |
|             |       MQTT协议      |  MQTT服务器  |
|             |  <================> |  (Broker)   |
+-------------+                     +-------------+
```

- **USART1**：波特率9600，用于调试输出或其他低速设备通信
- **USART2**：波特率115200，用于与ESP8266 WiFi模块通信
- **ESP8266**：通过AT指令控制，连接WiFi并建立TCP连接
- **MQTT**：在TCP连接之上实现物联网通信协议

---

### 二、my_usart1.h 分析

#### 2.1 文件整体功能

`my_usart1.h` 是 **USART1串口通信的头文件**。头文件的作用是"提前告诉"其他代码文件：这个模块有哪些函数可以用，有哪些常量定义。就像一份菜单，告诉你餐厅提供什么菜。

USART1在本项目中主要用于**调试信息输出**（连接电脑串口助手查看打印信息），波特率配置为9600bps。

#### 2.2 完整代码与逐行注释

```c
#ifndef __MY_USART1_H__          // 【条件编译】防止重复包含。如果还没定义这个宏，就执行下面的代码
#define __MY_USART1_H__          // 【宏定义】定义这个宏，标记该头文件已被包含过

#include "sys.h"                 // 【头文件包含】引入系统基础头文件，包含STM32寄存器定义和常用宏

// ============================================
// 硬件引脚定义
// ============================================
// 什么是GPIO引脚？STM32芯片有很多引脚（脚），每个引脚可以配置成不同的功能。
// PA9和PA10是GPIOA端口的第9和第10号引脚，复用为USART1的发送和接收功能。

#define USART1_GPIO_PIN_TX   GPIO_Pin_9    // 【宏定义】USART1发送引脚 = PA9
                                           // 作用：标识USART1的数据发送（TX）使用GPIOA的第9号引脚
                                           // 数据流向：STM32 -> 外部设备（通过PA9发送出去）

#define USART1_GPIO_PIN_RX   GPIO_Pin_10   // 【宏定义】USART1接收引脚 = PA10
                                           // 作用：标识USART1的数据接收（RX）使用GPIOA的第10号引脚
                                           // 数据流向：外部设备 -> STM32（通过PA10接收进来）

#define USART1_MAX_RECV_LEN   400          // 【宏定义】最大接收缓冲区长度 = 400字节
                                           // 作用：定义USART1一次最多能接收400个字节的数据
                                           // 为什么需要这个限制？内存是有限的，不能无限接收
                                           // 如果接收超过400字节，数据会被截断或标记接收完成

// ============================================
// 函数声明（告诉其他文件：这些函数存在，可以调用）
// ============================================

void My_USART1(void);            // 【函数声明】USART1初始化函数
                                 // 参数：无
                                 // 返回值：无
                                 // 功能：配置GPIO引脚、设置波特率9600、使能串口和中断

void u1_printf(char* fmt, ...);  // 【函数声明】格式化打印函数（类似printf）
                                 // 参数：fmt - 格式化字符串（如"Temp:%d\n"）
                                 //       ... - 可变参数，对应fmt中的占位符
                                 // 返回值：无
                                 // 功能：将格式化的字符串通过USART1发送出去
                                 // 示例：u1_printf("Hello %d", 123) -> 发送 "Hello 123"

void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data);
                                 // 【函数声明】单字节发送函数
                                 // 参数：USARTx - 要使用的串口（如USART1或USART2）
                                 //       Data - 要发送的数据（0-0x1FF，即0-511）
                                 // 返回值：无
                                 // 功能：向指定串口发送一个字节的数据

// ============================================
// 全局变量声明（extern表示"变量在其他文件中定义，这里只是声明"）
// ============================================

extern vu16 USART1_RX_STA;       // 【变量声明】USART1接收状态标志
                                 // vu16 = volatile unsigned short（16位无符号易失变量）
                                 // 作用：记录接收状态，第15位表示接收是否完成
                                 // 为什么用volatile？因为该变量会在中断中修改，编译器不能优化

extern u8 USART1_RX_BUF[USART1_MAX_RECV_LEN];
                                 // 【变量声明】USART1接收缓冲区数组
                                 // u8 = unsigned char（8位无符号，即1个字节）
                                 // 大小：400字节（由USART1_MAX_RECV_LEN定义）
                                 // 作用：存储从USART1接收到的所有字节数据
                                 // 就像一个400格的储物柜，每个格子放1个字节

#endif                           // 【条件编译结束】与#ifndef配对
```

#### 2.3 关键概念解释

**volatile关键字**：
`volatile` 是一个C语言关键字，告诉编译器"这个变量可能会在任何时候被外部因素改变"。在STM32中，USART1_RX_STA会在串口接收中断中被修改。如果不加volatile，编译器可能认为这个变量不会变，从而做出错误优化。

比喻：就像教室门口的显示屏，它的内容可能被外面的人随时修改。如果你每次都去看一眼显示屏（volatile），就不会错过更新；但如果你看了一眼就记在小本子上，以后只看小本子（编译器优化），就会错过新的通知。

---

### 三、my_usart1.c 分析

#### 3.1 文件整体功能

`my_usart1.c` 是 **USART1串口通信的驱动实现文件**。它负责：
1. 初始化USART1的硬件配置（GPIO引脚、波特率、中断等）
2. 提供字节发送和格式化打印功能
3. 实现接收中断服务程序，自动接收外部数据

#### 3.2 全局变量定义

```c
#include "sys.h"                 // 引入系统头文件

// 全局变量定义（注意：这里用 = 号赋值，是"定义"而非"声明"）
vu16 USART1_RX_STA = 0;           // 接收状态标志，初始化为0
                                  // 位15 = 1 表示接收完成
                                  // 位0~14 = 已接收的字节数（最大32767，实际受400字节限制）

u8 USART1_RX_BUF[USART1_MAX_RECV_LEN];  // 400字节的接收缓冲区
                                  // 用于存储USART1接收到的所有数据
```

#### 3.3 My_USART1() 初始化函数

```c
/**
 * 函数名：My_USART1
 * 功能：初始化USART1串口（配置GPIO、波特率、中断等）
 * 参数：无
 * 返回值：无
 * 硬件连接：TX=PA9, RX=PA10, 波特率9600
 */
void My_USART1(void)
{
    // 定义两个结构体变量，用于存储配置参数
    GPIO_InitTypeDef  GPIO_InitStrue;      // GPIO初始化结构体（配置引脚模式、速度等）
    USART_InitTypeDef USART1_InitStrue;    // USART初始化结构体（配置波特率、数据格式等）
    
    // ==== 第一步：使能时钟 ====
    // STM32的每个外设都有独立的时钟开关，使用前必须先打开时钟（就像给设备通电）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 使能GPIOA端口的时钟
                                                              // GPIOA控制PA0~PA15引脚
                                                              // ENABLE = 打开，DISABLE = 关闭
                                                              
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  // 使能USART1模块的时钟
                                                              // USART1挂在APB2总线上
                                                              // APB2总线最高72MHz
    
    // ==== 第二步：配置TX引脚（PA9，发送引脚）====
    // TX引脚需要输出数据，所以配置为"复用推挽输出"模式
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出（AF = Alternate Function 复用功能）
                                                       // 推挽输出：可以稳定输出高电平和低电平
                                                       // 复用：该引脚不是普通GPIO，而是用作USART功能
                                                       
    GPIO_InitStrue.GPIO_Pin = USART1_GPIO_PIN_TX;    // 选择PA9引脚（在.h中定义为GPIO_Pin_9）
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_10MHz;    // 输出速度10MHz（串口9600bps，10MHz足够）
    GPIO_Init(GPIOA, &GPIO_InitStrue);               // 将配置应用到GPIOA端口
    
    // ==== 第三步：配置RX引脚（PA10，接收引脚）====
    // RX引脚只需要读取外部数据，所以配置为"浮空输入"模式
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入模式
                                                        // 浮空：引脚既不接上拉电阻也不接下拉电阻
                                                        // 输入：只读取外部电平，不输出
                                                        // 为什么用浮空？因为串口线本身会驱动电平
                                                        
    GPIO_InitStrue.GPIO_Pin = USART1_GPIO_PIN_RX;     // 选择PA10引脚
    GPIO_Init(GPIOA, &GPIO_InitStrue);                // 将配置应用到GPIOA端口
    
    // ==== 第四步：配置USART参数 ====
    USART1_InitStrue.USART_BaudRate = 9600;                          // 波特率9600bps
                                                                       // 含义：每秒传输9600个bit
                                                                       // 换算：每秒约960字节（不算起始位/停止位等开销）
                                                                       
    USART1_InitStrue.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
                                                                                   // 流控：用额外的引脚控制数据流（防止接收方处理不过来）
                                                                                   // None：不使用流控，简单直接
                                                                                   
    USART1_InitStrue.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;     // 同时使能接收和发送模式
                                                                       // Rx = Receive（接收）
                                                                       // Tx = Transmit（发送）
                                                                       // | 是按位或操作，将两个模式组合
                                                                       
    USART1_InitStrue.USART_Parity = USART_Parity_No;                 // 无校验位
                                                                       // 校验位用于检测传输错误
                                                                       // 无校验 = 速度快一点，但不检查错误
                                                                       
    USART1_InitStrue.USART_StopBits = USART_StopBits_1;              // 1位停止位
                                                                       // 停止位标志一帧数据结束
                                                                       // 1位是标准配置
                                                                       
    USART1_InitStrue.USART_WordLength = USART_WordLength_8b;         // 8位数据长度
                                                                       // 即每个数据帧传输8个bit = 1个字节
                                                                       // 这是最常见的配置
                                                                       
    USART_Init(USART1, &USART1_InitStrue);    // 将所有USART参数应用到硬件
    
    // ==== 第五步：配置中断 ====
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);   // 使能接收中断（RXNE = Receive Data Register Not Empty）
                                                       // 含义：当收到数据时，自动触发中断函数执行
                                                       // 这样不用一直查询是否有数据，效率高
                                                       
    // ==== 第六步：使能USART ====
    USART_Cmd(USART1, ENABLE);    // 打开USART1总开关，开始工作
                                    // 就像打开电器的电源开关
}
```

#### 3.4 USART_SendByte() 单字节发送函数

```c
/**
 * 函数名：USART_SendByte
 * 功能：向指定USART串口发送单个字节
 * 参数：USARTx - USART外设指针（如USART1、USART2）
 *       Data   - 要发送的数据（范围0-0x1FF）
 * 返回值：无
 */
void USART_SendByte(USART_TypeDef* USARTx, uint16_t Data)
{
    // 参数检查（断言）：确保传入的USART外设是合法的
    assert_param(IS_USART_ALL_PERIPH(USARTx));    // 检查USARTx是否是有效的USART外设
    assert_param(IS_USART_DATA(Data));            // 检查Data是否在有效范围（0-0x1FF）
    
    // 发送数据：将数据写入数据寄存器（DR = Data Register）
    USARTx->DR = (Data & (uint16_t)0x01FF);     // 只取低9位数据写入寄存器
                                                   // -> 是"指针成员访问"运算符
                                                   // DR是USART的数据寄存器，写入这里就会发送出去
                                                   
    // 等待发送完成（阻塞式等待）
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
        // USART_GetFlagStatus：读取USART的状态标志位
        // USART_FLAG_TXE：Transmit Empty（发送寄存器空）标志
        // RESET = 0，表示发送寄存器还满着（数据还没移走）
        // 循环直到TXE=1（SET），表示数据已从寄存器移入移位寄存器
        // 
        // 比喻：就像等快递小哥把包裹拿走，你才能放下一个包裹
}
```

#### 3.5 u1_printf() 格式化打印函数

```c
// 发送缓冲区，8字节对齐（__align是编译器指令，要求数组在8字节边界上对齐）
// 对齐的目的是提高CPU访问效率
__align(8) char USART1_TxBuff[256];   // 256字节的发送缓冲区

/**
 * 函数名：u1_printf
 * 功能：格式化输出函数（类似标准C的printf，但输出到串口而非屏幕）
 * 参数：fmt - 格式化字符串（如"Value=%d\n"）
 *       ... - 可变参数列表（对应fmt中的%d、%f等占位符）
 * 返回值：无
 * 示例：u1_printf("Temperature:%d, Humidity:%d\n", 25, 60);
 *       -> 发送 "Temperature:25, Humidity:60\n" 到串口
 */
void u1_printf(char* fmt, ...) 
{  
    unsigned int i = 0, length = 0;    // i:循环计数器, length:格式化后字符串长度
    
    // ==== 可变参数处理（C语言标准做法）====
    va_list ap;                        // 定义可变参数列表变量
    va_start(ap, fmt);                 // 初始化ap，让它指向第一个可变参数
    vsprintf(USART1_TxBuff, fmt, ap);  // 将格式化字符串写入缓冲区
                                       // vsprintf = "v(可变参数) string print formatted"
                                       // 功能：按照fmt格式，用ap中的参数，生成完整字符串到USART1_TxBuff
    va_end(ap);                        // 清理可变参数列表（释放资源）
    
    // ==== 计算字符串长度 ====
    length = strlen((const char*)USART1_TxBuff);   // 获取格式化后字符串的长度
    
    // ==== 逐字节发送 ====
    while(i < length)                  // 循环发送每个字符
    {
        USART_SendByte(USART1, USART1_TxBuff[i]);   // 发送第i个字符
        i++;                                          // 计数器加1
    }
    
    // ==== 等待全部发送完成 ====
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
        // TC = Transmission Complete（传输完成）
        // TXE只表示数据从DR移走，TC表示数据真正发送到线路上
        // 等待TC确保最后一个字节确实发出了
}
```

#### 3.6 USART1_IRQHandler() 接收中断服务函数

```c
/**
 * 函数名：USART1_IRQHandler
 * 功能：USART1接收中断服务程序（ISR - Interrupt Service Routine）
 * 触发条件：当USART1收到数据时，硬件自动调用此函数
 * 参数：无
 * 返回值：无
 * 注意：ISR函数名必须与启动文件中的中断向量表匹配
 */
void USART1_IRQHandler(void)
{
    uint8_t res;    // 临时变量，存储接收到的字节
    
    // 检查是否是接收中断（可能还有其他中断源触发）
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)   // 判断接收中断标志是否置位
    {
        res = USART_ReceiveData(USART1);    // 读取数据寄存器的值（同时清除中断标志）
                                             // 这一步很重要！不读DR的话中断会一直触发
        
        // 检查接收是否已完成（位15是否为0）
        if((USART1_RX_STA & (1 << 15)) == 0)    // 如果位15=0，表示还没接收完成
        {
            // 检查缓冲区是否还有空间
            if(USART1_RX_STA < USART1_MAX_RECV_LEN)    // 如果已接收字节数 < 400
            {
                USART1_RX_BUF[USART1_RX_STA++] = res;   // 将数据存入缓冲区
                                                         // USART1_RX_STA++：先使用当前值，再加1
                                                         // 第一次接收：BUF[0] = 数据，STA变为1
                                                         // 第二次接收：BUF[1] = 数据，STA变为2
            }
            else    // 缓冲区已满（400字节都接收了）
            {
                USART1_RX_STA |= 1 << 15;    // 将位15置1，标记接收完成
                                              // |= 是按位或赋值运算符
                                              // 1 << 15 = 0x8000 = 32768
            }
        }
        
        // 清除中断挂起位（确保中断不会重复触发）
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
```

#### 3.7 USART1_RX_STA 状态标志详解

```
USART1_RX_STA 是一个16位变量，位的分配如下：

  位15 | 位14 ~ 位0
   |        |
   |        +---> 已接收的字节计数（最大32767，但实际限制在400以内）
   |
   +---> 接收完成标志（1=接收完成，0=还在接收中）

示例：
  USART1_RX_STA = 0x0005  -> 已接收5个字节，还在接收中
  USART1_RX_STA = 0x8005  -> 已接收5个字节，接收完成（位15置1）
  USART1_RX_STA = 0x0190  -> 已接收400个字节，还在接收中
  USART1_RX_STA = 0x8190  -> 已接收400个字节，缓冲区满，强制完成
```

---

### 四、my_usart2.h 分析

#### 4.1 文件整体功能

`my_usart2.h` 是 **USART2串口通信的头文件**。USART2在本项目中专门用于与**ESP8266 WiFi模块**通信，波特率配置为115200bps（比USART1快12倍），因为WiFi模块需要更高的数据传输速率。

#### 4.2 完整代码与逐行注释

```c
#ifndef __MY_USART2_H__          // 【条件编译】防止重复包含
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

#include "sys.h"  // 系统级头文件

// ============================================
// 硬件引脚定义
// ============================================
#define USART2_GPIO_PIN_TX   GPIO_Pin_2  // 发送引脚PA2
                                           // STM32的PA2引脚复用为USART2_TX
                                           // 连接ESP8266的RX引脚（交叉连接：TX接RX，RX接TX）
                                           
#define USART2_GPIO_PIN_RX   GPIO_Pin_3  // 接收引脚PA3
                                           // STM32的PA3引脚复用为USART2_RX
                                           // 连接ESP8266的TX引脚

// ============================================
// 全局变量声明
// ============================================
extern u16 USART2_RxCounter;       // 接收数据计数器（当前接收到的字节数）
                                    // u16 = unsigned short（16位无符号整数）
                                    // 作用：记录USART2已经接收了多少个字节
                                    // 注意：这个变量可能在中断中更新
                                    
extern char USART2_RxBuff[1024];   // 接收缓冲区（1024字节）
                                    // 作用：存储从USART2（即ESP8266）接收到的数据
                                    // 大小1024 = 1KB，比USART1的400字节更大
                                    // 因为WiFi通信数据量更大

// ============================================
// 函数声明
// ============================================
void My_USART2(void);  // USART2初始化函数
                       // 参数：无
                       // 返回值：无
                       // 功能：配置GPIO、波特率115200、使能中断等

// 格式化打印函数（类printf实现）
// 注意: 使用内部256字节缓冲区，长数据可能被截断
void u2_printf(char* fmt, ...);
                       // 参数：fmt - 格式化字符串
                       //       ... - 可变参数
                       // 返回值：无
                       // 功能：格式化输出到USART2（发送给ESP8266模块）
                       // 注意：与u1_printf类似，但发送到USART2

// 自定义协议数据包发送函数
// 数据包格式: [长度高字节][长度低字节][数据1][数据2]...
// 参数data: 需包含完整包头的数据缓冲区指针
void u2_TxData(unsigned char *data);
                       // 参数：data - 指向数据包的指针
                       // 返回值：无
                       // 功能：发送带长度头的自定义协议数据包
                       // 协议格式说明：
                       //   data[0] = 长度高字节（数据部分长度 >> 8）
                       //   data[1] = 长度低字节（数据部分长度 & 0xFF）
                       //   data[2] ~ data[length+1] = 实际数据

#endif  // __MY_USART2_H__
```

---

### 五、my_usart2.c 分析

#### 5.1 文件整体功能

`my_usart2.c` 是 **USART2串口通信的驱动实现文件**。它是与ESP8266 WiFi模块通信的"桥梁"，所有发往WiFi模块的AT指令和MQTT数据都通过这里发送。

#### 5.2 全局变量定义

```c
#include "sys.h"

u16 USART2_RxCounter = 0;         // 接收计数器，初始化为0
                                  // 每收到一个字节，这个计数器加1
                                  // 用于跟踪当前接收缓冲区中有多少数据
                                  
char USART2_RxBuff[1024];         // 1024字节接收缓冲区
                                  // 索引范围：0 ~ 1023
                                  // 用于存储ESP8266返回的响应数据
```

#### 5.3 My_USART2() 初始化函数

```c
/**
 * 函数功能: USART2初始化配置
 * 硬件连接:
 *   TX - PA2(复用推挽输出)
 *   RX - PA3(浮空输入)
 * 通信参数:
 *   - 波特率: 115200bps（ESP8266默认波特率）
 *   - 数据位: 8位
 *   - 停止位: 1位
 *   - 校验位: 无
 *   - 流控  : 无
 * 特殊配置:
 *   - 使能接收中断(USART_IT_RXNE)
 *   - 使用APB1总线时钟(36MHz典型值)
 */
void My_USART2(void)
{
    GPIO_InitTypeDef  GPIO_InitStrue;   // GPIO配置结构体
    USART_InitTypeDef USART2_InitStrue; // USART配置结构体
    
    // ==== 时钟使能配置 ====
    // 注意：USART2挂在APB1总线上，而USART1挂在APB2总线上！
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 使能GPIOA时钟（GPIO都在APB2上）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  // 使能USART2时钟（APB1总线）
                                                              // APB1最高36MHz，USART2由此分频得到波特率
    
    // ==== TX引脚(PA2)配置 ====
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_AF_PP;       // 复用推挽输出模式
                                                        // AF_PP = Alternate Function Push-Pull
                                                        // 推挽：可以主动输出高电平（推）或低电平（挽）
                                                        
    GPIO_InitStrue.GPIO_Pin = USART2_GPIO_PIN_TX;     // PA2引脚（在.h中定义为GPIO_Pin_2）
    GPIO_InitStrue.GPIO_Speed = GPIO_Speed_10MHz;     // 输出速度10MHz
    GPIO_Init(GPIOA, &GPIO_InitStrue);                // 应用TX配置
    
    // ==== RX引脚(PA3)配置 ====
    GPIO_InitStrue.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入模式
                                                        // 不加上拉/下拉电阻，直接读取外部电平
                                                        // ESP8266的TX引脚会驱动电平，不需要上拉
                                                        
    GPIO_InitStrue.GPIO_Pin = USART2_GPIO_PIN_RX;     // PA3引脚（在.h中定义为GPIO_Pin_3）
    GPIO_Init(GPIOA, &GPIO_InitStrue);                // 应用RX配置
    
    // ==== USART参数配置 ====
    USART2_InitStrue.USART_BaudRate = 115200;         // 波特率115200
                                                        // 为什么用115200？因为这是ESP8266模块的默认波特率
                                                        // 115200 = 每秒约11.52KB数据传输
                                                        
    USART2_InitStrue.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART2_InitStrue.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收发模式
    USART2_InitStrue.USART_Parity = USART_Parity_No;  // 无校验位
    USART2_InitStrue.USART_StopBits = USART_StopBits_1; // 1位停止位
    USART2_InitStrue.USART_WordLength = USART_WordLength_8b; // 8位数据长度
    
    USART_Init(USART2, &USART2_InitStrue);            // 应用所有USART配置
    
    // ==== 中断与使能配置 ====
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);    // 使能接收中断
                                                        // 当ESP8266发来数据时，自动进入中断处理
                                                        
    USART_Cmd(USART2, ENABLE);                        // 使能USART2外设，开始工作
}
```

#### 5.4 u2_printf() 格式化输出函数

```c
/**
 * 函数功能: USART2格式化输出函数(阻塞式发送)
 * 参数说明:
 *   fmt : 格式化字符串(支持%d,%f等标准格式符)
 *   ... : 可变参数列表(与fmt中的格式符匹配)
 * 实现流程:
 *   1. 使用va_list处理可变参数
 *   2. 通过vsprintf将格式化结果存入256字节发送缓冲区
 *   3. 轮询方式逐个字节发送数据
 *   4. 等待最后一个字节发送完成(TC标志)
 */
__align(8) char USART2_TxBuff[256];  // 发送缓冲区(8字节对齐)
                                         // 大小256字节，格式化字符串不能超过255字符（含结束符\0）

void u2_printf(char* fmt, ...)
{
    unsigned int i = 0, length = 0;    // i:循环计数器, length:字符串长度
    
    // ==== 可变参数处理 ====
    va_list ap;                        // 定义可变参数列表
    va_start(ap, fmt);                 // 初始化，指向第一个可变参数
    vsprintf(USART2_TxBuff, fmt, ap);  // 格式化字符串到缓冲区
    va_end(ap);                        // 结束可变参数处理
    
    // ==== 计算有效数据长度 ====
    length = strlen((const char*)USART2_TxBuff);  // 获取格式化后字符串长度
    
    // ==== 轮询发送数据 ====
    while(i < length)                  // 循环发送每个字节
    {
        USART_SendByte(USART2, USART2_TxBuff[i]); // 阻塞发送单字节
                                                      // USART_SendByte内部会等待TXE标志
        i++;                                      // 下一个字节
    }
    
    // ==== 等待发送完成 ====
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
        // TC = Transmission Complete
        // 等待所有数据真正发送到线路上
}
```

#### 5.5 u2_TxData() 自定义协议数据包发送

```c
/**
 * 函数功能: USART2阻塞式数据包发送
 * 参数说明:
 *   data - 数据包指针，结构为：
 *          [0]   : 长度高字节 (length >> 8)
 *          [1]   : 长度低字节 (length & 0xFF)
 *          [2~n+1]: 实际数据 (n = 数据长度 = data[0]*256 + data[1])
 * 执行流程:
 *   1. 等待上一次传输完成(TC标志置位)
 *   2. 解析数据包前两字节获取数据长度
 *   3. 按长度值遍历发送后续字节
 *   4. 每个字节发送后等待硬件传输完成
 */
void u2_TxData(unsigned char *data)
{
    int i;    // 循环计数器
    
    // 等待历史传输完成
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
        // 确保上一次发送已经完全结束，避免数据冲突
    
    // 解析长度并遍历发送
    // data[0] * 256 + data[1]  =  (data[0] << 8) | data[1]
    // 这是大端模式（高位字节在前）的长度编码
    for(i = 1; i <= (data[0] * 256 + data[1]); i++)
    {
        USART_SendData(USART2, data[i + 1]);  // 发送data[2]开始的数据
                                                  // i+1是因为data[0]和data[1]是长度头，实际数据从data[2]开始
                                                  
        // 阻塞等待单字节发送完成
        while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
            // 每发一个字节都要等它真正发出去，再发下一个
            // 这种方式简单但效率较低，适合数据量不大的场景
    }
}
```

#### 5.6 注释掉的USART2_IRQHandler中断处理

```c
// 这段代码被注释掉了，说明当前项目中USART2的接收中断在其他地方处理
// 从esp8266_drv.c中可以看到，USART2_IRQHandler实际定义在那里
//
//void USART2_IRQHandler(void)
//{
//    if(Connect_flag == 0)    // 如果还没连接到MQTT服务器
//    {
//        if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  // 接收中断
//        {
//            if(USART2->DR)    // 如果数据有效
//            {
//                // 简单模式：直接存入缓冲区
//                USART2_RxBuff[USART2_RxCounter] = USART_ReceiveData(USART2);
//                USART2_RxCounter++;
//            }
//        }
//    }
//    else    // 已连接到MQTT服务器
//    {
//        // 高级模式：使用定时器判断数据帧结束
//        USART2_RxBuff[USART2_RxCounter] = USART_ReceiveData(USART2);
//        if(USART2_RxCounter == 0)
//        {
//            TIM_Cmd(TIM4, ENABLE);    // 第一个字节到达，启动定时器
//        }
//        else
//        {
//            TIM_SetCounter(TIM4, 0);  // 后续字节到达，重置定时器计数
//        }
//        USART2_RxCounter++;
//        // 原理：如果在一定时间内没有新数据到来，就认为一帧数据接收完毕
//        // 这是一种常见的"空闲中断"替代方案
//    }
//};
```

---

### 六、esp8266_drv.h 分析

#### 6.1 文件整体功能

`esp8266_drv.h` 是 **ESP8266 WiFi模块驱动的头文件**。它定义了：
1. WiFi连接参数（SSID和密码）
2. ESP8266复位引脚配置
3. 多个数据结构体（状态结构体、接收结构体、AT指令结构体等）
4. MQTT连接参数结构体
5. 网络状态枚举
6. 所有相关函数的声明

#### 6.2 完整代码与逐行注释

```c
#ifndef _ESP8266_DRV_H_          // 条件编译：防止重复包含
#define _ESP8266_DRV_H_

#include "sys.h"                 // 系统基础头文件

// ============================================
// WiFi连接参数配置
// ============================================
#define ID   "abcd"              // 【WiFi热点名称（SSID）】
                                  // 作用：ESP8266要连接的WiFi网络名称
                                  // 注意：必须连接2.4GHz频段的WiFi，ESP8266不支持5GHz
                                  // 使用提示：需要用手机开启热点，热点名称改成"abcd"
                                  
#define PASSWORD   "12345678"    // 【WiFi热点密码】
                                  // 作用：对应WiFi网络的密码
                                  // 安全提示：实际项目中不应明文存储密码

// ============================================
// ESP8266硬件复位引脚配置
// ============================================
// ESP8266模块有一个RESET引脚，拉低一段时间可以复位模块
// 本项目使用PA1引脚控制ESP8266的复位

#define ESP8266_RESET_LCK        RCC_APB2Periph_GPIOA    // 复位引脚时钟源
                                                          // PA1属于GPIOA端口
                                                          
#define ESP8266_RESET_PORT       GPIOA                   // 复位引脚所属端口
#define ESP8266_RESET_PIN        GPIO_Pin_1              // 复位引脚 = PA1

// 复位控制宏：设置引脚高电平（模块正常工作）
#define ESP8266_RESET_SET_H     GPIO_SetBits(ESP8266_RESET_PORT, ESP8266_RESET_PIN)
                                 // GPIO_SetBits：将指定引脚设为高电平（3.3V）
                                 // 高电平时ESP8266正常工作
                                 
// 复位控制宏：设置引脚低电平（开始复位）
#define ESP8266_RESET_SET_L     GPIO_ResetBits(ESP8266_RESET_PORT, ESP8266_RESET_PIN)
                                 // GPIO_ResetBits：将指定引脚设为低电平（0V）
                                 // 低电平持续一定时间后ESP8266复位重启

// ============================================
// 数据结构体定义
// ============================================

/**
 * 数据接收结构体
 * 用途：存储从串口接收到的原始数据
 */
typedef struct
{
    unsigned int rcv_trick_ms;       // 最后接收时间戳（毫秒）
                                      // 作用：记录最后一次收到数据的时间
                                      // 用途：通过时间差判断数据帧是否接收完毕
                                      
    unsigned char buff[256];         // 接收缓冲区（256字节）
                                      // 作用：临时存储串口收到的字节数据
                                      
    unsigned int rcv_cnt;            // 已接收字节计数
                                      // 作用：记录当前buff中有效数据的字节数
} DATA_RCV_STR;

/**
 * ESP8266状态结构体
 * 用途：管理ESP8266模块的整体运行状态
 */
typedef struct
{
    unsigned char Net_stu;           // 网络连接状态（对应ESP8266_NET_STU枚举）
                                      // 0=NET_NULL（初始/复位），1=复位完成，... 9=透传模式
                                      
    unsigned char Cmd_stu;           // AT指令执行状态
                                      // 0=空闲/发送指令，1=等待响应
                                      
    unsigned int cmd_send_trick;     // 指令发送时间戳
                                      // 作用：记录发送AT指令的时间，用于判断超时
                                      
    unsigned char retry_cnt;         // 当前重试次数
                                      // 作用：AT指令发送失败时，记录已重试的次数
                                      
    unsigned char run_heart;         // 运行心跳计数（调试用途）
                                      // 每次主循环加1，用于判断程序是否在运行
                                      
    unsigned short rcv_idle_cnt;     // 接收空闲计数（单位：秒）
                                      // 作用：计算距离上次收到数据多少秒
                                      // 用途：判断连接是否断开（超过10秒认为断线）
                                      
    unsigned char offline_cnt;       // 离线计数
                                      // 作用：统计断线重连的次数
                                      
    unsigned char MQTT_Connect_flag; // MQTT连接状态标志
                                      // 0=未连接，1=已连接
} ESP8266_STR;

/**
 * AT指令结构体
 * 用途：定义一条AT指令的命令内容、预期响应、超时时间和重试次数
 */
typedef struct
{
    char cmd[80];                    // AT指令字符串（如"AT\r\n"）
                                      // 最大长度80字符
                                      
    char respond[20];                // 预期响应字符串（如"OK"）
                                      // 最大长度20字符
                                      // 作用：判断AT指令是否执行成功
                                      
    unsigned short wait_time;        // 等待响应的超时时间（毫秒）
                                      // 如2000 = 等待2秒
                                      
    unsigned short retry_cnt;        // 最大重试次数
                                      // 如3 = 失败后最多重试3次
} CMD_TYPE_STRUCT;

// ============================================
// 网络状态枚举
// ============================================
/**
 * ESP8266网络连接状态机
 * 这些值按顺序排列，从NET_NULL逐步执行到NET_CIPSEND
 * 每个值对应AT指令表strEsp8266_cmd中的一个索引
 */
typedef enum
{
    NET_NULL = 0,        // 0: 初始状态 / 需要复位
    NET_RESET,           // 1: AT测试（发送"AT\r\n"）
    NET_AT,              // 2: 二次AT测试
    NET_RST,             // 3: 设置STA模式（AT+CWMODE=1）
    NET_CWAUTOCONN,      // 4: 模块软复位（AT+RST）
    NET_CAJAP,           // 5: 关闭自动重连（AT+CWAUTOCONN=0）
    NET_CIPMUX,          // 6: 连接WiFi（AT+CWJAP）
    NET_CIPMODE,         // 7: 单连接模式（AT+CIPMUX=0）
    NET_CIPSTART,        // 8: 透传模式（AT+CIPMODE=1）
    NET_CIPSEND,         // 9: 建立TCP连接（AT+CIPSTART）
                         // 10: 进入数据发送模式（AT+CIPSEND）
                         //     此时Net_stu > NET_CIPSEND，进入MQTT通信阶段
} ESP8266_NET_STU;

// ============================================
// MQTT参数结构体
// ============================================
/**
 * MQTT连接信息结构体
 * 用途：存储MQTT Broker连接所需的各项参数
 */
typedef struct
{
    uint32_t device_id;       // 设备ID（数值型）
                               // 作用：标识当前设备的唯一编号
                               
    unsigned int Round_num;   // 随机数
                               // 作用：生成随机客户端ID和主题名，避免冲突
                               
    char device_name[20];     // 设备名称
                               // 格式：基础名称 + 6位随机数
                               // 示例："DEVICENAME123456"
                               
    char key[20];             // 产品密钥（对应MQTT的Username）
                               // 格式：基础密钥 + 6位随机数
                               // 示例："PRODUCTKEY123456"
                               
    char secre[20];           // 设备密钥（对应MQTT的Password）
                               // 格式：基础密钥 + 6位随机数
                               // 示例："DEVICESECRE123456"
                               
    char sub[20];             // 订阅主题（Subscribe Topic）
                               // 设备订阅这个主题来接收服务器消息
                               // 示例："pub123456"
                               
    char pub[20];             // 发布主题（Publish Topic）
                               // 设备向这个主题发送数据
                               // 示例："sub123456"
} STR_MQTT_INFO;

// ============================================
// 全局变量声明（extern表示定义在其他文件中）
// ============================================
extern STR_MQTT_INFO strMqtt_Inof;        // MQTT连接参数实例
extern ESP8266_STR strEsp8266_info;       // ESP8266状态实例
extern DATA_RCV_STR strEsp8266_rcv;       // 数据接收缓冲区实例

// ============================================
// 函数声明
// ============================================
unsigned int get_sys_tick(void);           // 获取系统当前时间戳（毫秒）
void send_data_to_dev(char *data, unsigned short len);  // 通过USART2发送数据
void USART_rcv_ch(unsigned char ch, DATA_RCV_STR *str_rcv);  // 接收字符处理
void ESP8266_run_handle(void);             // ESP8266主状态机（核心函数）
void USARTx_RCVHandler(USART_TypeDef *USARTX);  // 通用USART接收中断处理
unsigned int get_round_num(void);          // 通过ADC生成随机数
void Mqtt_Parameter_init(void);            // 初始化MQTT参数（含随机数）
void WIFI_RESET_init(void);                // 初始化复位引脚
void MqttCon_Display(void);                // 显示连接参数到OLED
void Topic_Display(void);                  // 显示主题到OLED
void mqttPublic(void);                     // 发布MQTT消息
void Scheduled(void);                      // 定时1秒发布一次数据

#endif  // _ESP8266_DRV_H_
```

---

### 七、esp8266_drv.c 分析

#### 7.1 文件整体功能

`esp8266_drv.c` 是 **ESP8266 WiFi模块驱动的核心实现文件**，是整个通信模块中最复杂的文件。它实现了：
1. ESP8266的AT指令状态机（自动执行一系列AT指令完成WiFi连接）
2. 硬件复位控制
3. 串口数据发送和接收处理
4. MQTT参数初始化和随机数生成
5. 定时数据发布（Scheduled）
6. OLED显示连接状态

#### 7.2 AT指令配置表

```c
#include "sys.h"                        // 系统头文件

// ESP8266模块全局状态结构体
ESP8266_STR strEsp8266_info;            // 存储ESP8266整体状态
DATA_RCV_STR strEsp8266_rcv;            // 串口接收缓冲区（用于存储AT指令响应）
STR_MQTT_INFO strMqtt_Inof;             // MQTT连接参数

// AT指令配置表：命令、预期响应、超时时间、重试次数
// 这是一个"指令剧本"，ESP8266按照这个顺序逐步执行
CMD_TYPE_STRUCT strEsp8266_cmd[20] =
{
    // 索引0: 基础AT指令测试
    {.cmd = "AT\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // 作用：最基础的AT指令，如果模块正常会回复OK
    // \r\n 是回车换行符（即按下回车键），AT指令必须以这个结尾
    
    // 索引1: 二次确认模块就绪
    {.cmd = "AT\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // 再发一次AT确保模块真的准备好了
    
    // 索引2: 设置STA模式（Station模式，作为客户端连接WiFi）
    {.cmd = "AT+CWMODE=1\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // CWMODE=1 表示STA模式（像手机一样连接路由器）
    // CWMODE=2 表示AP模式（自己当路由器）
    // CWMODE=3 表示STA+AP双模式
    
    // 索引3: 模块软复位
    {.cmd = "AT+RST\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // 复位后模块会重新启动，等待"ready"响应
    
    // 索引4: 关闭自动重连
    {.cmd = "AT+CWAUTOCONN=0\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // 关闭WiFi自动连接，由程序手动控制连接时机
    
    // 索引5: 动态填充 - 连接WiFi
    {.cmd = " ", .respond = "OK", .wait_time = 10000, .retry_cnt = 3},
    // 这个指令在运行时动态填充：
    // sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ID, PASSWORD);
    // 示例：AT+CWJAP="abcd","12345678"
    // 等待时间10秒，因为连接WiFi可能需要较长时间
    
    // 索引6: 单连接模式
    {.cmd = "AT+CIPMUX=0\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // CIPMUX=0 表示单连接模式（一次只能连接一个服务器）
    // CIPMUX=1 表示多连接模式（最多5个连接）
    
    // 索引7: 透传模式
    {.cmd = "AT+CIPMODE=1\r\n", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // CIPMODE=1 开启透传模式
    // 透传模式：发送的数据直接传给TCP连接，不需要额外的AT指令包装
    // 就像打开了"直通通道"，方便后续发送MQTT数据
    
    // 索引8: 动态填充 - 建立TCP连接
    {.cmd = " ", .respond = "OK", .wait_time = 2000, .retry_cnt = 3},
    // 运行时动态填充：
    // sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ServerIP, ServerPort);
    // 示例：AT+CIPSTART="TCP","47.109.89.8",1883
    // TCP: 使用TCP协议（可靠传输）
    // 47.109.89.8: MQTT Broker服务器IP
    // 1883: MQTT默认端口
    
    // 索引9: 进入数据发送模式
    {.cmd = "AT+CIPSEND\r\n", .respond = "\r\nOK\r\n\r\n>", .wait_time = 2000, .retry_cnt = 3},
    // 发送这个指令后，ESP8266进入透传发送模式
    // 响应中的 ">" 表示模块准备好了，可以开始发送数据
    // 之后所有数据直接通过TCP发送到服务器
};
```

#### 7.3 WIFI_RESET_init() 复位引脚初始化

```c
/**
 * 函数名：WIFI_RESET_init
 * 功能：初始化ESP8266硬件复位引脚（PA1）
 * 参数：无
 * 返回值：无
 */
void WIFI_RESET_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    RCC_APB2PeriphClockCmd(ESP8266_RESET_LCK, ENABLE);  // 使能复位引脚所在端口的时钟
    
    GPIO_InitStruct.GPIO_Pin = ESP8266_RESET_PIN;       // 配置PA1引脚
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;       // 推挽输出模式
                                                         // 推挽：可以主动输出高电平或低电平
                                                         // 需要主动控制复位引脚的电平
                                                         
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      // 输出速度50MHz
                                                         // 复位不需要高速，但50MHz也没问题
                                                         
    GPIO_Init(ESP8266_RESET_PORT, &GPIO_InitStruct);    // 应用配置
}
```

#### 7.4 get_sys_tick() 获取系统时间戳

```c
/**
 * 函数名：get_sys_tick
 * 功能：获取系统当前时间戳（单位：毫秒）
 * 参数：无
 * 返回值：unsigned int - 从系统启动开始的毫秒数
 * 原理：依赖全局变量sysTickUptime，由系统定时器中断每1ms累加
 */
unsigned int get_sys_tick(void)
{
    return sysTickUptime;   // 返回系统滴答计数器
                             // 这个变量在sys.c中的SysTick中断里每1ms加1
                             // 相当于一个毫秒级的"秒表"
}
```

#### 7.5 send_data_to_dev() 发送数据到ESP8266

```c
/**
 * 函数名：send_data_to_dev
 * 功能：通过USART2发送数据到ESP8266模块
 * 参数：data - 待发送数据指针
 *       len  - 数据长度
 * 返回值：无
 * 特点：使用寄存器级操作，效率较高
 */
void send_data_to_dev(char *data, unsigned short len)
{
    unsigned int i;
    for(i = 0; i < len; i++)    // 轮询方式逐字节发送
    {
        // 等待发送缓冲区空（TXE = Transmit Empty）
        while(!(USART2->SR & USART_FLAG_TXE));
            // USART2->SR 是状态寄存器
            // USART_FLAG_TXE = 0x80（第7位）
            // 当第7位为1时，表示发送数据寄存器为空，可以写入新数据
            
        USART2->DR = data[i];   // 写入数据寄存器，硬件自动发送
                                 // DR = Data Register
                                 // 写入后，数据被移入移位寄存器，逐位发送到TX引脚
                                 
        // 等待发送完成（TC = Transmission Complete）
        while(!(USART2->SR & USART_FLAG_TC));
            // 等待这一位数据真正发送完毕
            // TC标志表示数据已从移位寄存器发送到线路上
    }
}
```

#### 7.6 USART_rcv_ch() 接收字符存入缓冲区

```c
/**
 * 函数名：USART_rcv_ch
 * 功能：将接收到的单个字符存入接收缓冲区
 * 参数：ch      - 接收到的字符
 *       str_rcv - 接收缓冲区结构体指针
 * 返回值：无
 */
void USART_rcv_ch(unsigned char ch, DATA_RCV_STR *str_rcv)
{
    if(str_rcv->rcv_cnt < 256)    // 缓冲区未满时存入（最大256字节）
    {
        str_rcv->buff[str_rcv->rcv_cnt] = ch;   // 将字符存入缓冲区尾部
        str_rcv->rcv_cnt++;                        // 计数器加1
        str_rcv->rcv_trick_ms = get_sys_tick();    // 更新最后接收时间戳
                                                     // 这个更新时间很重要！用于判断数据帧是否结束
    }
}
```

#### 7.7 USARTx_RCVHandler() 通用接收中断处理

```c
/**
 * 函数名：USARTx_RCVHandler
 * 功能：通用USART接收中断处理函数
 * 参数：USARTX - 串口外设指针（可以是USART1、USART2等）
 * 返回值：无
 */
void USARTx_RCVHandler(USART_TypeDef *USARTX)
{
    unsigned char ch;    // 临时存储接收到的字节
    
    // 检查是否是接收中断（RXNE）或溢出错误（ORE）
    if((USARTX->SR & USART_FLAG_RXNE) || (USARTX->SR & USART_FLAG_ORE))
    {
        ch = USARTX->DR;   // 读取数据寄存器（这一步同时清除中断标志）
                            // 如果不读取DR，中断会一直触发，程序卡死在中断里
                            
        USART_ClearFlag(USARTX, USART_FLAG_RXNE);   // 清除接收中断标志
        
        if(USARTX == USART2)    // 只处理USART2的数据（ESP8266通信）
        {
            USART_rcv_ch(ch, &strEsp8266_rcv);   // 存入ESP8266接收缓冲区
        }
    }
}

/**
 * 函数名：USART2_IRQHandler
 * 功能：USART2中断服务函数（STM32启动文件中定义的中断入口）
 * 注意：函数名必须完全匹配，否则中断无法正确响应
 */
void USART2_IRQHandler(void)
{
    USARTx_RCVHandler(USART2);   // 调用通用处理函数
}
```

#### 7.8 write_cmd() AT指令发送与响应处理（核心函数）

```c
/**
 * 函数名：write_cmd
 * 功能：发送AT指令并等待响应，处理重试逻辑
 *      这是一个简单的状态机，有两个状态：发送指令、等待响应
 * 参数：strCMD - 指令结构体指针（包含命令、预期响应、超时时间、重试次数）
 * 返回值：1 - 成功（收到预期响应）
 *         2 - 超时/失败（重试次数耗尽）
 *         0 - 处理中（还在等待响应）
 */
unsigned char write_cmd(CMD_TYPE_STRUCT *strCMD)
{
    char *pStr = NULL;    // 字符串匹配结果指针
    
    // ==== 状态0：发送指令 ====
    if(strEsp8266_info.Cmd_stu == 0)
    {
        // 发送AT指令到ESP8266
        send_data_to_dev(strCMD->cmd, strlen(strCMD->cmd));
        
        strEsp8266_info.Cmd_stu = 1;                  // 切换到"等待响应"状态
        strEsp8266_info.cmd_send_trick = get_sys_tick(); // 记录发送时间（用于超时判断）
        
        // 清空接收缓冲区（准备接收新的响应）
        memset(strEsp8266_rcv.buff, 0, 256);
        strEsp8266_rcv.rcv_cnt = 0;
    }
    // ==== 状态1：等待响应 ====
    else if(strEsp8266_info.Cmd_stu == 1)
    {
        // 检查是否超时
        if(get_sys_tick() - strEsp8266_info.cmd_send_trick < strCMD->wait_time)
        {
            // 还没超时，检查是否收到预期响应
            if(strEsp8266_rcv.rcv_cnt >= 2)    // 至少接收到2字节才开始匹配（避免误匹配）
            {
                // 在接收缓冲区中查找预期响应字符串
                pStr = strstr((char *)strEsp8266_rcv.buff, strCMD->respond);
                if(pStr)    // 匹配成功
                {
                    strEsp8266_info.Cmd_stu = 0;      // 重置指令状态为空闲
                    strEsp8266_info.retry_cnt = 0;    // 重置重试计数
                    return 1;                          // 返回成功
                }
            }
        }
        else    // 超时处理
        {
            strEsp8266_info.Cmd_stu = 0;              // 重置为发送状态
            strEsp8266_info.retry_cnt++;              // 重试计数加1
            
            if(strEsp8266_info.retry_cnt > strCMD->retry_cnt)
            {
                strEsp8266_info.retry_cnt = 0;        // 重置重试计数
                return 2;                              // 重试次数耗尽，返回失败
            }
        }
    }
    return 0;    // 还在处理中（等待响应或等待重试）
}
```

#### 7.9 ESP8266_reset() 硬件复位函数

```c
/**
 * 函数名：ESP8266_reset
 * 功能：硬件复位ESP8266模块
 *      拉低复位引脚250ms -> 恢复高电平 -> 等待250ms -> 完成
 * 参数：无
 * 返回值：1 - 复位完成；0 - 处理中
 */
unsigned char ESP8266_reset(void)
{
    // static关键字：变量只初始化一次，下次调用保留上次值
    // 这是实现状态机的关键！
    static unsigned int reset_start_cnt = 0;    // 记录阶段开始时间
    static unsigned char reset_stu = 0;          // 复位状态（0/1/2）
    
    // 阶段0：开始复位，拉低引脚
    if(reset_stu == 0)
    {
        reset_stu = 1;                          // 进入阶段1
        reset_start_cnt = get_sys_tick();       // 记录当前时间
        ESP8266_RESET_SET_L;                     // 拉低复位引脚（PA1 = 0V）
                                                 // ESP8266开始复位
    }
    // 阶段1：保持低电平250ms
    else if(reset_stu == 1)
    {
        if(get_sys_tick() - reset_start_cnt > 250)   // 已经保持了250ms
        {
            reset_stu = 2;                           // 进入阶段2
            reset_start_cnt = get_sys_tick();        // 重新计时
            ESP8266_RESET_SET_H;                      // 恢复高电平（PA1 = 3.3V）
                                                      // ESP8266开始启动
        }
    }
    // 阶段2：等待250ms让模块启动
    else if(reset_stu == 2)
    {
        if(get_sys_tick() - reset_start_cnt > 250)   // 又等待了250ms
        {
            reset_stu = 0;                           // 重置状态，为下次复位做准备
            return 1;                                 // 复位完成！
        }
    }
    return 0;    // 还在处理中
}
```

#### 7.10 ESP8266_run_handle() 主状态机（最核心函数）

```c
// MQTT服务器IP地址和端口
char ServerIP[128] = {"47.109.89.8"};   // MQTT Broker服务器的IP地址
int  ServerPort = 1883;                  // MQTT默认端口号（1883是MQTT的标准端口）

/**
 * 函数名：ESP8266_run_handle
 * 功能：ESP8266主状态机，这是整个通信模块的"总指挥"
 *      管理网络连接、AT指令流程和MQTT通信
 * 参数：无
 * 返回值：无
 * 调用周期：需要在主循环中持续调用（如每10ms一次）
 */
void ESP8266_run_handle(void)
{
    unsigned char ret;                                    // 指令执行结果
    static unsigned int last_mqtt_heart_cnt = 0;         // MQTT心跳包计时器
    static unsigned int last_subscirbe_tick = 0;         // 订阅重试计时器
    
    Scheduled();    // 先执行定时数据发布函数（检查是否需要发送数据）
    
    // ============================================================
    // 状态1：NET_NULL - 初始状态或需要复位
    // ============================================================
    if(strEsp8266_info.Net_stu == NET_NULL)
    {
        // 执行硬件复位流程
        if(ESP8266_reset())    // ESP8266_reset()返回1表示复位完成
        {
            strEsp8266_info.Net_stu = 1;    // 进入AT指令执行阶段（从索引1开始）
            
            // 动态填充AT指令5：连接WiFi的指令
            // ID = "abcd"（WiFi名称）, PASSWORD = "12345678"（WiFi密码）
            sprintf(strEsp8266_cmd[5].cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ID, PASSWORD);
            // 结果：strEsp8266_cmd[5].cmd = "AT+CWJAP=\"abcd\",\"12345678\"\r\n"
            
            // 修改预期响应：WiFi连接成功后会返回"CONNECTED"
            sprintf(strEsp8266_cmd[5].respond, "CONNECTED");
            
            // 动态填充AT指令8：建立TCP连接到MQTT服务器
            sprintf(strEsp8266_cmd[8].cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ServerIP, ServerPort);
            // 结果：strEsp8266_cmd[8].cmd = "AT+CIPSTART=\"TCP\",\"47.109.89.8\",1883\r\n"
        }
    }
    // ============================================================
    // 状态2：AT指令执行阶段（从状态1逐步执行到NET_CIPSEND）
    // ============================================================
    else if(strEsp8266_info.Net_stu > 0 && strEsp8266_info.Net_stu <= NET_CIPSEND)
    {
        // 发送当前状态对应的AT指令，并等待响应
        // strEsp8266_info.Net_stu 的值就是AT指令表的索引
        ret = write_cmd(&strEsp8266_cmd[strEsp8266_info.Net_stu]);
        
        if(ret == 1)    // 指令响应成功
        {
            strEsp8266_info.Net_stu++;     // 状态加1，执行下一条AT指令
            
            // 所有AT指令执行完毕（进入透传模式）
            if(strEsp8266_info.Net_stu > NET_CIPSEND)
            {
                // 清空接收缓冲区，准备接收MQTT数据
                memset(strEsp8266_rcv.buff, 0, 256);
                strEsp8266_rcv.rcv_cnt = 0;
                
                MQTT_Buff_Init();    // 初始化MQTT协议缓冲区（在mqtt_drv.c中定义）
            }
        }
        else if(ret == 2)    // 指令响应超时或失败
        {
            strEsp8266_info.Net_stu = NET_NULL;    // 重置为初始状态，重新从头开始
        }
    }
    // ============================================================
    // 状态3：TCP连接成功后，维持MQTT通信
    // ============================================================
    else
    {
        period_get_server();    // 处理从服务器接收到的数据（解析MQTT报文）
        
        // ---- 心跳包逻辑：每3秒发送一次MQTT Ping ----
        if(get_sys_tick() - last_mqtt_heart_cnt >= 3 * 1000)   // 3000ms = 3秒
        {
            last_mqtt_heart_cnt = get_sys_tick();   // 重置计时器
            mqtt_Ping();                             // 发送MQTT心跳包
        }
        
        // ---- 订阅逻辑：连接成功但未订阅时，每秒尝试一次 ----
        if(ConnectPack_flag && !SubscribePack_flag)
        // ConnectPack_flag=1 表示MQTT连接成功
        // SubscribePack_flag=0 表示还没订阅主题
        {
            if(get_sys_tick() - last_subscirbe_tick >= 1000)   // 1000ms = 1秒
            {
                last_subscirbe_tick = get_sys_tick();          // 重置计时器
                Mqtt_Subscribe(strMqtt_Inof.sub, 1);           // 订阅主题（QoS=1）
            }
        }
        
        mqtt_Content();    // 处理MQTT数据内容（解析收到的消息等）
        
        // ---- 连接保持检测：10秒内未收到数据则判定为断线 ----
        strEsp8266_info.rcv_idle_cnt = (get_sys_tick() - strEsp8266_rcv.rcv_trick_ms) / 1000;
        // 计算距离上次收到数据多少秒
        
        if(strEsp8266_info.rcv_idle_cnt > 10)    // 超过10秒没收到数据
        {
            strEsp8266_info.Net_stu = NET_NULL;    // 重置连接状态，重新连接
            strEsp8266_info.offline_cnt++;         // 离线计数器累加（统计用）
        }
    }
    
    strEsp8266_info.run_heart++;    // 运行计数器（调试用）
}
```

#### 7.11 get_round_num() ADC随机数生成

```c
/**
 * 函数名：get_round_num
 * 功能：通过ADC（模数转换器）采样生成6位随机数
 * 原理：ADC在没有任何输入信号时，采样值会有微小的噪声波动（硬件本底噪声）
 *      利用这种不可预测的噪声来生成随机数
 * 返回值：6位十进制随机数（范围：000000-999999）
 */
unsigned int get_round_num(void)
{
    unsigned short adc_res;       // ADC原始采样值（12位精度，范围0-4095）
    unsigned int round_num = 0;   // 生成的6位随机数
    unsigned char i, S_bit, G_bit; // 临时变量
    
    // ==== 时钟配置 ====
    RCC->APB2ENR |= 0x204;        // 使能APB2总线时钟
                                   // bit2 = ADC1时钟, bit8 = GPIOA时钟
                                   // 0x204 = 0b 0000 0010 0000 0100
                                   
    RCC->CFGR &= 0xFFFF3FFF;      // 清零ADC预分频器位（bit15-14）
    RCC->CFGR |= 0x00008000;      // 设置ADC时钟 = 主时钟/6（72MHz/6 = 12MHz）
                                   // ADC最高允许14MHz，12MHz是安全值
    
    // ==== ADC初始化 ====
    ADC1->CR2 = 0X000E0000;       // 控制寄存器初始值
    ADC1->CR2 |= 0X00000001;      // 使能ADC1（ADON位置1）
    
    // ==== ADC校准 ====
    ADC1->CR2 |= 0X08;            // 初始化校准
    while((ADC1->CR2 & 0x08) != 0);  // 等待校准初始化完成
    ADC1->CR2 |= 0X04;            // 开始校准
    while((ADC1->CR2 & 0x04) != 0);  // 等待校准完成
    
    // ==== 采样时间配置 ====
    ADC1->SMPR2 = 0X001C000;      // 通道4采样时间：480周期（最长时间，提高精度）
    
    // ==== 随机数生成核心 ====
    for(i = 0; i < 6; i++)        // 生成6位随机数
    {
        // 启动ADC转换
        ADC1->CR2 |= 0X00500000;   // 启动单次转换
        while(!(ADC1->SR & 0x02));  // 等待转换完成
        adc_res = ADC1->DR;         // 读取12位ADC值（0-4095）
        
        // 数字处理：取ADC值的个位和十位相加
        G_bit = adc_res % 10;           // 提取个位数字（0-9）
        S_bit = (adc_res / 10) % 10;    // 提取十位数字（0-9）
        S_bit += G_bit;                  // 数字叠加（0-18）
        
        // 构建6位数：每次循环左移1位（十进制）并叠加新值
        round_num *= 10;     // 十进制左移（如 123 -> 1230）
        round_num += S_bit;  // 添加新数字（如 1230 + 7 = 1237）
    }
    return round_num;   // 返回6位随机数
}
```

#### 7.12 Mqtt_Parameter_init() MQTT参数初始化

```c
// 宏定义（用于拼接随机数生成唯一的MQTT参数）
#define DEVICENAME      "DEVICENAME"    // 设备名前缀
#define PRODUCTKEY      "PRODUCTKEY"    // 产品密钥前缀（Username）
#define DEVICESECRE     "DEVICESECRE"   // 设备密钥前缀（Password）
#define SUBSCRIBE_TOPIC  "pub"           // 订阅主题前缀
#define P_TOPIC_NAME     "sub"           // 发布主题前缀

/**
 * 函数名：Mqtt_Parameter_init
 * 功能：初始化MQTT连接参数，用随机数拼接生成唯一的客户端标识
 * 目的：避免多个设备使用相同的客户端ID导致连接冲突
 * 参数：无
 * 返回值：无
 */
void Mqtt_Parameter_init(void)
{
    // 清零MQTT参数结构体
    memset((char *)&strMqtt_Inof, 0, sizeof(strMqtt_Inof));
    
    // 生成6位随机数（取模确保不超过999999）
    strMqtt_Inof.Round_num = get_round_num() % 1000000;
    
    // 用sprintf格式化生成带随机数的MQTT参数
    // %s 表示字符串，%06d 表示6位数字（不足补0）
    
    sprintf(strMqtt_Inof.device_name, "%s%06d", DEVICENAME, strMqtt_Inof.Round_num);
    // 结果示例："DEVICENAME123456"
    
    sprintf(strMqtt_Inof.key, "%s%06d", PRODUCTKEY, strMqtt_Inof.Round_num);
    // 结果示例："PRODUCTKEY123456"
    
    sprintf(strMqtt_Inof.secre, "%s%06d", DEVICESECRE, strMqtt_Inof.Round_num);
    // 结果示例："DEVICESECRE123456"
    
    sprintf(strMqtt_Inof.sub, "%s%06d", SUBSCRIBE_TOPIC, strMqtt_Inof.Round_num);
    // 结果示例："pub123456"
    
    sprintf(strMqtt_Inof.pub, "%s%06d", P_TOPIC_NAME, strMqtt_Inof.Round_num);
    // 结果示例："sub123456"
}
```

#### 7.13 mqttPublic() 发布MQTT消息

```c
/**
 * 函数名：mqttPublic
 * 功能：向MQTT服务器发布传感器数据
 * 数据格式：JSON格式
 * 参数：无（使用全局变量SensorData和Led_Controller）
 * 返回值：无
 */
void mqttPublic(void)
{
    char Send[256];    // 临时发送缓冲区
    
    // 用sprintf格式化JSON数据
    sprintf(Send,
        "{\"sensor1\":%10.6f,\"sensor2\":%10.6f,\"sensor3\":%d}",
        SensorData.longitudeVal,      // 经度值（浮点数，保留6位小数）
        SensorData.latitudeVal,       // 纬度值（浮点数，保留6位小数）
        Led_Controller.CurrentMode    // LED当前模式（整数）
    );
    // 结果示例：{"sensor1": 123.456789,"sensor2":  45.678901,"sensor3":1}
    
    // 调用MQTT QoS0发布函数
    mqtt_PublishQs0(strMqtt_Inof.pub, Send, strlen(Send));
    // 参数1：发布主题（如"sub123456"）
    // 参数2：消息内容（JSON字符串）
    // 参数3：消息长度
}
```

#### 7.14 Scheduled() 定时发布函数

```c
/**
 * 函数名：Scheduled
 * 功能：定时触发MQTT数据发布（1秒周期）
 * 实现逻辑：
 *   1. 每隔1秒设置发布标志位
 *   2. 当标志位有效且网络/订阅状态就绪时执行发布
 * 参数：无
 * 返回值：无
 */
void Scheduled(void)
{
    static unsigned int mqtt_pub_cnt = 0;   // 发布周期计时器（单位：毫秒）
                                             // static确保变量在函数调用间保持值
    
    // ==== 定时器逻辑：每秒触发一次标志位 ====
    if(get_sys_tick() - mqtt_pub_cnt >= 1000)    // 检测1秒间隔
    {
        mqtt_pub_cnt = get_sys_tick();        // 重置计时器
        System.mqttflag = 1;                  // 设置MQTT发布触发标志
    }
    
    // ==== 发布执行逻辑 ====
    if(System.mqttflag)    // 如果标志位被置1
    {
        // 检查网络和订阅状态
        if(strEsp8266_info.Net_stu > NET_CIPSEND && SubscribePack_flag)
        // Net_stu > NET_CIPSEND 表示TCP连接已建立并进入透传模式
        // SubscribePack_flag = 1 表示已成功订阅主题
        {
            mqttPublic();    // 执行MQTT数据发布
        }
        
        System.mqttflag = 0;    // 无论是否发布成功，都清除标志位
                                 // 这样下次1秒计时器会再次触发
    }
}
```

#### 7.15 MqttCon_Display() 和 Topic_Display() OLED显示

```c
/**
 * 函数名：MqttCon_Display
 * 功能：在OLED显示屏上显示MQTT连接参数和状态
 * 显示内容：
 *   - NET: 网络连接状态（数字）
 *   - MQ_CNT: MQTT连接状态（0=未连接, 1=已连接）
 *   - Name: WiFi热点名称
 *   - Pass: WiFi密码
 * 提示：NET=5表示没开热点，NET=9表示热点没有网络
 */
void MqttCon_Display(void)
{
    oled_ShowString(0, 0, (unsigned char *)"NET:", 16);     // 第0行显示"NET:"
    oled_ShowString(0, 2, (unsigned char *)"MQ_CNT", 16);   // 第2行显示"MQ_CNT"
    OLED_ShowDNum(60, 0, strEsp8266_info.Net_stu, 16);      // 显示网络状态值
    OLED_ShowDNum(60, 2, strEsp8266_info.MQTT_Connect_flag, 16);  // 显示MQTT连接状态
    oled_ShowString(16 * 0, 2 * 2, (u8 *)"Name:", 16);      // 显示WiFi名称标签
    oled_ShowString(16 * 3, 2 * 2, (u8 *)ID, 16);           // 显示WiFi名称值
    oled_ShowString(16 * 0, 2 * 3, (u8 *)"Pass:", 16);      // 显示密码标签
    oled_ShowString(16 * 3, 2 * 3, (u8 *)PASSWORD, 16);     // 显示密码值
}

/**
 * 函数名：Topic_Display
 * 功能：在OLED上显示当前生成的订阅主题和发布主题
 * 目的：因为每次重启都会生成不同的随机主题，需要显示出来让用户知道
 *      用户在APP上需要按照显示屏的主题名来填写
 */
void Topic_Display(void)
{
    oled_ShowString(16 * 0, 2 * 0, (u8 *)"Sub:", 16);       // "Sub:"标签
    oled_ShowString(16 * 0, 2 * 1, (u8 *)strMqtt_Inof.pub, 16);   // 显示发布主题
    oled_ShowString(16 * 0, 2 * 2, (u8 *)"Pub:", 16);       // "Pub:"标签
    oled_ShowString(16 * 0, 2 * 3, (u8 *)strMqtt_Inof.sub, 16);   // 显示订阅主题
}
```

---

### 八、mqtt_drv.h 分析

#### 8.1 文件整体功能

`mqtt_drv.h` 是 **MQTT协议实现的对外接口头文件**。它定义了：
1. MQTT协议相关的全局变量（接收缓冲区、状态标志等）
2. MQTT报文构造函数的声明（Connect、Subscribe、Publish、Ping等）
3. 数据处理函数的声明（接收解析、状态管理等）
4. 阿里云MQTT连接参数（已注释掉，作为参考模板）

#### 8.2 完整代码与注释

```c
#ifndef __MQTT_H__           // 条件编译防止重复包含
#define __MQTT_H__

// ============================================
// 阿里云MQTT连接参数参考模板（已注释掉）
// ============================================
// 这些是被注释掉的阿里云IoT平台参数示例
// 实际项目中需要替换为真实的设备三元组
//
// #define SUBSCRIBE_TOPIC  "/SmartWaterGlasses0329/Sub"
// #define P_TOPIC_NAME     "/SmartWaterGlasses0329/Pub"
// #define DEVICENAME  "k1ctrLnsSlm.MYLED|securemode=2,..."
// #define PRODUCTKEY  "MYLED&k1ctrLnsSlm"
// #define DEVICESECRE "7289b93a675f4a9587bdc39838ce942a..."
// #define BAD         "iot-06z00g5z48h0n39.mqtt.iothub.aliyuncs.com"

// ============================================
// MQTT协议对外接口声明
// ============================================

// 设备密钥长度宏
#define DEVICESECRE_LEN  strlen(DEVICESECRE)

// 硬件参数
extern char ServerIP[128];    // MQTT Broker IP地址
extern int  ServerPort;       // MQTT Broker端口号（默认1883）

// 接收缓冲区声明
extern unsigned char mqtt_RxBuf[7][250];    // 接收缓冲区：7通道 x 250字节
                                             // 设计为环形缓冲区，最多缓存7帧数据
extern unsigned char *mqtt_RxInPtr;         // 接收写入指针（数据从这里写入）
extern unsigned char *mqtt_RxOutPtr;        // 接收读取指针（数据从这里读出）
extern unsigned char *mqtt_RxEndPtr;        // 缓冲区结束地址指针

// MQTT协议状态标志
extern unsigned char ConnectPack_flag;      // MQTT连接成功标志（1=已连接）
extern unsigned char SubscribePack_flag;    // 订阅成功标志（1=已订阅）
extern unsigned char Ping_flag;             // 心跳响应标志

// ============================================
// 函数声明
// ============================================

// 协议报文构造函数
void Mqtt_ConnectMessege(void);              // 构造并发送MQTT CONNECT报文
void Mqtt_Subscribe(char *topic_name, int QoS);   // 构造并发送SUBSCRIBE报文
void mqtt_PublishQs0(char *topic, char *data, int data_len);  // 构造QoS0 PUBLISH报文
void mqtt_Ping(void);                        // 构造并发送PINGREQ心跳包

// 数据处理函数
void mqtt_Content(void);                     // MQTT协议核心处理引擎
void period_get_server(void);                // 周期处理接收数据（20ms间隔）
void mqtt_Dealsetdata_Qs0(unsigned char *redata);   // 解析QoS0 PUBLISH报文
void mqtt_DealTxData(unsigned char *data, int size); // 封装数据并提交发送
void MQTT_Buff_Init(void);                   // 初始化MQTT缓冲区
void Ali_MsessageInit(void);                 // 初始化阿里云MQTT参数

#endif  // __MQTT_H__
```

---

### 九、mqtt_drv.c 分析

#### 9.1 文件整体功能

`mqtt_drv.c` 是 **MQTT协议的核心实现文件**。它完整地实现了MQTT v3.1.1协议的以下功能：
1. **CONNECT** - 连接请求报文构造
2. **SUBSCRIBE** - 订阅主题报文构造
3. **PUBLISH (QoS0)** - 发布消息报文构造和解析
4. **PINGREQ/PINGRESP** - 心跳包处理
5. **CONNACK/SUBACK** - 响应报文处理
6. 环形缓冲区管理
7. 数据封装和解析

#### 9.2 全局变量定义

```c
#include "sys.h"

// 温度/IP阈值（用途需结合具体项目上下文）
unsigned char TEMIP_MAX;
unsigned char TEMIP_MIN;

// MQTT协议数据收发主缓冲区（400字节）
unsigned char mqtt_buff[400];
// 用途：临时存储要发送的MQTT报文

// MQTT状态标志位
unsigned char ConnectPack_flag = 0;      // MQTT连接成功标志（0=未连接, 1=已连接）
unsigned char SubscribePack_flag = 0;    // 主题订阅成功标志（0=未订阅, 1=已订阅）
unsigned char Ping_flag = 0;             // 心跳包状态标志

// 传感器数据相关
int sensorMax = 0;           // 传感器最大值缓存
uint16_t ReadData = 0;       // 传感器原始数据（16位ADC值）
float tempx;                 // 浮点临时变量

// 通用字符串缓冲区
char str[20] = "";           // 临时字符串存储

// MQTT接收缓冲矩阵（7通道 x 250字节 = 1750字节）
// 设计为环形队列，最多缓存7帧数据
unsigned char mqtt_RxBuf[7][250];
unsigned char *mqtt_RxInPtr;     // 写入指针（数据从串口进来，写入这里）
unsigned char *mqtt_RxOutPtr;    // 读取指针（协议处理从这里读取）
unsigned char *mqtt_RxEndPtr;    // 结束指针（用于判断环形队列是否到末尾）

// 设备标识缓冲区
char dest[5] = {0};

// MQTT连接参数（从mqtt_param_init填充）
char clientid[128];       // 客户端ID（如"Device_123456"）
int  clientid_size;       // 客户端ID实际长度
char lex[256];            // 主题存储
char username[128];       // MQTT用户名
int  username_size;
char passward[128];       // MQTT密码
int  passward_size;
```

#### 9.3 mqtt_TxData() 底层发送函数

```c
/**
 * 函数名：mqtt_TxData
 * 功能：通过串口逐字节发送MQTT协议数据包
 * 参数：data - 数据包指针
 *       协议格式：data[0]和data[1]为长度高位/低位字节，后续为实际数据
 * 说明：这个函数被mqtt_DealTxData调用，添加长度头后发送
 */
void mqtt_TxData(unsigned char *data)
{
    int i;
    // 解析协议头中的长度字段（大端格式：高字节在前）
    for(i = 1; i <= (data[0] * 256 + data[1]); i++)
    {
        // 逐字节发送有效数据（从data[2]开始）
        send_data_to_dev((char *)&data[i + 1], 1);
        // 调用esp8266_drv.c中的底层发送函数
    }
}
```

#### 9.4 MQTT_Buff_Init() 缓冲区初始化

```c
/**
 * 函数名：MQTT_Buff_Init
 * 功能：初始化MQTT协议相关缓冲区并启动连接流程
 * 调用时机：ESP8266完成AT指令流程、进入透传模式后调用
 */
void MQTT_Buff_Init(void)
{
    strEsp8266_info.MQTT_Connect_flag = 0;   // 重置MQTT连接状态
    
    // 接收缓冲区环形队列初始化
    mqtt_RxInPtr = mqtt_RxBuf[0];    // 写入指针指向第一通道
    mqtt_RxOutPtr = mqtt_RxInPtr;    // 读取指针同步（表示空队列）
    mqtt_RxEndPtr = mqtt_RxBuf[6];   // 结束指针指向第七通道（共7通道）
    
    // 初始化阿里云MQTT参数（ClientID、Username、Password）
    Ali_MsessageInit();
    
    // 发送MQTT CONNECT报文，请求建立连接
    Mqtt_ConnectMessege();
}
```

#### 9.5 Mqtt_Subscribe() 订阅主题

```c
/**
 * 函数名：Mqtt_Subscribe
 * 功能：构造并发送MQTT SUBSCRIBE协议报文
 * 
 * MQTT SUBSCRIBE报文格式：
 * | 固定头(1B) | 剩余长度(1-4B) | 报文标识符(2B) | 主题长度(2B) | 主题内容 | QoS(1B) |
 * 
 * 参数：
 *   topic_name - 需要订阅的主题字符串
 *   QoS - 服务质量等级（0/1/2）
 */
void Mqtt_Subscribe(char *topic_name, int QoS)
{
    int payload_size;       // 有效载荷长度
    int remaining_size;     // 剩余长度
    int len;                // 编码字节
    int all_size = 1;       // 当前写入位置（从mqtt_buff[1]开始，[0]留给固定头）
    int variable_size = 2;  // 报文标识符固定2字节
    
    // 计算各部分长度
    payload_size = 2 + strlen(topic_name) + 1;   // 主题长度字段(2B) + 主题内容 + QoS(1B)
    remaining_size = variable_size + payload_size; // 剩余长度 = 报文标识符 + 有效载荷
    
    // 固定头：0x82
    // 1000 0010 = 高4位1000=SUBSCRIBE(8)，低4位0010=QoS=1
    mqtt_buff[0] = 0x82;
    
    // 剩余长度编码（MQTT变长编码算法）
    // 规则：每个字节低7位存储数据，最高位表示是否有后续字节
    while(remaining_size > 0)
    {
        len = remaining_size % 128;       // 取低7位
        remaining_size = remaining_size / 128;  // 右移7位
        if(remaining_size > 0)            // 如果还有后续字节
        {
            len = len | 0x80;             // 设置最高位为1
        }
        mqtt_buff[all_size++] = len;      // 写入编码字节，位置后移
    }
    
    // 报文标识符（固定值0x000A，实际应动态生成避免冲突）
    mqtt_buff[all_size + 0] = 0x00;    // 高位
    mqtt_buff[all_size + 1] = 0x0A;    // 低位
    
    // 主题长度（大端编码）
    mqtt_buff[all_size + 2] = strlen(topic_name) / 256;  // 长度高位
    mqtt_buff[all_size + 3] = strlen(topic_name) % 256;  // 长度低位
    
    // 主题内容
    memcpy(&mqtt_buff[all_size + 4], topic_name, strlen(topic_name));
    
    // QoS等级
    mqtt_buff[all_size + 4 + strlen(topic_name)] = QoS;
    
    // 提交发送（总长度 = 固定头位置 + 报文标识符 + 有效载荷）
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size);
}
```

#### 9.6 Mqtt_ConnectMessege() 连接报文构造

```c
/**
 * 函数名：Mqtt_ConnectMessege
 * 功能：构造并发送MQTT CONNECT协议报文
 * 
 * MQTT CONNECT报文格式（v3.1.1）：
 * | 固定头(1B) | 剩余长度(1-4B) | 协议名长度(2B) | 协议名"MQTT"(4B) |
 * | 协议等级(1B) | 连接标志(1B) | 心跳间隔(2B) |
 * | 客户端ID长度(2B) | 客户端ID | 用户名长度(2B) | 用户名 | 密码长度(2B) | 密码 |
 */
void Mqtt_ConnectMessege(void)
{
    int remaining_size;
    int len;
    int variable_size;
    int payload_size;
    int all_size = 1;       // 从mqtt_buff[1]开始写（[0]留给固定头）
    
    variable_size = 10;     // 协议头固定部分 = 2+4+1+1+2 = 10字节
                            // 协议名长度(2B) + "MQTT"(4B) + 等级(1B) + 标志(1B) + 心跳(2B)
    
    // 有效载荷 = 客户端ID + 用户名 + 密码（各含2字节长度字段）
    payload_size = 2 + clientid_size + 2 + username_size + 2 + passward_size;
    remaining_size = variable_size + payload_size;
    
    // 固定头：0x10
    // 0001 0000 = 高4位0001=CONNECT(1)，低4位保留全0
    mqtt_buff[0] = 0x10;
    
    // 剩余长度编码（变长编码）
    while(remaining_size > 0)
    {
        len = remaining_size % 128;
        remaining_size = remaining_size / 128;
        if(remaining_size > 0)
        {
            len = len | 0x80;
        }
        mqtt_buff[all_size++] = len;
    }
    
    // ==== 协议描述段 ====
    mqtt_buff[all_size + 0] = 0x00;    // 协议名长度高位（固定4字节）
    mqtt_buff[all_size + 1] = 0x04;    // 协议名长度低位
    mqtt_buff[all_size + 2] = 0x4D;    // 'M' ASCII码 = 0x4D
    mqtt_buff[all_size + 3] = 0x51;    // 'Q' ASCII码 = 0x51
    mqtt_buff[all_size + 4] = 0x54;    // 'T' ASCII码 = 0x54
    mqtt_buff[all_size + 5] = 0x54;    // 'T' ASCII码 = 0x54
    mqtt_buff[all_size + 6] = 0x04;    // 协议等级4 = MQTT v3.1.1
    
    // ==== 连接标志段 ====
    mqtt_buff[all_size + 7] = 0xC2;    // 连接标志位
    // 0xC2 = 1100 0010（二进制）
    // bit7 = 1: 清理会话（Clean Session）
    // bit6 = 1: 遗嘱标志（Will Flag）- 代码中为1但实际遗嘱未设置
    // bit5-4 = 00: 遗嘱QoS
    // bit3 = 0: 遗嘱保留
    // bit2 = 0: 密码标志 - 实际为0？但后面有密码... 这里可能有bug
    // bit1 = 1: 用户名标志 - 使用用户名
    // bit0 = 0: 保留
    // 
    // 注：根据MQTT规范，如果bit2(密码标志)=0，则不应包含密码字段
    //     但代码后面确实发送了密码，这里存在不一致
    
    // ==== 心跳间隔 ====
    mqtt_buff[all_size + 8] = 0x00;    // 心跳间隔高位（100秒）
    mqtt_buff[all_size + 9] = 0x64;    // 心跳间隔低位（0x0064 = 100）
    // Keep Alive = 100秒：设备每100秒内必须发一次心跳包
    
    // ==== 客户端ID段 ====
    mqtt_buff[all_size + 10] = clientid_size / 256;
    mqtt_buff[all_size + 11] = clientid_size % 256;
    memcpy(&mqtt_buff[all_size + 12], clientid, clientid_size);
    
    // ==== 用户名段 ====
    mqtt_buff[all_size + 12 + clientid_size] = username_size / 256;
    mqtt_buff[all_size + 13 + clientid_size] = username_size % 256;
    memcpy(&mqtt_buff[all_size + 14 + clientid_size], username, username_size);
    
    // ==== 密码段 ====
    mqtt_buff[all_size + 14 + clientid_size + username_size] = passward_size / 256;
    mqtt_buff[all_size + 15 + clientid_size + username_size] = passward_size % 256;
    memcpy(&mqtt_buff[all_size + 16 + clientid_size + username_size], passward, passward_size);
    
    // 提交完整报文
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size);
}
```

#### 9.7 mqtt_PublishQs0() QoS0消息发布

```c
/**
 * 函数名：mqtt_PublishQs0
 * 功能：构造并发送QoS0级别的MQTT PUBLISH报文
 * 
 * MQTT PUBLISH报文格式（QoS0）：
 * | 固定头(1B) | 剩余长度(1-4B) | 主题长度(2B) | 主题内容 | 消息载荷 |
 * 
 * QoS0特点：
 * - 最多一次投递（发出去就不管了）
 * - 不需要报文标识符
 * - 最快但不可靠（可能丢失）
 * 
 * 参数：
 *   topic    - 发布主题字符串
 *   data     - 消息内容指针
 *   data_len - 消息内容长度
 */
void mqtt_PublishQs0(char *topic, char *data, int data_len)
{
    int all_size = 1;        // 写入位置（从mqtt_buff[1]开始）
    int variable_size;       // 可变头长度
    int payload_size;        // 有效载荷长度
    int remaining_size;      // 剩余长度
    int len;
    
    // 计算各部分长度
    variable_size = 2 + strlen(topic);   // 主题长度字段(2B) + 主题内容
    payload_size = data_len;              // 消息载荷 = data_len
    remaining_size = variable_size + payload_size;
    
    // 固定头：0x30
    // 0011 0000 = 高4位0011=PUBLISH(3)，低4位0000=QoS0, 不保留
    mqtt_buff[0] = 0x30;
    
    // 剩余长度编码
    while(remaining_size > 0)
    {
        len = remaining_size % 128;
        remaining_size = remaining_size / 128;
        if(remaining_size > 0)
        {
            len = len | 0x80;
        }
        mqtt_buff[all_size++] = len;
    }
    
    // 主题长度（大端编码）
    mqtt_buff[all_size + 0] = strlen(topic) / 256;
    mqtt_buff[all_size + 1] = strlen(topic) % 256;
    
    // 主题内容
    memcpy(&mqtt_buff[all_size + 2], topic, strlen(topic));
    
    // 消息载荷
    memcpy(&mqtt_buff[all_size + 2 + strlen(topic)], data, data_len);
    
    // 提交发送
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size);
}
```

#### 9.8 mqtt_DealTxData() 数据封装发送

```c
// 发送数据静态缓冲区
unsigned char Tx_Buff[400];

/**
 * 函数名：mqtt_DealTxData
 * 功能：封装MQTT协议数据并提交发送
 * 封装格式：
 *   | 数据长度高位(2B, 大端) | 原始数据内容 |
 * 
 * 这个函数在MQTT原始数据前添加2字节长度头，然后交给底层发送
 */
void mqtt_DealTxData(unsigned char *data, int size)
{
    Tx_Buff[0] = size / 256;     // 数据长度高位（大端格式）
    Tx_Buff[1] = size % 256;     // 数据长度低位
    memcpy(&Tx_Buff[2], data, size);  // 拷贝协议数据
    
    mqtt_TxData(Tx_Buff);        // 调用底层发送函数
}
```

#### 9.9 mqtt_Ping() 心跳包

```c
/**
 * 函数名：mqtt_Ping
 * 功能：构造并发送MQTT PINGREQ心跳请求包
 * 
 * MQTT PINGREQ报文格式：
 * | 固定头(1B) | 剩余长度(1B) |
 * 
 * 固定头：0xC0 = 1100 0000
 *   - 高4位1100 = PINGREQ报文类型(12)
 *   - 低4位0000 = 保留
 * 
 * 剩余长度：0x00 = 0（没有可变头和载荷）
 * 
 * 心跳机制：
 * - 客户端定期发送PINGREQ
 * - 服务器回复PINGRESP（0xD0）
 * - 如果在KeepAlive*1.5时间内未收到PINGRESP，认为连接断开
 */
void mqtt_Ping(void)
{
    mqtt_buff[0] = 0xC0;   // PINGREQ报文类型
    mqtt_buff[1] = 0x00;   // 剩余长度为0
    mqtt_DealTxData(mqtt_buff, 2);   // 发送2字节心跳包
}
```

#### 9.10 Ali_MsessageInit() 阿里云参数初始化

```c
/**
 * 函数名：Ali_MsessageInit
 * 功能：从strMqtt_Inof结构体复制参数到MQTT连接变量
 * 注意：代码中存在memset参数顺序错误
 *       正确用法：memset(目标地址, 填充值, 长度)
 *       代码中写反了：memset(clientid, 128, 0) 应该为 memset(clientid, 0, 128)
 *       但因为memset的第二个参数是int类型，0填充128字节和128填充0字节效果不同
 *       实际上当第三个参数为0时，memset什么都不做，所以效果等价于没清空
 */
void Ali_MsessageInit()
{
    // 客户端ID
    memset(clientid, 128, 0);   // 有bug：意图清空128字节，实际是memset(clientid, 128, 0)
                                 // 因为长度为0，所以什么都没做
    sprintf(clientid, "%s", strMqtt_Inof.device_name);   // 拷贝设备名
    clientid_size = strlen(clientid);                     // 计算长度
    
    // 用户名
    memset(username, 128, 0);
    sprintf(username, "%s", strMqtt_Inof.key);
    username_size = strlen(username);
    
    // 密码
    memset(passward, 128, 0);
    sprintf(passward, "%s", strMqtt_Inof.secre);
    passward_size = strlen(passward);
}
```

#### 9.11 period_get_server() 周期接收处理

```c
/**
 * 函数名：period_get_server
 * 功能：周期性地将串口接收数据存入环形缓冲区
 * 触发条件：距上次接收 >= 20ms 且 有数据待处理
 * 
 * 缓冲区结构（每单元400字节）：
 * | 长度高位(1B) | 长度低位(1B) | 数据内容(最大398B) |
 */
void period_get_server(void)
{
    // 20ms定时检测：如果在20ms内没有新数据到来，认为一帧数据接收完毕
    if(get_sys_tick() - strEsp8266_rcv.rcv_trick_ms >= 20 && strEsp8266_rcv.rcv_cnt)
    {
        // 拷贝数据到环形缓冲区（偏移2字节留给长度头）
        memcpy(&mqtt_RxInPtr[2], strEsp8266_rcv.buff, strEsp8266_rcv.rcv_cnt);
        
        // 写入长度头（大端格式）
        mqtt_RxInPtr[0] = strEsp8266_rcv.rcv_cnt / 256;   // 长度高位
        mqtt_RxInPtr[1] = strEsp8266_rcv.rcv_cnt % 256;   // 长度低位
        
        // 环形缓冲区指针维护
        mqtt_RxInPtr += 400;   // 移动到下一个400字节单元
        if(mqtt_RxInPtr >= mqtt_RxEndPtr)    // 到达末尾
        {
            mqtt_RxInPtr = mqtt_RxBuf[0];    // 回绕到起始位置
        }
        
        // 清空临时接收区
        strEsp8266_rcv.rcv_cnt = 0;
    }
}
```

#### 9.12 mqtt_Content() MQTT核心处理引擎

```c
/**
 * 函数名：mqtt_Content
 * 功能：MQTT协议核心处理引擎
 * 主要逻辑：
 *   1. 解析接收到的MQTT报文（CONNACK/SUBACK/PINGRESP/PUBLISH）
 *   2. 处理接收到的命令
 */
void mqtt_Content(void)
{
    // ==== 接收缓冲区处理（当读写指针不同时，表示有数据待处理）====
    if(mqtt_RxOutPtr != mqtt_RxInPtr)
    {
        // ---- CONNACK报文处理（0x20 = 连接响应）----
        if(mqtt_RxOutPtr[2] == 0x20)    // mqtt_buff[0][1]是长度头，[2]是MQTT固定头
        {
            switch(mqtt_RxOutPtr[5])    // 连接返回码在第5字节
            {
                case 0x00:   // 连接成功
                    ConnectPack_flag = 1;
                    strEsp8266_info.MQTT_Connect_flag = 1;
                    break;
                case 0x01:   // 协议版本不支持
                case 0x02:   // 客户端标识不合法
                case 0x03:   // 服务端不可用
                case 0x04:   // 用户名密码错误
                case 0x05:   // 未授权
                default:     // 其他错误
                    strEsp8266_info.MQTT_Connect_flag = 0;
                    break;
            }
        }
        // ---- SUBACK报文处理（0x90 = 订阅响应）----
        else if(mqtt_RxOutPtr[2] == 0x90)
        {
            switch(mqtt_RxOutPtr[6])    // 订阅返回码
            {
                case 0x01:   // QoS1订阅成功
                    SubscribePack_flag = 1;
                    Ping_flag = 0;
                    // 订阅成功后，立即发布一次当前数据
                    mqtt_PublishQs0(strMqtt_Inof.pub, lex, strlen(lex));
                    break;
                default:     // 订阅失败
                    strEsp8266_info.MQTT_Connect_flag = 0;
                    break;
            }
        }
        // ---- PINGRESP报文处理（0xD0 = 心跳响应）----
        else if(mqtt_RxOutPtr[2] == 0xD0)
        {
            Ping_flag = 1;    // 心跳响应正常
        }
        // ---- PUBLISH报文处理（0x30 = 服务器推送数据）----
        else if(mqtt_RxOutPtr[2] == 0x30)
        {
            mqtt_Dealsetdata_Qs0(mqtt_RxOutPtr);   // 解析消息载荷
        }
        
        // 环形缓冲区指针维护
        mqtt_RxOutPtr += 400;   // 移动到下一单元
        if(mqtt_RxOutPtr >= mqtt_RxEndPtr)
        {
            mqtt_RxOutPtr = mqtt_RxBuf[0];    // 回绕
        }
    }
    
    // ==== 命令缓冲区处理（收到新命令时）====
    if(is_new_CMD_rx == 1)
    {
        is_new_CMD_rx = 0;    // 清除标志
        
        // 命令：切换到显示模式
        if(strstr((char *)CMD_rx_buff + 2, "Display"))
        {
            oled_Clear();
            mode_selection = 0;
            displayFlag = 0;
            System.Switch3 = 0;
        }
        
        // 命令：切换到防盗模式
        if(strstr((char *)CMD_rx_buff + 2, "Anti_theft_mode"))
        {
            oled_Clear();
            displayFlag = 1;
        }
    }
}
```

#### 9.13 mqtt_Dealsetdata_Qs0() 解析PUBLISH报文

```c
/**
 * 函数名：mqtt_Dealsetdata_Qs0
 * 功能：解析QoS0级别的MQTT PUBLISH报文，提取有效载荷数据
 * 
 * PUBLISH报文结构：
 * | 固定头 | 剩余长度(变长) | 主题长度(2B) | 主题内容 | 消息载荷 |
 * 
 * 解析步骤：
 *   1. 解析剩余长度字段（MQTT变长编码）
 *   2. 计算主题长度
 *   3. 计算消息载荷的位置和长度
 *   4. 提取消息载荷
 */
void mqtt_Dealsetdata_Qs0(unsigned char *redata)
{
    int i, topic_len, cmd_len, cmd_local, temp, temp_len, multiplier;
    unsigned char tempbuff[400];   // 临时缓冲区
    unsigned char *data;            // 指向原始数据偏移2字节后的位置
    
    temp_len = 0;
    i = multiplier = 1;             // 从data[1]开始解析剩余长度
    data = &redata[2];              // 跳过外部长度头（非MQTT标准）
    
    // 步骤1：解析MQTT剩余长度字段（变长编码）
    do
    {
        temp = data[i];                           // 读取当前字节
        temp_len += (temp & 127) * multiplier;    // 取低7位并累加
        multiplier *= 128;                         // 位权倍增
        i++;                                       // 下一个字节
    }
    while((temp & 128) != 0);                      // 最高位为1则继续
    
    // 步骤2：计算主题长度（大端格式）
    topic_len = data[i] * 256 + data[i + 1] + 2;   // +2是因为包含2字节长度字段
    
    // 步骤3：计算消息载荷长度和起始位置
    cmd_len = temp_len - topic_len;                 // 有效载荷长度
    cmd_local = topic_len + i;                      // 有效载荷起始位置
    
    // 步骤4：提取有效载荷
    memcpy(tempbuff, &data[cmd_local], cmd_len);
    mqtt_DealCmdSet(tempbuff, cmd_len);             // 提交给命令处理
}
```

---

### 十、ESP8266 WiFi模块工作流程

#### 10.1 整体流程图

```
上电启动
    |
    v
+----------------------------+
| 状态0: NET_NULL            |  硬件复位ESP8266（拉低PA1 250ms）
| (初始/复位状态)             |  复位后动态填充WiFi连接参数和TCP连接参数
+----------------------------+
    | 复位完成后 Net_stu = 1
    v
+----------------------------+
| 状态1-2: AT测试             |  发送 "AT\r\n" 测试模块是否就绪
| (NET_AT / NET_RST)          |  等待响应 "OK"
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态3: 设置STA模式           |  发送 "AT+CWMODE=1\r\n"
| (NET_RST)                   |  设置为Station模式（作为客户端连WiFi）
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态4: 模块软复位            |  发送 "AT+RST\r\n"
| (NET_CWAUTOCONN)            |  复位使STA模式生效
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态5: 关闭自动重连          |  发送 "AT+CWAUTOCONN=0\r\n"
| (NET_CAJAP)                 |  关闭自动连接，由程序控制
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态6: 连接WiFi             |  发送 "AT+CWJAP=\"ssid\",\"pwd\"\r\n"
| (NET_CIPMUX)                |  等待 "CONNECTED"
|                             |  超时10秒（连接WiFi需要较长时间）
+----------------------------+
    | 收到CONNECTED后 Net_stu++
    v
+----------------------------+
| 状态7: 单连接模式            |  发送 "AT+CIPMUX=0\r\n"
| (NET_CIPMODE)               |  设置为单连接模式
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态8: 透传模式             |  发送 "AT+CIPMODE=1\r\n"
| (NET_CIPSTART)              |  开启透传模式
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态9: 建立TCP连接           |  发送 "AT+CIPSTART=\"TCP\",\"IP\",port\r\n"
| (NET_CIPSEND)               |  连接到MQTT服务器
+----------------------------+
    | 收到OK后 Net_stu++
    v
+----------------------------+
| 状态10: 进入数据模式          |  发送 "AT+CIPSEND\r\n"
| (Net_stu > NET_CIPSEND)     |  进入透传，之后数据直接发送给MQTT服务器
+----------------------------+
    | 收到 ">" 后，进入MQTT通信阶段
    v
+----------------------------+
| MQTT通信阶段                 |  1. 发送MQTT CONNECT报文
| (持续运行)                   |  2. 收到CONNACK后发送SUBSCRIBE
|                             |  3. 订阅成功后开始定时发布数据
|                             |  4. 每3秒发送PING心跳包
|                             |  5. 处理服务器推送的PUBLISH消息
+----------------------------+
    |
    | 如果任何AT指令超时或MQTT断线
    v
  返回 NET_NULL（重新开始）
```

#### 10.2 AT指令详解

| 序号 | AT指令 | 功能 | 预期响应 | 超时 |
|------|--------|------|----------|------|
| 1 | `AT` | 测试模块是否正常 | `OK` | 2s |
| 2 | `AT` | 二次确认 | `OK` | 2s |
| 3 | `AT+CWMODE=1` | 设置为STA模式 | `OK` | 2s |
| 4 | `AT+RST` | 软复位模块 | `OK` | 2s |
| 5 | `AT+CWAUTOCONN=0` | 关闭自动重连 | `OK` | 2s |
| 6 | `AT+CWJAP="ssid","pwd"` | 连接WiFi | `CONNECTED` | 10s |
| 7 | `AT+CIPMUX=0` | 单连接模式 | `OK` | 2s |
| 8 | `AT+CIPMODE=1` | 透传模式 | `OK` | 2s |
| 9 | `AT+CIPSTART="TCP","IP",port` | TCP连接 | `OK` | 2s |
| 10 | `AT+CIPSEND` | 进入发送模式 | `>` | 2s |

---

### 十一、MQTT协议实现原理

#### 11.1 MQTT报文格式总览

MQTT协议数据以"报文（Packet）"为单位，每种报文都有固定格式：

```
所有MQTT报文的通用结构：
+----------------+----------+------------------+
|  固定头(1~5B)  | 可变头   |    有效载荷       |
+----------------+----------+------------------+

固定头 = 报文类型(高4位) + 标志位(低4位)
剩余长度 = 可变头长度 + 有效载荷长度（变长编码）
```

#### 11.2 本项目实现的MQTT报文类型

| 报文类型 | 值 | 方向 | 功能 |
|----------|-----|------|------|
| CONNECT | 0x10 | C->S | 客户端请求连接 |
| CONNACK | 0x20 | S->C | 连接响应 |
| PUBLISH | 0x30 | C<->S | 发布消息 |
| SUBSCRIBE | 0x82 | C->S | 订阅主题 |
| SUBACK | 0x90 | S->C | 订阅响应 |
| PINGREQ | 0xCO | C->S | 心跳请求 |
| PINGRESP | 0xD0 | S->C | 心跳响应 |

#### 11.3 剩余长度变长编码算法

MQTT用1~4个字节编码剩余长度，每个字节的最高位表示是否还有后续字节：

```
编码规则：
- 取值范围 0~127：用1字节，最高位=0
- 取值范围 128~16383：用2字节
  第一个字节 = (长度 % 128) | 0x80
  第二个字节 = 长度 / 128
- 以此类推...

示例：编码长度=300
  300 % 128 = 44 -> 字节1 = 44 | 0x80 = 0xAC
  300 / 128 = 2  -> 字节2 = 2 = 0x02
  结果：[0xAC, 0x02]
  解码：0xAC & 0x7F = 44; 44 + 2*128 = 300 ✓
```

#### 11.4 CONNECT报文详解

```
CONNECT报文（本项目中实际发送的数据）：

位置    值          说明
[0]     0x10        固定头：CONNECT类型
[1]     0x??        剩余长度（变长编码）
[2]     0x00        协议名长度高字节
[3]     0x04        协议名长度低字节 = 4
[4-7]   "MQTT"      协议名（0x4D, 0x51, 0x54, 0x54）
[8]     0x04        协议等级 = 4（v3.1.1）
[9]     0xC2        连接标志
[10]    0x00        KeepAlive高字节
[11]    0x64        KeepAlive低字节 = 100秒
[12-13] client_id_len 客户端ID长度
[14..]  client_id   客户端ID
[...]   username_len 用户名长度
[...]   username    用户名
[...]   password_len 密码长度
[...]   password    密码
```

#### 11.5 PUBLISH报文详解

```
QoS0 PUBLISH报文：

位置    值          说明
[0]     0x30        固定头：PUBLISH, QoS=0
[1]     0x??        剩余长度
[2]     0x00        主题长度高字节
[3]     0x??        主题长度低字节
[4..n]  topic       主题内容
[n+1..] payload     消息载荷（JSON数据）

示例：发布主题"sub123456"，消息"{\"temp\":25}"
固定头：0x30
剩余长度：2 + 10 + 13 = 25（主题长度字段+主题+消息）
主题长度：0x00 0x0A = 10
主题："sub123456" (10字节)
消息：{"temp":25} (13字节)
```

#### 11.6 心跳机制

```
心跳时序图：

STM32                          MQTT服务器
  |                                 |
  |-------- PINGREQ (0xC0 0x00) -->|
  |                                 |
  |<------- PINGRESP (0xD0 0x00) --|
  |                                 |
  |       (3秒后)                   |
  |                                 |
  |-------- PINGREQ (0xC0 0x00) -->|
  |                                 |
  |<------- PINGRESP (0xD0 0x00) --|
  |                                 |

如果连续10秒（KeepAlive * 1.5 = 100 * 1.5 = 150秒，
但代码中自定义为10秒）没有收到任何数据，
则判定连接断开，重新初始化。
```

---

### 十二、整体代码逻辑流程

#### 12.1 程序启动流程

```
main() 函数中的初始化顺序：

1. 系统初始化
   └── sys_init()              // 时钟、中断向量表等
   
2. USART初始化
   ├── My_USART1()             // 串口1初始化（调试，9600bps）
   └── My_USART2()             // 串口2初始化（ESP8266，115200bps）
   
3. ESP8266初始化
   ├── WIFI_RESET_init()       // 复位引脚配置
   └── Mqtt_Parameter_init()   // MQTT参数生成（含随机数）
   
4. 其他外设初始化
   ├── OLED_Init()             // 显示屏
   ├── ADC_Init()              // ADC（用于随机数）
   └── TIM_Init()              // 定时器
   
5. 主循环 while(1)
   └── ESP8266_run_handle()    // 每循环一次执行一次
       ├── Scheduled()          // 定时发布检查
       ├── 状态机处理            // WiFi连接/MQTT通信
       └── MqttCon_Display()    // 显示状态（可选）
```

#### 12.2 数据流向图

```
发送数据流向（STM32 -> MQTT服务器）：

传感器数据
    |
    v
mqttPublic()              // 格式化JSON数据
    |
    v
mqtt_PublishQs0()         // 构造MQTT PUBLISH报文
    |
    v
mqtt_DealTxData()         // 添加2字节长度头
    |
    v
mqtt_TxData()             // 解析长度，逐字节发送
    |
    v
send_data_to_dev()        // 写入USART2寄存器
    |
    v
USART2 -> TX -> PA2 ======> ESP8266 RX
    |
    v
ESP8266（透传模式）
    |
    v
WiFi -> Internet -> MQTT Broker

接收数据流向（MQTT服务器 -> STM32）：

MQTT Broker
    |
    v
Internet -> WiFi
    |
    v
ESP8266 TX ======> PA3 -> RX -> USART2
    |
    v
USART2_IRQHandler()       // 接收中断
    |
    v
USART_rcv_ch()            // 存入接收缓冲区
    |
    v
period_get_server()       // 20ms后存入环形缓冲区
    |
    v
mqtt_Content()            // 解析MQTT报文
    |
    v
应用处理（如控制LED、切换模式等）
```

#### 12.3 关键时序参数

| 参数 | 值 | 说明 |
|------|-----|------|
| USART1波特率 | 9600 | 调试串口 |
| USART2波特率 | 115200 | ESP8266通信 |
| WiFi连接超时 | 10秒 | AT+CWJAP指令 |
| 其他AT指令超时 | 2秒 | |
| AT指令重试次数 | 3次 | |
| 数据帧间隔 | 20ms | period_get_server判断 |
| 心跳包间隔 | 3秒 | mqtt_Ping |
| 订阅重试间隔 | 1秒 | |
| 数据发布间隔 | 1秒 | Scheduled |
| 断线检测时间 | 10秒 | 无数据接收判定断线 |
| MQTT KeepAlive | 100秒 | CONNECT报文中声明 |
| 复位低电平时间 | 250ms | ESP8266硬件复位 |
| 复位后等待时间 | 250ms | 模块启动时间 |

---

### 十三、关键设计要点总结

#### 13.1 设计亮点

1. **状态机设计**：ESP8266的整个连接过程使用状态机实现，代码结构清晰，便于调试和维护
2. **AT指令配置表**：用数组存储AT指令，便于扩展和修改，动态填充参数也很灵活
3. **环形缓冲区**：MQTT接收使用7通道环形缓冲，避免数据丢失
4. **随机数主题**：每次重启生成不同主题，避免多设备冲突
5. **ADC噪声生成随机数**：利用硬件特性生成真随机数，创意巧妙
6. **超时重试机制**：每条AT指令都有独立的超时和重试配置

#### 13.2 潜在问题

1. **memset参数错误**：`memset(clientid, 128, 0)` 应该是 `memset(clientid, 0, 128)`
2. **连接标志不一致**：CONNECT报文中连接标志为0xC2（bit2=0），但代码实际发送了密码字段
3. **缓冲区溢出风险**：部分memcpy操作缺少长度检查
4. **静态变量多线程不安全**：多个函数使用static变量实现状态机，不支持多线程/多实例
5. **重入问题**：ESP8266_run_handle需要在主循环中持续调用，不能被长时间阻塞
6. **中文注释乱码**：代码文件中的中文注释使用GB2312编码，在某些编辑器中显示为乱码
7. **WiFi密码明文存储**：ID和PASSWORD以宏定义明文存储，存在安全风险

#### 13.3 改进建议

1. 添加更多的错误处理和日志输出（通过USART1打印调试信息）
2. 使用看门狗定时器防止程序卡死
3. 对敏感信息进行加密存储
4. 添加OTA（空中升级）功能
5. 使用TLS/SSL加密MQTT通信（端口8883）
6. 优化ADC随机数生成算法（当前算法生成的随机数分布不均匀）
7. 添加更多的AT指令错误码处理（不只是匹配"OK"）

---



---

## 第四章：外设驱动代码详解（OLED、GPS、蜂鸣器等）

> 本章讲解如何控制各种硬件外设。你将学习到：OLED屏幕如何显示文字、GPS模块如何获取定位、蜂鸣器如何报警、按键如何检测、LED如何闪烁、振动传感器如何工作。每个外设都有完整的代码和注释。

---

### 目录

1. [OLED显示屏驱动 (bsp_oled_iic.c / .h / oledFont.h)](#1-oled显示屏驱动)
2. [GPS模块驱动 (gps.c / gps.h)](#2-gps模块驱动)
3. [蜂鸣器驱动 (bsp_beep.c / bsp_beep.h)](#3-蜂鸣器驱动)
4. [延时系统 (delay.c / delay.h)](#4-延时系统)
5. [按键驱动 (key.c / key.h)](#5-按键驱动)
6. [LED驱动 (led.c / led.h)](#6-led驱动)
7. [振动传感器驱动 (shake.c / shake.h)](#7-振动传感器驱动)

---

### 1. OLED显示屏驱动

#### 1.1 文件整体功能说明

**涉及文件**：`bsp_oled_iic.c`、`bsp_oled_iic.h`、`oledFont.h`

**通俗解释**：
想象你有一块很小的电子墨水屏幕（128x64像素），就像智能手表上的那种。要在上面显示文字和图案，你需要：
1. **初始化屏幕** —— 告诉屏幕"醒醒，开始工作了"
2. **发送命令** —— 告诉屏幕"我要在某某位置显示东西"
3. **发送数据** —— 告诉屏幕"每个像素是亮还是暗"

这组代码就是用STM32 microcontroller通过**I2C通信协议**来控制OLED屏幕的全部逻辑。

**什么是I2C？**
I2C（Inter-Integrated Circuit）是一种"两根线就能通信"的协议。就像两个人用摩斯密码交流，只需要一根时钟线（SCL，告诉对方"现在该读数据了"）和一根数据线（SDA，实际传输数据）。因为只需要两根线，所以非常适合引脚少的设备。

**SSD1306控制器**：OLED屏幕内部有一个叫SSD1306的芯片，它是真正的"大脑"，负责接收STM32发来的命令并控制每个像素的亮灭。SSD1306的I2C地址是0x3C（写操作地址0x78）。

---

#### 1.2 bsp_oled_iic.h 头文件分析

```c
#ifndef __OLEDIIC_H
#define __OLEDIIC_H
```
**解释**：这是标准的"头文件卫士"（Header Guard），防止同一个头文件被重复包含导致编译错误。`#ifndef`的意思是"如果没有定义过`__OLEDIIC_H`这个宏，就定义它并处理下面的内容"。

```c
#include "sys.h"  // 依赖系统级头文件(包含STM32寄存器定义)
```
**解释**：`sys.h`是STM32标准库的核心头文件，里面定义了所有STM32的寄存器地址和常用类型别名（如`u8`就是`unsigned char`，`u32`就是`unsigned int`）。

##### 1.2.1 状态宏定义

```c
#define TRUE  1   // 逻辑真值
#define FALSE 0   // 逻辑假值
```
**解释**：在C语言中，0表示"假"，非0表示"真"。这里用TRUE/FALSE让代码更易读。注意：C语言标准库其实已经有定义（`stdbool.h`中的`true`和`false`），但嵌入式开发中常常自己定义以避免引入过多依赖。

##### 1.2.2 硬件引脚配置

```c
#define SCL_1_PORT  GPIOA       // SCL时钟线端口
#define SCL_1_PIN   GPIO_Pin_12 // PA12作为SCL引脚
#define SDA_1_PORT  GPIOA       // SDA数据线端口
#define SDA_1_PIN   GPIO_Pin_11 // PA11作为SDA引脚
```
**解释**：
- **GPIO**（General Purpose Input/Output）通用输入输出，就是芯片上那些可以控制的引脚。
- **GPIOA** 是STM32的第一组GPIO端口，有16个引脚（PA0~PA15）。
- **GPIO_Pin_12** 表示PA12这个引脚。
- SCL（Serial Clock）是I2C的时钟线，SDA（Serial Data）是数据线。
- 这里选择PA12作为时钟线，PA11作为数据线。这两个引脚在STM32F103上也可以复用为USB引脚，所以使用时要注意不要同时启用USB功能。

##### 1.2.3 GPIO电平操作宏

```c
#define SCL_H  SCL_1_PORT->BSRR = SCL_1_PIN   // SCL置高电平
#define SCL_L  SCL_1_PORT->BRR  = SCL_1_PIN   // SCL置低电平
#define SDA_H  SDA_1_PORT->BSRR = SDA_1_PIN   // SDA置高电平
#define SDA_L  SDA_1_PORT->BRR  = SDA_1_PIN   // SDA置低电平
```
**解释**：
- 这些宏通过直接操作STM32的**寄存器**来控制引脚的电平。
- **BSRR**（Bit Set Reset Register）是置位/复位寄存器。往它的低16位写1可以把对应引脚置高电平。
- **BRR**（Bit Reset Register）是复位寄存器，往里面写1可以把对应引脚拉低电平。
- 直接操作寄存器比调用库函数（如`GPIO_SetBits()`）更快，因为省去了函数调用的开销。这在时序敏感的I2C通信中非常重要。
- `->`是C语言中通过指针访问结构体成员的操作符。`GPIOA`实际上是一个指向GPIOA寄存器组的指针。

##### 1.2.4 GPIO输入状态读取宏

```c
#define SCL_read  SCL_1_PORT->IDR & SCL_1_PIN  // 读取SCL电平状态
#define SDA_read  SDA_1_PORT->IDR & SDA_1_PIN  // 读取SDA电平状态
```
**解释**：
- **IDR**（Input Data Register）输入数据寄存器，存储了当前每个引脚的电平状态。
- 用`&`（按位与）操作来提取某个特定引脚的状态。如果结果为0，说明该引脚是低电平；如果非0，就是高电平。

##### 1.2.5 函数声明

```c
void oled_Init(void);                    // 初始化SSD1306
void oled_ColorTurn(uint8_t i);          // 颜色反相控制
void oled_DisplayTurn(uint8_t i);        // 屏幕旋转控制
void oled_Clear(void);                   // 清屏
void oled_Display_On(void);              // 开显示
void oled_Display_Off(void);             // 关显示
void oled_ShowString(unsigned char x, unsigned char y, unsigned char *p, unsigned char Char_Size);  // 显示字符串
void oled_ShowCHinese(unsigned char x, unsigned char y, unsigned char no);  // 显示汉字
void oled_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);  // 显示位图
void oled_ShowNum(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char size2);  // 显示数字
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char Char_Size);  // 显示单个字符
void OLED_ShowDNum(unsigned char x, unsigned char y, int Dnum, unsigned char size1);  // 显示整数
void OLED_ShowFNum(unsigned char x, unsigned char y, float Fnum, unsigned char size1);  // 显示浮点数
```

**extern变量**：
```c
extern unsigned char OLED_IIC_stu;  // I2C操作状态标志（0失败，1成功）
```

---

#### 1.3 bsp_oled_iic.c 源文件分析

##### 1.3.1 基础配置宏

```c
#define OLED_MODE   0      // OLED接口模式（0=I2C，1=SPI）
#define SIZE        8      // 基础字体大小（8x8像素）
#define XLevelL     0x00   // 显存列地址低位起始值
#define XLevelH     0x10   // 显存列地址高位起始值
#define Max_Column  128    // 屏幕最大列数（像素宽度）
#define Max_Row     64     // 屏幕最大行数（像素高度）
#define Brightness  0xFF   // 初始亮度（0x00-0xFF，FF最亮）
#define X_WIDTH     128    // 物理屏幕宽度
#define Y_WIDTH     64     // 物理屏幕高度
#define OLED_CMD    0      // 操作类型：写命令
#define OLED_DATA   1      // 操作类型：写数据
```

**SSD1306显存结构理解**：
SSD1306的显存（GDDRAM，Graphic Display Data RAM）大小是128x64位。但它不是按"一个一个像素"寻址的，而是按**页（Page）**组织的：
- 总共**8页**（Page 0 ~ Page 7），每页对应屏幕的8行像素
- 每页有**128列**，每列1字节（8位），刚好对应8个像素
- 所以总共是 8页 x 128列 x 8位 = 8192位 = 1024字节

##### 1.3.2 I2C时序延时函数

```c
static void I2C_delay(void)
{
    unsigned char i = 5;  // 空循环5次
    while (i)
    {
        i--;
    }
}
```
**解释**：I2C通信对时序有严格要求。这个函数通过空循环产生一个极短的延时（约0.5微秒），确保在改变引脚电平后有足够的稳定时间。`static`关键字表示这个函数只在当前文件内可见，其他文件无法调用。

```c
static void delay5ms(void)
{
    int i = 50;
    while (i)
    {
        i--;
    }
}
```
**解释**：约5毫秒的延时，用于某些需要等待的操作（如EEPROM写入后的等待）。

##### 1.3.3 I2C起始条件

```c
static unsigned char I2C_Start(void)
{
    SDA_H;        // 释放SDA线（拉高）
    SCL_H;        // 释放SCL线（拉高）
    I2C_delay();  // 等待电平稳定
    
    if (!SDA_read)         // 检测SDA线是否为低电平
        return FALSE;      // 总线被其他设备占用
    
    SDA_L;        // 在SCL高电平期间拉低SDA → 这就是I2C的起始信号！
    I2C_delay();
    
    if (SDA_read)          // 检测SDA是否成功拉低
        return FALSE;      // SDA无法被拉低（总线异常）
    
    SDA_L;        // 保持SDA低电平
    I2C_delay();
    return TRUE;  // 起始条件生成成功
}
```

**I2C起始条件详解**（非常重要）：
I2C协议规定，当**SCL为高电平**时，SDA从高电平跳变为低电平，就表示"通信开始"。这就像老师敲黑板说"同学们注意了，我要开始讲课了"。

为什么先检测SDA是否为高？因为I2C是多设备共享的总线，如果别的设备正在通信，SDA可能被拉低。这就像课堂上有人在说话，你得等他们说完才能开始讲课。

##### 1.3.4 I2C停止条件

```c
static void I2C_Stop(void)
{
    SCL_L;        // 先将SCL置低（安全操作）
    I2C_delay();
    SDA_L;        // SDA置低
    I2C_delay();
    SCL_H;        // 拉高SCL
    I2C_delay();
    SDA_H;        // 在SCL高电平期间拉高SDA → 这就是停止信号！
    I2C_delay();
}
```

**I2C停止条件详解**：
与起始条件相反，停止条件是在**SCL为高电平**时，SDA从低电平跳变为高电平。这表示"本次通信结束，总线释放了"。

注意：在拉高SCL之前，SDA必须先拉低。这是为了确保在SCL变高之前SDA不会产生"假的起始信号"。

##### 1.3.5 I2C应答信号（ACK/NACK）

```c
static void I2C_Ack(void)    // 发送ACK（应答）
{
    SCL_L;        // 时钟线置低，允许数据线变化
    I2C_delay();
    SDA_L;        // 数据线置低（ACK信号）
    I2C_delay();
    SCL_H;        // 时钟线置高（对方检测到ACK）
    I2C_delay();
    SCL_L;        // 结束ACK周期
    I2C_delay();
}

static void I2C_NoAck(void)  // 发送NACK（非应答）
{
    SCL_L;
    I2C_delay();
    SDA_H;        // 数据线置高（NACK信号）
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}
```

**ACK/NACK详解**：
在I2C协议中，每传输完8位数据后，第9个时钟周期是**应答位**。接收方把SDA拉低表示"我收到了"（ACK），保持高电平表示"我没收到"或"我不想再收了"（NACK）。

就像课堂上老师讲了一段内容后，会问"听懂了吗？"，学生点头（ACK）或摇头（NACK）。

##### 1.3.6 等待应答

```c
static unsigned char I2C_WaitAck(void)
{
    SCL_L;        // 拉低时钟开始ACK检测
    I2C_delay();
    SDA_H;        // 释放数据线（切换为输入模式，让对方控制）
    I2C_delay();
    SCL_H;        // 时钟上升沿
    I2C_delay();
    
    if (SDA_read)  // 读取SDA状态
    {
        SCL_L;
        I2C_delay();
        return FALSE;  // 高电平 = NACK，对方没应答
    }
    
    SCL_L;
    I2C_delay();
    return TRUE;   // 低电平 = ACK，对方应答了
}
```

**解释**：在发送完一个字节（如设备地址或数据）后，主机需要释放SDA线（拉高），然后给一个时钟脉冲，检测从机是否把SDA拉低。如果拉低了，说明从机收到了数据。

##### 1.3.7 发送字节

```c
static void I2C_SendByte(unsigned char SendByte)
{
    unsigned char i = 8;
    while (i--)          // 循环发送8个bit
    {
        SCL_L;           // 拉低时钟，准备数据
        I2C_delay();
        
        if (SendByte & 0x80)   // 检测最高位（0x80 = 10000000b）
            SDA_H;             // 发送1
        else
            SDA_L;             // 发送0
        
        SendByte <<= 1;   // 左移一位，准备发送下一个bit
        I2C_delay();
        SCL_H;            // 时钟上升沿（从机在此采样）
        I2C_delay();
    }
    SCL_L;                // 最终保持时钟低电平
}
```

**详解**：
I2C协议规定数据**高位在前**（MSB First）。`0x80`是二进制的`10000000`，用按位与`&`操作可以检测最高位是1还是0。

`<<= 1`是左移操作，把数据向左移一位，原来的最高位被移出，最低位补0。这样下一轮循环就会检测下一个bit。

整个流程就是：
1. 准备数据位（设置SDA）
2. 给时钟脉冲（SCL从低到高再到低）
3. 从机在SCL上升沿读取SDA上的数据
4. 重复8次，一个字节发送完成

##### 1.3.8 接收字节

```c
static unsigned char I2C_RadeByte(void)
{
    unsigned char i = 8;
    unsigned char ReceiveByte = 0;
    SDA_H;               // 释放数据线（切换为输入模式）
    while (i--)
    {
        ReceiveByte <<= 1;    // 左移腾出最低位
        SCL_L;                // 拉低时钟开始采样
        I2C_delay();
        SCL_H;                // 时钟上升沿（从机输出数据稳定）
        I2C_delay();
        if (SDA_read)         // 读取SDA电平
        {
            ReceiveByte |= 0x01;  // 最低位置1
        }
    }
    SCL_L;
    return ReceiveByte;
}
```

**详解**：
接收字节与发送相反。主机先释放SDA线（让对方控制），然后给时钟脉冲，在每个脉冲的高电平期间读取SDA的状态。

`ReceiveByte <<= 1`把已接收的数据左移，最低位腾出0。如果SDA为高电平，就用` |= 0x01`把最低位设为1。

##### 1.3.9 OLED写入单字节

```c
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
    if (cmd)    // 数据模式
    {
        OLED_IIC_stu = Single_Write(0x78, 0X40, dat);
    }
    else        // 命令模式
    {
        OLED_IIC_stu = Single_Write(0x78, 0X00, dat);
    }
}
```

**详解**：
- **0x78** 是SSD1306的I2C写地址（7位地址0x3C左移1位，最低位为0表示写操作）。
- **0x00** 是命令寄存器地址，告诉SSD1306"接下来的字节是命令"。
- **0x40** 是数据寄存器地址，告诉SSD1306"接下来的字节是要显示的数据"。
- SSD1306有一个特殊的设计：发送的第一个控制字节（Co=0, D/C#=0/1）决定了后续字节是命令还是数据。

##### 1.3.10 OLED初始化

```c
void oled_Init(void)
{
    // GPIO初始化
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能GPIOA时钟
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 50MHz速率
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;         // SCL
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;         // SDA
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOA, GPIO_Pin_12);  // SCL初始高电平
    GPIO_SetBits(GPIOA, GPIO_Pin_11);  // SDA初始高电平
    
    // 初始化命令序列
    while (!Single_Write(0x78, 0X00, 0xae));  // 先关闭显示直到成功
    Delay_1ms(400);
    
    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭显示
    OLED_WR_Byte(0x00, OLED_CMD); // 设置低列地址
    OLED_WR_Byte(0x10, OLED_CMD); // 设置高列地址
    OLED_WR_Byte(0x40, OLED_CMD); // 设置显示起始行
    OLED_WR_Byte(0xB0, OLED_CMD); // 设置页地址
    OLED_WR_Byte(0x81, OLED_CMD); // 对比度设置命令
    OLED_WR_Byte(0xFF, OLED_CMD); // 对比度最大值
    OLED_WR_Byte(0xA1, OLED_CMD); // 段重映射（水平翻转）
    OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示（非反色）
    OLED_WR_Byte(0xA8, OLED_CMD); // 设置复用率
    OLED_WR_Byte(0x3F, OLED_CMD); // 1/64占空比
    OLED_WR_Byte(0xC8, OLED_CMD); // COM扫描方向（逆向）
    OLED_WR_Byte(0xD3, OLED_CMD); // 显示偏移
    OLED_WR_Byte(0x00, OLED_CMD); // 无偏移
    OLED_WR_Byte(0xD5, OLED_CMD); // 时钟分频
    OLED_WR_Byte(0x80, OLED_CMD); // 建议默认值
    OLED_WR_Byte(0xD8, OLED_CMD); // 区域颜色模式
    OLED_WR_Byte(0x05, OLED_CMD); // 禁用区域颜色
    OLED_WR_Byte(0xD9, OLED_CMD); // 预充电周期
    OLED_WR_Byte(0xF1, OLED_CMD); // Phase1=15, Phase2=1
    OLED_WR_Byte(0xDA, OLED_CMD); // COM引脚配置
    OLED_WR_Byte(0x12, OLED_CMD); // 顺序COM引脚
    OLED_WR_Byte(0xDB, OLED_CMD); // VCOMH电压
    OLED_WR_Byte(0x30, OLED_CMD); // 约0.83*VCC
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵设置
    OLED_WR_Byte(0x14, OLED_CMD); // 使能电荷泵
    OLED_WR_Byte(0xAF, OLED_CMD); // 开启显示
    
    oled_Clear();  // 清屏
}
```

**SSD1306初始化命令详解**：
| 命令 | 值 | 作用 |
|------|-----|------|
| 0xAE | 关显示 | 进入休眠模式，防止初始化过程中的花屏 |
| 0xA4 | - | 从GDDRAM读取显示数据（而非全亮测试模式） |
| 0xA6 | 正常显示 | 1=亮像素（白底黑字用0xA7反色） |
| 0xA8,0x3F | 复用率64 | 设置显示行数为64行 |
| 0xD3,0x00 | 偏移0 | 不偏移 |
| 0x40 | 起始行0 | 从第0行开始显示 |
| 0xA1 | 段重映射 | 左右翻转显示（适配屏幕安装方向） |
| 0xC8 | COM逆向扫描 | 从下到上扫描（适配屏幕安装方向） |
| 0xDA,0x12 | COM配置 | 设置COM引脚的硬件连接方式 |
| 0x81,0xFF | 对比度 | 0xFF=最大亮度 |
| 0xD9,0xF1 | 预充电 | 设置充电时间，影响显示质量 |
| 0xDB,0x30 | VCOMH | 设置COM引脚的取消选择电压 |
| 0x8D,0x14 | 电荷泵 | 内部升压，必须开启否则不显示 |
| 0xAF | 开显示 | 正式点亮屏幕 |

##### 1.3.11 字符显示函数

```c
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char chr, unsigned char Char_Size)
{
    unsigned char c = 0, i = 0;
    c = chr - ' ';    // 计算字库偏移量（空格字符ASCII 32是字库起点）
    
    if (x > Max_Column - 1)    // 坐标超界处理
    {
        x = 0;
        y = y + 2;             // 换行（16px字体占2页）
    }
    
    if (Char_Size == 16)       // 16x8点阵字体
    {
        OLED_Set_Pos(x, y);    // 定位到上半页
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);    // 前8字节 = 上半部分
        
        OLED_Set_Pos(x, y + 1); // 定位到下半页
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA); // 后8字节 = 下半部分
    }
    else                        // 6x8点阵字体
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
    }
}
```

**详解**：
- `chr - ' '`：`F8X16`字库中，第一个字符是空格（ASCII码32），所以要减去32得到字库中的索引。
- `F8X16[c * 16 + i]`：每个16x8字符占16字节，前8字节是上8行，后8字节是下8行。
- `F6x8[c][i]`：每个6x8字符占6字节。

##### 1.3.12 汉字显示函数

```c
void oled_ShowCHinese(unsigned char x, unsigned char y, unsigned char no)
{
    unsigned char t;
    OLED_Set_Pos(x, y);           // 定位到上半页
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA);      // 上半部分16字节
    
    OLED_Set_Pos(x, y + 1);       // 定位到下半页
    for (t = 0; t < 16; t++)
        OLED_WR_Byte(Hzk[2 * no + 1][t], OLED_DATA);  // 下半部分16字节
}
```

**详解**：每个16x16的汉字占32字节（2页 x 16列）。`Hzk`数组中，每个汉字占两行（两个16字节的数组元素），`2*no`是上半部分索引，`2*no+1`是下半部分索引。

---

#### 1.4 oledFont.h 字库文件分析

**整体说明**：这个文件存储了三种字体的"点阵数据"。所谓点阵，就是把字符的形状用一堆0和1表示，1表示亮的像素，0表示暗的像素。

##### 1.4.1 6x8 ASCII字库 (F6x8)

```c
const static unsigned char F6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 空格
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00}, // !
    // ... 更多字符
};
```

**详解**：
- 每个字符占**6字节**，每个字节8位，代表一列8个像素。
- 总共定义了95个ASCII字符（从空格0x20到~0x7E）。
- 例如字符'!'：`0x00, 0x00, 0x00, 0x2f, 0x00, 0x00`
  - `0x2f` = 二进制`00101111`，中间靠下的5个像素点亮，形成感叹号的形状。

##### 1.4.2 8x16 ASCII字库 (F8X16)

```c
const static unsigned char F8X16[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 空格
    // ...
};
```

**详解**：
- 每个字符占**16字节**，前8字节是上8行，后8字节是下8行。
- 宽度为8像素，适合显示更大的ASCII字符。

##### 1.4.3 16x16 汉字字库 (Hzk)

```c
const static unsigned char Hzk[][64] = {
    { /* 32字节数据 */ },  // "汽"
    { /* 32字节数据 */ },  // "车"
    // ... 更多汉字
};
```

**详解**：
- 每个汉字占**32字节**（16x16点阵 = 256位 = 32字节）。
- 排列方式是：前16字节是上半部分（Page0），后16字节是下半部分（Page1）。
- 这个字库包含了"汽车防盗"、"温度"、"湿度"、"光照"、"人体发生震动"等项目中用到的汉字。

---

### 2. GPS模块驱动

#### 2.1 文件整体功能说明

**涉及文件**：`gps.c`、`gps.h`

**通俗解释**：
GPS模块是一个能接收卫星信号并告诉你"你在哪里"的设备。它通过串口（UART）与STM32通信，发送**NMEA格式的文本数据**。这些数据是一行一行的字符串，包含纬度、经度、时间、速度等信息。

**NMEA协议**就像一种约定好的"暗号"，比如：
```
$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```
这行数据的含义是：
- `$GNRMC` —— 消息类型（推荐最小定位信息）
- `123519` —— UTC时间12:35:19
- `A` —— 定位有效
- `4807.038,N` —— 北纬48度07.038分
- `01131.000,E` —— 东经11度31.000分
- `022.4` —— 地面速度22.4节
- `084.4` —— 航向84.4度
- `230394` —— UTC日期1994年3月23日
- `003.1,W` —— 磁偏角3.1度西

---

#### 2.2 gps.h 头文件分析

```c
#ifndef _GPS_H
#define _GPS_H
#include "sys.h"
```

##### 2.2.1 NMEA数据结构体

```c
__packed typedef struct
{
    u32 latitude;    // 纬度（放大100000倍存储，避免小数）
    u8  nshemi;      // 北纬/南纬：'N'=北，'S'=南
    u32 longitude;   // 经度（放大100000倍存储）
    u8  ewhemi;      // 东经/西经：'E'=东，'W'=西
} nmea_msg;
```

**详解**：
- `__packed`：告诉编译器不要对结构体进行字节对齐填充，紧凑存储。这在处理通信协议数据结构时很重要。
- 经纬度用`u32`（无符号32位整数）存储，实际值乘以100000。例如纬度48.12345度就存为4812345。
- 不用浮点数直接存的原因：STM32F103没有硬件浮点单元，浮点运算很慢。

```c
typedef struct Data
{
    float latitude;    // 纬度（浮点值）
    char N_S;          // 南北纬标志
    float longitude;   // 经度（浮点值）
    char E_W;          // 东西经标志
} Data;
```

**详解**：这是给上层应用使用的数据结构，用`float`存储方便直接显示和打印。

```c
extern Data GPS_Data;
extern nmea_msg gpsx;

void NMEA_GNRMC_Analysis(nmea_msg *gpsx, u8 *buf);
void get_gps(void);
```

---

#### 2.3 gps.c 源文件分析

##### 2.3.1 查找逗号位置函数

```c
u8 NMEA_Comma_Pos(u8 *buf, u8 cx)
{
    u8 *p = buf;
    while(cx)
    {
        if(*buf=='*' || *buf<' ' || *buf>'z')
            return 0XFF;   // 遇到'*'或非法字符，说明没有第cx个逗号
        if(*buf == ',')
            cx--;            // 找到一个逗号，计数减1
        buf++;               // 指针后移
    }
    return buf - p;          // 返回第cx个逗号的位置偏移量
}
```

**详解**：
NMEA数据用逗号分隔各个字段。这个函数的作用就是"找到第几个逗号在哪里"。
- 参数`cx`：要找第几个逗号（从1开始计数）
- 返回值：该逗号距离字符串起始位置的偏移量
- 返回0xFF：表示没找到（错误码）

例如字符串`"$GNRMC,123519,A,4807.038"`：
- `NMEA_Comma_Pos(buf, 1)` 返回 6（第1个逗号在第6位）
- `NMEA_Comma_Pos(buf, 2)` 返回 13（第2个逗号在第13位）

##### 2.3.2 幂函数

```c
u32 NMEA_Pow(u8 m, u8 n)
{
    u32 result = 1;
    while(n--)
        result *= m;
    return result;
}
```
**解释**：计算m的n次方。例如`NMEA_Pow(10, 3)`返回1000。用于字符串转数字时的位权计算。

##### 2.3.3 字符串转数字

```c
int NMEA_Str2num(u8 *buf, u8* dx)
{
    u8 *p = buf;
    u32 ires = 0, fres = 0;     // ires=整数部分, fres=小数部分
    u8 ilen = 0, flen = 0, i;
    u8 mask = 0;
    int res;
    
    while(1)   // 第一步：统计整数和小数部分的长度
    {
        if(*p == '-') { mask |= 0X02; p++; }       // 检测到负号
        if(*p == ',' || (*p == '*')) break;        // 遇到结束符
        if(*p == '.') { mask |= 0X01; p++; }       // 遇到小数点
        else if(*p > '9' || (*p < '0'))            // 非法字符
        { ilen = 0; flen = 0; break; }
        
        if(mask & 0X01) flen++;    // 小数点后的位数
        else ilen++;               // 小数点前的位数
        p++;
    }
    
    if(mask & 0X02) buf++;    // 跳过负号
    
    for(i = 0; i < ilen; i++) // 计算整数部分
        ires += NMEA_Pow(10, ilen - 1 - i) * (buf[i] - '0');
    
    if(flen > 5) flen = 5;    // 最多取5位小数
    *dx = flen;               // 返回小数位数
    
    for(i = 0; i < flen; i++) // 计算小数部分
        fres += NMEA_Pow(10, flen - 1 - i) * (buf[ilen + 1 + i] - '0');
    
    res = ires * NMEA_Pow(10, flen) + fres;  // 合并整数和小数
    if(mask & 0X02) res = -res;              // 处理负数
    return res;
}
```

**详解**：这是整个GPS模块最复杂的函数。它把NMEA字符串中的数字（如"4807.038"）转换为整数。

`mask`变量的巧妙用法：
- `mask & 0x01`：表示是否已经遇到小数点
- `mask & 0x02`：表示是否是负数

为什么把小数也存成整数？因为在没有浮点硬件的STM32上，整数运算快得多。调用者可以根据`dx`（小数位数）自己决定怎么处理。

##### 2.3.4 GNRMC数据解析

```c
void NMEA_GNRMC_Analysis(nmea_msg *gpsx, u8 *buf)
{
    u8 *p1, dx;
    u8 posx;
    u32 temp;
    float rs;
    
    p1 = (u8*)strstr((const char *)buf, "$GNRMC");  // 查找$GNRMC字符串
    
    // 解析纬度
    posx = NMEA_Comma_Pos(p1, 3);           // 找到第3个逗号（纬度字段）
    if(posx != 0XFF)
    {
        temp = NMEA_Str2num(p1 + posx, &dx);       // 转换为数字
        gpsx->latitude = temp / NMEA_Pow(10, dx + 2);   // 得到度
        rs = temp % NMEA_Pow(10, dx + 2);               // 得到分
        gpsx->latitude = gpsx->latitude * NMEA_Pow(10, 5) 
                       + (rs * NMEA_Pow(10, 5 - dx)) / 60;  // 转换为度
    }
    
    // 解析北纬/南纬标志
    posx = NMEA_Comma_Pos(p1, 4);
    if(posx != 0XFF) gpsx->nshemi = *(p1 + posx);
    
    // 解析经度
    posx = NMEA_Comma_Pos(p1, 5);
    if(posx != 0XFF)
    {
        temp = NMEA_Str2num(p1 + posx, &dx);
        gpsx->longitude = temp / NMEA_Pow(10, dx + 2);
        rs = temp % NMEA_Pow(10, dx + 2);
        gpsx->longitude = gpsx->longitude * NMEA_Pow(10, 5) 
                        + (rs * NMEA_Pow(10, 5 - dx)) / 60;
    }
    
    // 解析东经/西经标志
    posx = NMEA_Comma_Pos(p1, 6);
    if(posx != 0XFF) gpsx->ewhemi = *(p1 + posx);
}
```

**详解**：

**纬度格式转换**：NMEA中的纬度格式是"度分格式"（ddmm.mmmm），比如4807.038表示"48度07.038分"。需要转换为"度"的十进制格式（48.1173度）。

转换公式：
- 度 = 4807.038 / 100 = 48（整数除法）
- 分 = 4807.038 % 100 = 07.038
- 十进制度 = 48 + 07.038/60 = 48.1173

代码中为了避免浮点运算，全部用整数处理，最后放大100000倍存储。

##### 2.3.5 GPS主获取函数

```c
void get_gps(void)
{
    if(USART1_RX_STA & 0X8000)    // 检查串口接收完成标志
    {
        USART1_RX_STA = 0;        // 清除标志
        NMEA_GNRMC_Analysis(&gpsx, (u8*)USART1_RX_BUF);  // 解析数据
        
        GPS_Data.longitude = (float)((float)gpsx.longitude / 100000);  // 转回浮点数
        GPS_Data.latitude = (float)((float)gpsx.latitude / 100000);
        GPS_Data.N_S = (char)gpsx.nshemi;
        GPS_Data.E_W = (char)gpsx.ewhemi;
    }
}
```

**详解**：
- `USART1_RX_STA & 0X8000`：检查串口接收缓冲区是否收到完整的一帧数据（最高位为1表示接收完成）。
- `USART1_RX_BUF`：串口接收缓冲区，存储了GPS模块发来的一行NMEA数据。
- 最后把放大100000倍的整数转换回浮点数，方便显示和打印。

---

### 3. 蜂鸣器驱动

#### 3.1 文件整体功能说明

**涉及文件**：`bsp_beep.c`、`bsp_beep.h`

**通俗解释**：
蜂鸣器就是一个会"滴滴"叫的小喇叭。在STM32上控制蜂鸣器非常简单——只需要给一个GPIO引脚输出高电平（响）或低电平（不响）即可。这个项目使用的是**有源蜂鸣器**（内部带有振荡电路，只需要通电就会响），而不是无源蜂鸣器（需要输入PWM方波才能响）。

---

#### 3.2 bsp_beep.h 头文件分析

```c
#ifndef __BEEP_H
#define __BEEP_H
#include "sys.h"

/* 端口配置宏 */
#define BEEP_GPIO_CLK    RCC_APB2Periph_GPIOA   // GPIOA时钟
#define BEEP_GPIO_PORT   GPIOA                  // GPIOA端口
#define BEEP_GPIO_PIN    GPIO_Pin_8             // PA8引脚

/* 控制宏 */
#define BEEP_TOGGLE  digitalToggle(BEEP_GPIO_PORT, BEEP_GPIO_PIN)  // 翻转状态
#define BEEP_ON      digitalHi(BEEP_GPIO_PORT, BEEP_GPIO_PIN)      // 打开蜂鸣器
#define BEEP_OFF     digitalLo(BEEP_GPIO_PORT, BEEP_GPIO_PIN)      // 关闭蜂鸣器

void Beep_Init(void);
void Short_Beep(void);

#endif
```

**详解**：
- `digitalToggle` / `digitalHi` / `digitalLo`：这些是项目自定义的宏（在sys.h中定义），用来简化GPIO操作。分别对应：翻转引脚电平、置高电平、置低电平。
- **PA8**：选择的蜂鸣器控制引脚。PA8在STM32上也可以复用为定时器TIM1的通道，这里只用作普通GPIO输出。

---

#### 3.3 bsp_beep.c 源文件分析

```c
#include "sys.h"

void Beep_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;           // 定义GPIO初始化结构体
    RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE); // 使能GPIOA时钟

    GPIO_InitStructure.GPIO_Pin = BEEP_GPIO_PIN;       // 选择PA8
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 50MHz速度

    GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStructure);    // 初始化GPIO

    BEEP_OFF;  // 初始化后默认关闭蜂鸣器
}
```

**详解**：
- **推挽输出（Push-Pull Output）**：STM32的GPIO可以配置为推挽输出或开漏输出。推挽输出可以主动输出高电平和低电平，驱动能力强；开漏输出只能主动拉低，拉高需要外部上拉电阻。
- **50MHz**：GPIO输出速度配置。速度越快，驱动能力越强（可以更快地改变电平），但功耗也略大，EMI（电磁干扰）也可能更大。

```c
/* 蜂鸣器短响一次 */
void Short_Beep(void)
{
    BEEP_ON;        // 打开蜂鸣器
    delay_ms(100);  // 持续100毫秒
    BEEP_OFF;       // 关闭蜂鸣器
}
```
**解释**：最简单的蜂鸣器控制——打开，等100毫秒，关闭。发出一声短促的"滴"。常用于按键提示音或报警提示。

---

### 4. 延时系统

#### 4.1 文件整体功能说明

**涉及文件**：`delay.c`、`delay.h`

**通俗解释**：
延时就是"等一会儿"。在嵌入式开发中，延时是非常基础的功能，比如"LED亮500毫秒然后灭"。STM32有多种延时方式：空循环（简单但不精确）、定时器中断（精确但复杂）、SysTick（系统滴答定时器，既精确又简单）。这个项目中使用的是**SysTick方式**。

**什么是SysTick？**
SysTick是Cortex-M3内核自带的一个24位倒计时定时器。它每收到一个时钟脉冲就减1，减到0时可以产生中断。通过设置初始值，可以实现精确的毫秒级/微秒级延时。

---

#### 4.2 delay.h 头文件分析

```c
#ifndef __DELAY_H
#define __DELAY_H
#include "sys.h"

extern volatile unsigned int sysTickUptime;  // 系统运行时间计数器

void delay_init(void);      // 延时系统初始化
void delay_ms(u16 nms);     // 毫秒延时
void delay_us(u32 nus);     // 微秒延时
void HAL_Delay(uint32_t Delay);      // HAL风格毫秒延时
uint32_t HAL_GetTick(void);           // 获取系统运行毫秒数
unsigned int GetSysTime_us(void);     // 获取系统运行微秒数

#endif
```

**详解**：
- `sysTickUptime`：每次SysTick中断时加1，代表系统运行的毫秒数。
- `volatile`关键字：告诉编译器这个变量可能被中断处理程序修改，不要优化掉对它的读取操作。

---

#### 4.3 delay.c 源文件分析

##### 4.3.1 获取微秒级时间

```c
volatile unsigned int sysTickUptime;  // 每毫秒+1
#define usTicks (SystemCoreClock / 1000000)  // 每微秒的时钟周期数

unsigned int GetSysTime_us(void)
{
    register uint32_t ms, cycle_cnt;
    do
    {
        ms = sysTickUptime;          // 读取当前毫秒数
        cycle_cnt = SysTick->VAL;    // 读取SysTick当前计数值
    } while (ms != sysTickUptime);   // 如果读取过程中发生了中断，重新读取
    
    return ((ms * 1000) + (1000 - cycle_cnt / usTicks));  // 转换为微秒
}
```

**详解**：
这是一个非常巧妙的函数，实现了**微秒级精度**的时间获取。

- `SysTick->VAL`：SysTick的当前值寄存器，从设置的初始值递减到0。
- `SystemCoreClock`：系统主频，STM32F103通常为72MHz（72000000）。`usTicks` = 72，表示每微秒有72个时钟周期。
- `do-while`循环解决了一个**并发问题**：如果在读取`ms`和`cycle_cnt`之间发生了SysTick中断，`ms`的值就可能不一致。检测到`ms`变化时重新读取，确保原子性。
- 返回值计算：`ms * 1000`是把毫秒转微秒，`1000 - cycle_cnt/usTicks`是当前毫秒内已经过去的微秒数。

##### 4.3.2 延时初始化

```c
void delay_init()
{
    uint32_t prioritygroup = 0x00U;
    
    SysTick_Config(SystemCoreClock / 1000U);  // 配置为每1毫秒中断一次
    
    prioritygroup = NVIC_GetPriorityGrouping();  // 获取中断优先级分组
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(prioritygroup, 0, 0));  // 设置最高优先级
}
```

**详解**：
- `SysTick_Config(SystemCoreClock / 1000)`：配置SysTick每1/1000秒（即每1毫秒）产生一次中断。
- `NVIC_SetPriority(SysTick_IRQn, ...)`：设置SysTick中断为最高优先级。因为延时函数可能被中断嵌套调用，高优先级确保了延时的准确性。

##### 4.3.3 微秒延时

```c
void delay_us(u32 nus)
{
    uint32_t tickstart = GetSysTime_us();   // 记录开始时间
    uint32_t wait = nus;
    
    while ((GetSysTime_us() - tickstart) < wait);  // 等待直到时间到达
}
```

**详解**：
这是一种"忙等待"（busy-wait）方式的延时。CPU在while循环中不断检查时间，直到达到目标时间。优点是精确，缺点是CPU在这段时间内不能做其他事情。

##### 4.3.4 HAL风格延时

```c
uint32_t HAL_GetTick(void)
{
    return sysTickUptime;  // 返回系统运行的毫秒数
}

void HAL_Delay(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;
    
    while ((HAL_GetTick() - tickstart) < wait);
}

void delay_ms(u16 nms)
{
    HAL_Delay(nms);  // 毫秒延时就是调用HAL_Delay
}
```

**详解**：
- `HAL_GetTick`返回系统上电以来经过的毫秒数。这是一种"时间戳"式的延时方式，不像空循环那样依赖于CPU主频。
- `HAL_Delay`是STM32 HAL库的标准延时函数，这里做了兼容实现。

---

### 5. 按键驱动

#### 5.1 文件整体功能说明

**涉及文件**：`key.c`、`key.h`

**通俗解释**：
按键是最简单的人机交互方式。但按键有一个著名的物理问题——**机械抖动**。当你按下一个按键时，内部的金属触点不是立刻稳定接触的，而是会"抖动"几次（持续约10~20毫秒）。如果不处理抖动，单片机可能会认为你按了好几下。

这个项目实现了4个按键的驱动，采用**软件消抖**的方式。

---

#### 5.2 key.h 头文件分析

```c
#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

extern u8 isKey1, isKey2, isKey3, isKey4;  // 按键状态标志（供外部查询）

/* 端口配置宏 */
#define KEY_GPIO_CLK     RCC_APB2Periph_GPIOB
#define KEY1_GPIO_PORT   GPIOB
#define KEY1_GPIO_PIN    GPIO_Pin_12
#define KEY2_GPIO_PORT   GPIOB
#define KEY2_GPIO_PIN    GPIO_Pin_13
#define KEY3_GPIO_PORT   GPIOB
#define KEY3_GPIO_PIN    GPIO_Pin_14
#define KEY4_GPIO_PORT   GPIOB
#define KEY4_GPIO_PIN    GPIO_Pin_15

/* 读取按键电平的宏 */
#define KEY1  GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2  GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN)
#define KEY3  GPIO_ReadInputDataBit(KEY3_GPIO_PORT, KEY3_GPIO_PIN)
#define KEY4  GPIO_ReadInputDataBit(KEY4_GPIO_PORT, KEY4_GPIO_PIN)

void KEY_Init(void);
void Key1Press(void);
void Key2Press(void);
void Key3Press(void);
void Key4Press(void);
void KeyScan(void);

#endif
```

**详解**：
- `GPIO_ReadInputDataBit`：读取指定GPIO引脚的输入电平状态，返回0（低电平）或1（高电平）。
- 4个按键连接在PB12~PB15上，采用**上拉输入**模式——默认高电平，按下时变为低电平。

---

#### 5.3 key.c 源文件分析

##### 5.3.1 按键初始化

```c
u8 isKey1 = 0, isKey2 = 0, isKey3 = 0, isKey4 = 0;  // 按键状态标志

void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(KEY_GPIO_CLK, ENABLE);  // 使能GPIOB时钟

    GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_PIN | KEY2_GPIO_PIN 
                                | KEY3_GPIO_PIN | KEY4_GPIO_PIN;  // 4个按键引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入模式

    GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);
    GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);
    GPIO_Init(KEY3_GPIO_PORT, &GPIO_InitStructure);
    GPIO_Init(KEY4_GPIO_PORT, &GPIO_InitStructure);
}
```

**详解**：
- **上拉输入（IPU，Input Pull-Up）**：STM32内部有一个电阻把引脚拉到高电平。按键的另一端接地，按下时引脚被拉低。这样只需要一根信号线+地线就能检测按键。
- 四个按键都用同一个`GPIO_InitStructure`配置，但因为它们在不同的端口，所以需要分别调用`GPIO_Init`。

##### 5.3.2 按键事件处理

```c
void Key1Press(void)
{
    if(isKey1 == 0)
        isKey1 = 1;    // 置位标志位，表示按键1被按下过
}

// Key2Press, Key3Press, Key4Press 同理...
```

**详解**：
这些函数只负责"标记"按键被按下了。具体的响应动作由上层代码根据`isKey1`~`isKey4`的值来处理。

##### 5.3.3 按键扫描（核心消抖逻辑）

```c
void KeyScan(void)
{
    static int keyCount = 0;    // 消抖计数器
    static int keyState = 0;    // 按键状态机（0=等待按下，1=已按下）
    
    if(KEY1 == 0 && keyState == 0)    // KEY1被按下且当前处于等待状态
    {
        keyCount++;
        if(keyCount > 2 && KEY1 == 0 && keyState == 0)  // 连续3次检测到按下
        {
            Key1Press();     // 确认按键有效
            keyState = 1;    // 切换到"已按下"状态
        }
    }
    else if(KEY2 == 0 && keyState == 0)
    {
        keyCount++;
        if(keyCount > 2 && KEY2 == 0 && keyState == 0)
        {
            Key2Press();
            keyState = 1;
        }
    }
    // KEY3、KEY4类似...
    
    else if(KEY1 == 1 && KEY2 == 1 && KEY3 == 1 && KEY4 == 1 && keyState == 1)
    {
        keyCount = 0;    // 所有按键都松开了，重置计数器
        keyState = 0;    // 回到"等待按下"状态
    }
}
```

**详解**：

**消抖原理**：
这个函数需要被定时调用（比如每10毫秒调用一次）。`keyCount > 2`意味着连续3次（即30毫秒）都检测到按键为低电平，才认为是真正的按键按下。

**状态机**：
- `keyState = 0`：等待按键按下的状态
- `keyState = 1`：按键已确认按下，等待松手

为什么要等松手？如果不等松手就重置状态，按住按键不放就会连续触发多次。只有所有按键都恢复高电平（松开），才把状态重置为0。

---

### 6. LED驱动

#### 6.1 文件整体功能说明

**涉及文件**：`led.c`、`led.h`

**通俗解释**：
LED（发光二极管）是最常见的输出指示设备。这个项目的LED驱动不仅支持简单的开关，还支持**多种闪烁模式**（常亮、常灭、慢闪、快闪），使用状态机设计模式来实现。

---

#### 6.2 led.h 头文件分析

```c
#ifndef __LED_H__
#define __LED_H__
#include "sys.h"

/* LED引脚定义（GPIOB）*/
#define LED_GPIO_PIN1   GPIO_Pin_5   // LED1 - PB5
#define LED_GPIO_PIN2   GPIO_Pin_6   // LED2 - PB6
#define LED_GPIO_PIN3   GPIO_Pin_7   // LED3 - PB7
#define LED_GPIO_PORT   GPIOB
#define LED_GPIO_CLK    RCC_APB2Periph_GPIOB

/* LED控制宏 */
#define LED1_OFF  GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN1)
#define LED1_ON   GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN1)
#define LED2_OFF  GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN2)
#define LED2_ON   GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN2)
#define LED3_OFF  GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN3)
#define LED3_ON   GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN3)

/* LED状态枚举 */
typedef enum {
    STEADY_OFF = 0,   // 常灭
    STEADY_ON,        // 常亮
    SLOW_BLINK,       // 慢闪（亮500ms，灭1500ms）
    FAST_BLINK        // 快闪（亮100ms，灭100ms）
} LED_STATE_Typedef;

/* LED控制结构体 */
typedef struct {
    LED_STATE_Typedef CurrentMode;   // 当前模式
    uint8_t LED_State;               // 当前状态 0=OFF, 1=ON
    uint32_t LastTick;               // 上次状态改变时间戳
    uint32_t Duration_ON;            // 亮灯持续时间
    uint32_t Duration_OFF;           // 灭灯持续时间
} LED_CONTROLLER;

extern LED_CONTROLLER Led_Controller;

void LED_GPIO_Config(void);
void LED_SetBlinkMode(LED_STATE_Typedef mode);
void LED_Update(void);

#endif
```

**详解**：
- `GPIO_ResetBits`：把指定引脚设为低电平（LED灭，假设LED是低电平点亮还是高电平点亮取决于硬件接法。这里是GPIO_SetBits=ON即高电平点亮）。
- `enum`枚举：定义了一组命名的常量，比直接用数字更直观。
- `LED_CONTROLLER`结构体：封装了LED的所有控制信息，实现了面向对象的设计思想。

---

#### 6.3 led.c 源文件分析

##### 6.3.1 GPIO初始化

```c
LED_CONTROLLER Led_Controller;  // 定义LED控制结构体变量

void LED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE);  // 使能GPIOB时钟

    GPIO_InitStruct.GPIO_Pin = LED_GPIO_PIN1 | LED_GPIO_PIN2 | LED_GPIO_PIN3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    LED1_OFF;  // 初始关闭所有LED
    LED2_OFF;
    LED3_OFF;
}
```

##### 6.3.2 设置闪烁模式

```c
void LED_SetBlinkMode(LED_STATE_Typedef mode)
{
    Led_Controller.CurrentMode = mode;
    Led_Controller.LastTick = HAL_GetTick();  // 记录当前时间

    switch(mode) {
        case STEADY_OFF:
            LED3_OFF;
            Led_Controller.LED_State = 0;
            break;
        
        case STEADY_ON:
            LED3_ON;
            Led_Controller.LED_State = 1;
            break;
        
        case SLOW_BLINK:
            Led_Controller.Duration_ON = 500;    // 亮500ms
            Led_Controller.Duration_OFF = 1500;  // 灭1500ms
            LED3_ON;
            Led_Controller.LED_State = 1;
            break;
        
        case FAST_BLINK:
            Led_Controller.Duration_ON = 100;    // 亮100ms
            Led_Controller.Duration_OFF = 100;   // 灭100ms
            LED3_ON;
            Led_Controller.LED_State = 1;
            break;
    }
}
```

**详解**：
- `HAL_GetTick()`：获取当前系统运行的毫秒数，作为时间戳。
- 不同模式设置不同的亮/灭持续时间。慢闪适合"待机"提示，快闪适合"警告"提示。

##### 6.3.3 LED状态更新

```c
void LED_Update(void)
{
    if (Led_Controller.CurrentMode == STEADY_OFF || 
        Led_Controller.CurrentMode == STEADY_ON) {
        return;  // 常亮/常灭模式不需要更新
    }

    uint32_t Current_Tick = HAL_GetTick();
    
    // 根据当前状态选择要等待的时间
    uint32_t required_duration = Led_Controller.LED_State 
                                 ? Led_Controller.Duration_ON 
                                 : Led_Controller.Duration_OFF;

    if (Current_Tick - Led_Controller.LastTick >= required_duration) {
        if (Led_Controller.LED_State) {
            LED3_OFF;
            Led_Controller.LED_State = 0;
        } else {
            LED3_ON;
            Led_Controller.LED_State = 1;
        }
        Led_Controller.LastTick = Current_Tick;  // 更新时间戳
    }
}
```

**详解**：

**非阻塞闪烁设计**：
这个函数的设计非常巧妙——它不会"停在那里等"。每次调用时，它只检查"时间到了没有"。如果没有到，立即返回。这被称为**非阻塞式**设计。

`LED_Update()`需要在主循环中被反复调用（比如每10毫秒），由它来判断是否该切换LED状态。这样CPU在等待LED切换的同时可以做其他事情。

`Current_Tick - Led_Controller.LastTick`：这是计算时间差的标准写法。即使`HAL_GetTick()`的计数溢出了（从0xFFFFFFFF回到0），这个减法仍然能正确工作，因为是无符号数运算。

---

### 7. 振动传感器驱动

#### 7.1 文件整体功能说明

**涉及文件**：`shake.c`、`shake.h`

**通俗解释**：
振动传感器就像一个"感受到震动就会醒"的小精灵。它内部有一个弹簧连接的触点，当传感器受到震动时，触点会短暂接通，输出一个低电平信号。STM32只需要读取GPIO引脚的电平，就能知道"有没有震动发生"。

---

#### 7.2 shake.h 头文件分析（从shake.c推断）

```c
#ifndef __SHAKE_H
#define __SHAKE_H
#include "sys.h"

#define Shake_GPIO_PORT  GPIOA
#define Shake_GPIO_PIN   GPIO_Pin_4

void Shake_Init(void);
uint8_t Shake_GetValue(void);

#endif
```

---

#### 7.3 shake.c 源文件分析

```c
#include "sys.h"

void Shake_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 使能GPIOA时钟
    
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入模式
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;              // PA4引脚
    
    GPIO_Init(Shake_GPIO_PORT, &GPIO_InitStruct);
}

uint8_t Shake_GetValue(void)
{
    return GPIO_ReadInputDataBit(Shake_GPIO_PORT, Shake_GPIO_PIN);
}
```

**详解**：

**浮空输入模式（IN_FLOATING）**：
引脚既不接内部上拉电阻，也不接内部下拉电阻，完全"悬空"。这适合外部信号本身就是明确的数字信号（高电平或低电平）的场合。

为什么不使用上拉输入？因为振动传感器模块（如SW-420）通常内部已经集成了比较器和上拉电阻，输出是明确的0/1电平，所以直接用浮空输入即可。

**PA4引脚**：注意这个引脚同时也是STM32的SPI1_NSS（片选信号）和DAC_OUT1（DAC输出），在使用时要避免冲突。

**Shake_GetValue**：
返回值是0或1。通常振动传感器在静止时输出高电平（1），震动时输出低电平（0）。上层代码可以定时检测这个值，当检测到0时就认为"发生了震动"。

---

### 附录：各外设连接汇总表

| 外设 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| OLED SCL | PA12 | 推挽输出 | I2C时钟线 |
| OLED SDA | PA11 | 推挽输出 | I2C数据线 |
| 蜂鸣器 | PA8 | 推挽输出 | 高电平=响 |
| 振动传感器 | PA4 | 浮空输入 | 低电平=有震动 |
| LED1 | PB5 | 推挽输出 | 高电平=亮 |
| LED2 | PB6 | 推挽输出 | 高电平=亮 |
| LED3 | PB7 | 推挽输出 | 高电平=亮 |
| KEY1 | PB12 | 上拉输入 | 低电平=按下 |
| KEY2 | PB13 | 上拉输入 | 低电平=按下 |
| KEY3 | PB14 | 上拉输入 | 低电平=按下 |
| KEY4 | PB15 | 上拉输入 | 低电平=按下 |
| GPS | USART1 | 串口通信 | 9600bps默认 |

---

### 整体代码架构总结

```
STM32F103
    |
    +-- OLED显示模块 (I2C模拟) -- 显示文字/数字/图案
    |       |
    |       +-- I2C底层通信 (Start/Stop/Ack/发送/接收)
    |       +-- SSD1306命令层 (初始化/定位/开关)
    |       +-- 显示函数层 (字符/汉字/数字/字符串/位图)
    |       +-- 字库数据 (F6x8/F8X16/Hzk)
    |
    +-- GPS模块 (UART串口) -- 获取经纬度位置
    |       |
    |       +-- NMEA协议解析 (GNRMC语句)
    |       +-- 字符串转数字 (度分转十进制度)
    |
    +-- 蜂鸣器 (GPIO输出) -- 声音提示
    +-- LED (GPIO输出) -- 状态指示 (支持闪烁模式)
    +-- 按键 (GPIO输入) -- 人机交互 (4个按键)
    +-- 振动传感器 (GPIO输入) -- 震动检测
    +-- 延时系统 (SysTick) -- 时间基准
```

这个项目的代码体现了典型的**分层设计**思想：
- **硬件抽象层**（GPIO/I2C/UART寄存器操作）
- **协议层**（I2C时序/NMEA解析）
- **应用层**（显示/控制逻辑）

这种分层使得代码易于维护和移植——如果需要换一块不同接口的OLED屏幕，只需要修改底层的I2C函数，上层的显示代码完全不需要改动。



---

## 第五章：Android APP 代码详解

> 本章讲解手机端APP的完整代码。你将学习到：Android APP如何开发、界面如何布局、MQTT如何收发消息、GPS数据如何在手机上显示、报警通知如何实现。即使你没有Android基础，也能看懂每一行代码。

---

### 一、项目概览与整体架构

#### 1.1 APP是什么？

这是一个**汽车防盗监测APP**，安装在用户的Android手机上，通过与云端MQTT服务器通信，
远程监控汽车（或智能小车）的状态。主要功能包括：

| 功能 | 说明 |
|------|------|
| **GPS定位显示** | 显示汽车的经度和纬度坐标 |
| **防盗模式开关** | 开启/关闭防盗监测功能 |
| **振动报警** | 当检测到汽车振动时显示"发生震动" |
| **非法入侵报警** | 当检测到非法入侵时显示"非法入侵"并触发手机震动 |
| **远程MQTT通信** | 通过MQTT协议与小车上的STM32单片机实时通信 |

#### 1.2 系统整体架构图

```
+-------------------+        MQTT协议        +-------------------+
|   Android APP     |  <--subscribe--------- |   MQTT服务器       |
|  (本分析项目)      |  (接收小车数据)         |  47.109.89.8:1883 |
|                   |  ---------publish-->   |                   |
|                   |  (发送控制命令)         |                   |
+-------------------+                      +-------------------+
        ↑                                          ↑
        | WiFi/4G/5G                               | WiFi/4G
        |                                          |
   [用户手机]                                  [智能小车]
                                              (STM32 + 传感器)
                                                  ↑
                                                  |
                                            [GPS模块]
                                            [振动传感器]
                                            [人体红外传感器]
```

#### 1.3 工作流程（简版）

1. **打开APP** → 弹出登录对话框，让用户输入MQTT主题（Sub/Pub）
2. **连接MQTT服务器** → APP连接到云端的MQTT代理服务器
3. **订阅主题** → APP订阅指定Topic，等待小车发送数据
4. **开启防盗模式** → APP向小车发送"Anti_theft_mode"命令
5. **接收数据** → 小车通过MQTT上报GPS坐标、传感器状态
6. **报警处理** → 当传感器检测到异常时，APP显示报警并震动

---

### 二、核心配置文件分析

---

#### 2.1 settings.gradle（项目设置文件）

**文件路径**：`settings.gradle`

```gradle
pluginManagement {
    repositories {
        google()              // Google的Maven仓库
        mavenCentral()        // Maven中央仓库
        gradlePluginPortal()  // Gradle插件仓库
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()              // Google仓库（放AndroidX等库）
        mavenCentral()        // Maven中央仓库（放第三方开源库）
    }
}
rootProject.name = "Smart home"    // 项目名称
include ':app'                      // 包含app模块
```

**通俗解释**：
- `settings.gradle` 是Gradle构建系统的"目录索引"，告诉编译器这个项目叫什么名字、包含哪些模块
- `google()` 和 `mavenCentral()` 是"软件商店"，APP需要的各种第三方库（如MQTT库）从这里下载
- `FAIL_ON_PROJECT_REPOS` 表示不允许在模块级别单独设置仓库，统一从这里管理
- `rootProject.name = "Smart home"` 给项目起名叫"Smart home"（智能家居）
- `include ':app'` 表示项目只有一个叫`app`的模块

---

#### 2.2 gradle.properties（Gradle属性配置）

**文件路径**：`gradle.properties`

```properties
# 指定JVM参数，用于调整内存设置
org.gradle.jvmargs=-Xmx2048m -Dfile.encoding=UTF-8

# 使用AndroidX包结构
android.useAndroidX=true

# 启用R类的非传递性命名空间
android.nonTransitiveRClass=true

# 覆盖路径检查（解决某些路径问题）
android.overridePathCheck=true
```

**通俗解释**：
- `org.gradle.jvmargs=-Xmx2048m`：给编译器分配最多2GB内存，防止编译大型项目时内存不足
- `android.useAndroidX=true`：**重要！** 使用AndroidX库而不是旧版的Support Library。AndroidX是Google推出的新版支持库，包含所有UI组件和工具类
- `android.nonTransitiveRClass=true`：优化资源类（R类）的编译，只包含当前模块声明的资源
- `android.overridePathCheck=true`：允许覆盖某些路径检查，避免Windows上的长路径问题

---

#### 2.3 project/build.gradle（项目级构建配置）

**文件路径**：`build.gradle`（项目根目录）

```gradle
// 顶级构建文件，可以添加所有子项目/模块共用的配置选项
plugins {
    id 'com.android.application' version '8.0.1' apply false
    id 'com.android.library' version '8.0.1' apply false
}
```

**通俗解释**：
- 这是整个项目的"总配置文件"
- `com.android.application` version `8.0.1`：使用Android Gradle插件8.0.1版本
  - 这个插件负责将Java/XML代码编译成APK安装包
  - `apply false` 表示在这里只声明插件版本，不在根项目直接应用
  - 实际应用是在`app/build.gradle`中
- `com.android.library`：如果需要创建库模块也使用同样版本

---

#### 2.4 app/build.gradle（APP模块构建配置）

**文件路径**：`app/build.gradle`

```gradle
plugins {
    id 'com.android.application'   // 应用Android应用插件（这里才真正启用）
}

android {
    namespace 'cacom.example.smarthome'    // 应用的包名命名空间
    compileSdk 33                          // 使用Android 13 (API 33) 编译

    defaultConfig {
        applicationId "cacom.example.smarthome"   // APP唯一标识符
        minSdk 28                                  // 最低支持Android 9.0
        targetSdk 33                               // 目标Android版本
        versionCode 1                              // 版本号（数字，用于升级）
        versionName "1.0"                          // 版本名称（显示给用户看）
        testInstrumentationRunner "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            minifyEnabled false                                    // 不启用代码压缩
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
        }
    }
    compileOptions {
        sourceCompatibility JavaVersion.VERSION_1_8    // Java 8源代码兼容
        targetCompatibility JavaVersion.VERSION_1_8    // Java 8目标兼容
    }
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.4.1'           // AppCompat支持库
    implementation 'com.google.android.material:material:1.5.0'   // Material Design组件
    implementation 'androidx.constraintlayout:constraintlayout:2.1.3'  // 约束布局
    implementation 'org.eclipse.paho:org.eclipse.paho.client.mqttv3:1.2.5'  // MQTT客户端库
    testImplementation 'junit:junit:4.13.2'                        // 单元测试
    androidTestImplementation 'androidx.test.ext:junit:1.1.3'     // Android测试
    androidTestImplementation 'androidx.test.espresso:espresso-core:3.4.0'  // UI测试
}
```

**通俗解释**：

**基本配置部分**：
- `namespace 'cacom.example.smarthome'`：APP的包名，在Google Play商店中唯一标识这个应用
- `compileSdk 33`：用Android 13的SDK来编译，可以使用Android 13的所有API
- `minSdk 28`：最低支持Android 9.0（API 28），太老的安卓手机装不了
- `targetSdk 33`：告诉系统这个APP是为Android 13设计的
- `versionCode 1` / `versionName "1.0"`：版本号，更新APK时versionCode必须递增

**编译选项部分**：
- `minifyEnabled false`：不启用ProGuard代码混淆（简化调试，发布时通常开启）
- `sourceCompatibility JavaVersion.VERSION_1_8`：使用Java 8语法特性（支持Lambda表达式等）

**依赖库部分（dependencies）**：
| 依赖库 | 用途 |
|--------|------|
| `appcompat:1.4.1` | Android兼容库，让新特性在老Android版本上也能用 |
| `material:1.5.0` | Google的Material Design组件库（按钮、卡片等美观UI） |
| `constraintlayout:2.1.3` | 约束布局，灵活的界面排版方式 |
| `paho.client.mqttv3:1.2.5` | **核心！** Eclipse Paho MQTT客户端库，用于与MQTT服务器通信 |
| `junit:junit:4.13.2` | Java单元测试框架 |

---

#### 2.5 AndroidManifest.xml（应用清单文件）

**文件路径**：`app/src/main/AndroidManifest.xml`

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:tools="http://schemas.android.com/tools">

    <!-- 声明APP需要的权限 -->
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.VIBRATE" />

    <application
        android:allowBackup="true"
        android:dataExtractionRules="@xml/data_extraction_rules"
        android:fullBackupContent="@xml/backup_rules"
        android:icon="@mipmap/log"                    <!-- APP图标 -->
        android:label="@string/app_name"              <!-- APP名称 -->
        android:roundIcon="@mipmap/ic_launcher_round" <!-- 圆形图标 -->
        android:supportsRtl="true"
        android:theme="@style/Theme.SmartHome"        <!-- 应用主题 -->
        tools:targetApi="31">

        <!-- 主Activity声明 -->
        <activity
            android:name=".MainActivity"
            android:exported="true">   <!-- 允许外部应用启动 -->
            <intent-filter>
                <!-- 设置为主入口Activity -->
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>

</manifest>
```

**通俗解释**：

AndroidManifest.xml 是APP的"身份证"和"说明书"，系统在安装和运行APP时会读取这个文件。

**权限声明部分**：
- `INTERNET`：**网络权限**，允许APP访问互联网（连接MQTT服务器必须用）
- `VIBRATE`：**震动权限**，允许APP控制手机震动（报警时用）

> **注意**：Android 6.0以上需要动态申请危险权限，但INTERNET和VIBRATE属于普通权限，在清单中声明即可。

**应用配置部分**：
- `android:icon="@mipmap/log"`：APP图标使用`res/mipmap/`目录下的`log`图片
- `android:label="@string/app_name"`：APP名称引用`strings.xml`中的`app_name`，即"汽车防盗监测"
- `android:theme="@style/Theme.SmartHome"`：使用名为"Theme.SmartHome"的主题样式

**Activity声明部分**：
- `activity`：声明APP有一个界面（Activity）叫`MainActivity`
- `exported="true"`：允许其他应用启动这个Activity
- `intent-filter`中的`MAIN` + `LAUNCHER`：告诉系统这是APP的入口界面，点击桌面图标就启动这个界面

---

### 三、界面布局文件分析

---

#### 3.1 activity_main.xml（主界面布局）

**文件路径**：`app/src/main/res/layout/activity_main.xml`

**文件功能说明**：
这是APP的主界面布局文件，定义了用户打开APP后看到的所有UI元素。
界面采用层次化的布局结构，从上到下依次是：标题栏、GPS数据卡片、防盗模式控制区、状态显示区。

**完整代码与逐行注释**：

```xml
<?xml version="1.0" encoding="utf-8"?>
<!-- 
    ConstraintLayout: 约束布局（根布局）
    这是Android推荐的现代布局方式，可以精确控制元素位置
    match_parent: 宽/高填满父容器（这里就是手机屏幕）
-->
<androidx.constraintlayout.widget.ConstraintLayout 
    xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto"
    xmlns:tools="http://schemas.android.com/tools"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    tools:context=".MainActivity">   <!-- 关联到MainActivity.java -->

    <!-- 
        LinearLayout: 线性布局（根布局内的主要容器）
        orientation="vertical": 子元素垂直排列（从上往下）
        background="@mipmap/bg": 使用bg图片作为背景
    -->
    <LinearLayout
        android:layout_width="match_parent"
        android:layout_height="match_parent"
        android:background="@mipmap/bg"
        android:orientation="vertical">

        <!-- ========== 顶部标题栏 ========== -->
        <!-- RelativeLayout: 相对布局，子元素可以相对定位 -->
        <RelativeLayout
            android:layout_width="match_parent"
            android:layout_height="45dp"      <!-- 高度45dp（约135像素） -->
            android:background="#03A9F4">     <!-- 天蓝色背景 -->

            <!-- TextView: 文本显示控件（标题文字） -->
            <TextView
                android:layout_width="wrap_content"    <!-- 宽度自适应文字长度 -->
                android:layout_height="wrap_content"   <!-- 高度自适应文字高度 -->
                android:text="汽车防盗监测"              <!-- 显示的文字 -->
                android:layout_centerVertical="true"   <!-- 垂直居中 -->
                android:layout_centerHorizontal="true" <!-- 水平居中 -->
                android:textColor="#fff"               <!-- 白色文字 -->
                android:textSize="20sp"/>              <!-- 字号20sp -->
        </RelativeLayout>

        <!-- ScrollView: 可滚动视图容器，内容超出屏幕时可上下滚动 -->
        <ScrollView
            android:layout_width="match_parent"
            android:layout_height="match_parent">

            <!-- 内部再用一个LinearLayout垂直排列所有内容 -->
            <LinearLayout
                android:layout_width="match_parent"
                android:layout_height="match_parent"
                android:orientation="vertical">

                <!-- 空白间隔，20dp高度 -->
                <View
                    android:layout_width="match_parent"
                    android:layout_height="20dp">
                </View>

                <!-- ========== GPS数据卡片区域（第一行）========== -->
                <!-- 水平排列的两个卡片：经度和纬度 -->
                <LinearLayout
                    android:layout_width="match_parent"
                    android:layout_margin="10dp"    <!-- 四边外边距10dp -->
                    android:layout_height="130dp">

                    <!-- ===== 经度卡片（左边） ===== -->
                    <LinearLayout
                        android:layout_width="0dp"
                        android:layout_height="120dp"
                        android:layout_marginRight="10dp"
                        android:background="@drawable/lin_shape"   <!-- 圆角矩形背景 -->
                        android:elevation="8dp"                    <!-- 阴影效果（卡片浮起感） -->
                        android:padding="10dp"                     <!-- 内边距 -->
                        android:layout_weight="1">                 <!-- 权重1，占据一半宽度 -->

                        <!-- 左侧文字区域 -->
                        <LinearLayout
                            android:layout_width="wrap_content"
                            android:layout_height="match_parent"
                            android:orientation="vertical"
                            android:layout_weight="1">

                            <!-- "经度"标签 -->
                            <TextView
                                android:layout_width="match_parent"
                                android:layout_height="40dp"
                                android:gravity="center_vertical"     <!-- 文字垂直居中 -->
                                android:text="经度"
                                android:textColor="#000"              <!-- 黑色文字 -->
                                android:textSize="20sp"
                                android:textStyle="bold"/>            <!-- 粗体 -->

                            <!-- 经度数值显示（代码中动态更新） -->
                            <TextView
                                android:id="@+id/longitudeVal"        <!-- 唯一ID，代码中用findViewById获取 -->
                                android:layout_width="match_parent"
                                android:layout_height="wrap_content"
                                android:text=""                        <!-- 初始为空 -->
                                android:textSize="16sp"
                                android:layout_marginTop="10dp"
                                android:textColor="#03A9F4"/>        <!-- 蓝色数值 -->
                        </LinearLayout>

                        <!-- 右侧图标区域 -->
                        <LinearLayout
                            android:layout_width="50dp"
                            android:gravity="bottom"                  <!-- 底部对齐 -->
                            android:layout_height="match_parent">

                            <!-- 圆形背景 + 经度图标 -->
                            <LinearLayout
                                android:layout_width="50dp"
                                android:layout_height="50dp"
                                android:gravity="center"
                                android:background="@drawable/quan_shape">
                                <ImageView
                                    android:layout_width="35dp"
                                    android:layout_height="35dp"
                                    android:src="@mipmap/jindu"/>     <!-- 经度图标图片 -->
                            </LinearLayout>
                        </LinearLayout>
                    </LinearLayout>

                    <!-- ===== 纬度卡片（右边） ===== -->
                    <LinearLayout
                        android:layout_width="0dp"
                        android:layout_height="120dp"
                        android:layout_marginLeft="10dp"
                        android:background="@drawable/lin_shape"
                        android:elevation="8dp"
                        android:padding="10dp"
                        android:layout_weight="1">

                        <!-- 左侧文字区域 -->
                        <LinearLayout
                            android:layout_width="wrap_content"
                            android:layout_height="match_parent"
                            android:orientation="vertical"
                            android:layout_weight="1">

                            <!-- "纬度"标签 -->
                            <TextView
                                android:layout_width="match_parent"
                                android:layout_height="40dp"
                                android:gravity="center_vertical"
                                android:text="纬度"
                                android:textColor="#000"
                                android:textSize="20sp"
                                android:textStyle="bold"/>

                            <!-- 纬度数值显示（代码中动态更新） -->
                            <TextView
                                android:id="@+id/latitudeVal"
                                android:layout_width="match_parent"
                                android:layout_height="wrap_content"
                                android:text=""
                                android:textSize="16sp"
                                android:layout_marginTop="10dp"
                                android:textColor="#03A9F4"/>
                        </LinearLayout>

                        <!-- 右侧图标区域 -->
                        <LinearLayout
                            android:layout_width="50dp"
                            android:gravity="bottom"
                            android:layout_height="match_parent">

                            <!-- 圆形背景 + 纬度图标 -->
                            <LinearLayout
                                android:layout_width="50dp"
                                android:layout_height="50dp"
                                android:gravity="center"
                                android:background="@drawable/quan_shape">
                                <ImageView
                                    android:layout_width="35dp"
                                    android:layout_height="35dp"
                                    android:src="@mipmap/weidu"/>     <!-- 纬度图标图片 -->
                            </LinearLayout>
                        </LinearLayout>
                    </LinearLayout>
                </LinearLayout>

                <!-- ========== 防盗模式控制区域 ========== -->
                <LinearLayout
                    android:layout_width="match_parent"
                    android:layout_height="45dp"
                    android:gravity="center_vertical">    <!-- 内容垂直居中 -->

                    <!-- 左侧蓝色竖条装饰 -->
                    <View
                        android:layout_width="10dp"
                        android:layout_height="30dp"
                        android:layout_marginLeft="10dp"
                        android:background="@drawable/item_shape">
                    </View>

                    <!-- "防盗模式"标签 -->
                    <TextView
                        android:layout_width="wrap_content"
                        android:layout_height="match_parent"
                        android:layout_marginLeft="10dp"
                        android:gravity="center_vertical"
                        android:text="防盗模式"
                        android:textColor="#000"
                        android:textSize="18sp"
                        android:textStyle="bold"
                        android:layout_weight="1"/>

                    <!-- Switch: 开关控件 -->
                    <Switch
                        android:id="@+id/Anti_theft_mode"
                        android:layout_width="wrap_content"
                        android:layout_height="wrap_content"
                        android:layout_alignParentRight="true"
                        android:layout_centerVertical="true"
                        android:layout_marginRight="10dp"
                        android:thumb="@drawable/setting_button_thumb"   <!-- 开关滑块样式 -->
                        android:track="@drawable/setting_button_track"/> <!-- 开关轨道样式 -->
                </LinearLayout>

                <!-- ========== 状态显示区域（默认隐藏）========== -->
                <LinearLayout
                    android:id="@+id/layout1"
                    android:layout_width="match_parent"
                    android:layout_height="wrap_content"
                    android:layout_margin="10dp"
                    android:background="@drawable/lin_shape"
                    android:elevation="8dp"
                    android:padding="10dp"
                    android:layout_weight="1">

                    <LinearLayout
                        android:layout_width="match_parent"
                        android:layout_height="wrap_content"
                        android:orientation="vertical"
                        android:gravity="center"
                        android:layout_weight="1">

                        <!-- 状态文字显示（如"发生震动""非法入侵"） -->
                        <TextView
                            android:id="@+id/CurrentMode"
                            android:layout_width="wrap_content"
                            android:layout_height="wrap_content"
                            android:gravity="center"
                            android:text=""
                            android:textColor="#03A9F4"
                            android:textSize="16sp"/>
                    </LinearLayout>
                </LinearLayout>

            </LinearLayout>
        </ScrollView>
    </LinearLayout>
</androidx.constraintlayout.widget.ConstraintLayout>
```

**界面布局总结**：

整个界面从上到下分为以下几个区域：

```
+----------------------------------------+
|  [天蓝色标题栏]   汽车防盗监测            |  ← RelativeLayout, 45dp高
+----------------------------------------+
|                                        |
|  [经度卡片]          [纬度卡片]          |  ← 两个并排的卡片
|  经度: X.XXXXXX°   纬度: X.XXXXXX°    |    显示GPS坐标
|  [O图标]            [O图标]            |
|                                        |
|  | 防盗模式                    [开关]   |  ← 防盗模式控制行
|                                        |
|  +----------------------------------+  |
|  |                                  |  |
|  |       发生震动 / 非法入侵          |  ← 状态显示区（默认隐藏）
|  |                                  |  |
|  +----------------------------------+  |
|                                        |
+----------------------------------------+
```

**关键控件ID对照表**：

| ID | 控件类型 | 用途 |
|----|---------|------|
| `longitudeVal` | TextView | 显示经度数值 |
| `latitudeVal` | TextView | 显示纬度数值 |
| `CurrentMode` | TextView | 显示当前状态（震动/非法入侵） |
| `Anti_theft_mode` | Switch | 防盗模式开关 |
| `layout1` | LinearLayout | 状态显示区域（可隐藏/显示） |

---

#### 3.2 dialog_login.xml（登录对话框布局）

**文件路径**：`app/src/main/res/layout/dialog_login.xml`

**文件功能说明**：
这是一个弹出式登录对话框的布局，APP启动时首先显示这个对话框，
要求用户输入MQTT的订阅主题（Sub）和发布主题（Pub）。注意这里的"username"
和"password"实际上是MQTT的Topic名称，不是真正的用户名密码。

```xml
<?xml version="1.0" encoding="utf-8"?>
<!-- LinearLayout: 垂直线性布局 -->
<LinearLayout
    xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent"     <!-- 宽度填满对话框 -->
    android:layout_height="wrap_content"    <!-- 高度自适应内容 -->
    android:orientation="vertical"          <!-- 垂直排列 -->
    android:padding="16dp">                 <!-- 四边内边距16dp -->

    <!-- 
        EditText: 文本输入框（Sub主题）
        hint: 输入提示文字
    -->
    <EditText
        android:id="@+id/et_username"
        android:layout_width="match_parent"
        android:layout_height="wrap_content"
        android:hint="Sub:"/>     <!-- 提示用户输入订阅主题 -->

    <!-- EditText: 文本输入框（Pub主题） -->
    <EditText
        android:id="@+id/et_password"
        android:layout_width="match_parent"
        android:layout_height="wrap_content"
        android:hint="Pub:"/>     <!-- 提示用户输入发布主题 -->

</LinearLayout>
```

**通俗解释**：
- 这个布局非常简单，就是两个上下排列的输入框
- 第一个输入框`et_username`的hint是"Sub:"，提示输入MQTT订阅主题
- 第二个输入框`et_password`的hint是"Pub:"，提示输入MQTT发布主题
- **重要理解**：这里的username/password在代码中被用作MQTT的Topic名称，
  是一种简化设计。Sub Topic用于接收小车数据，Pub Topic用于向小车发送命令。

对话框显示效果：
```
+--------------------------+
|  首次输入 / 确认主题       |  ← 对话框标题
|                          |
|  [Sub: ______________]   |  ← 订阅主题输入框
|  [Pub: ______________]   |  ← 发布主题输入框
|                          |
|     [取消]    [确定]      |  ← 两个按钮
+--------------------------+
```

---

#### 3.3 strings.xml（字符串资源）

**文件路径**：`app/src/main/res/values/strings.xml`

```xml
<resources>
    <string name="app_name">汽车防盗监测</string>
</resources>
```

**通俗解释**：
- `strings.xml` 是APP的"文字库"，把所有界面上的文字集中管理
- `name="app_name"` 给这段文字起了个名字叫"app_name"
- 在`AndroidManifest.xml`中通过`@string/app_name`引用这个名字
- 这样做的好处是：**方便国际化**（做中文版、英文版只需替换这个文件）
- APP在桌面上显示的名称就是"汽车防盗监测"

---

#### 3.4 colors.xml（颜色资源）

**文件路径**：`app/src/main/res/values/colors.xml`

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <color name="black">#FF000000</color>    <!-- 纯黑色 -->
    <color name="white">#FFFFFFFF</color>    <!-- 纯白色 -->
</resources>
```

**通俗解释**：
- `colors.xml` 是APP的"调色盘"，集中管理所有颜色
- 颜色格式：`#AARRGGBB`（Alpha透明度 + Red红 + Green绿 + Blue蓝）
- `#FF000000`：FF表示不透明，00红，00绿，00蓝 = 纯黑色
- `#FFFFFFFF`：FF表示不透明，FF红，FF绿，FF蓝 = 纯白色
- 代码中可以通过 `@color/black` 或 `@color/white` 引用这些颜色
- 这个项目中的颜色主要是直接在布局文件中写了色值（如`#03A9F4`），没有大量使用这里定义的颜色

---

### 四、核心代码分析 —— MainActivity.java

---

#### 4.1 文件整体概述

**文件路径**：`app/src/main/java/cacom/example/smarthome/MainActivity.java`

**整体功能**：
这是APP的**核心代码文件**，也是唯一一个Activity（界面）。
它负责：
1. 显示主界面和处理用户交互
2. 显示登录对话框获取MQTT主题
3. 连接MQTT服务器并维持连接
4. 订阅Topic接收小车传感器数据
5. 解析JSON数据并更新界面显示
6. 控制防盗模式的开关
7. 处理报警（振动提示）

**通信原理**：
APP ↔ MQTT服务器 ↔ STM32单片机（小车上）
- APP通过**订阅（Subscribe）**某个Topic来接收数据
- APP通过**发布（Publish）**到某个Topic来发送命令
- STM32同样连接MQTT服务器，与APP实现双向通信

---

#### 4.2 包声明与导入（package & import）

```java
package cacom.example.smarthome;    // 声明这个类属于哪个包

import androidx.appcompat.app.AppCompatActivity;    // Android兼容Activity基类
import android.annotation.SuppressLint;              // 忽略Lint警告的注解
import android.app.AlertDialog;                       // 弹出对话框
import android.app.Service;                           // 系统服务基类
import android.content.SharedPreferences;             // 轻量级数据存储
import android.os.Bundle;                             // 用于保存/恢复Activity状态
import android.os.Handler;                            // 消息处理器（线程间通信）
import android.os.Looper;                             // 消息循环器
import android.os.Message;                            // 消息对象
import android.os.Vibrator;                           // 震动器
import android.view.LayoutInflater;                   // 布局加载器
import android.view.View;                             // UI视图基类
import android.widget.Button;                         // 按钮控件
import android.widget.CompoundButton;                 // 复合按钮基类（Switch继承自它）
import android.widget.EditText;                       // 文本输入框
import android.widget.ImageView;                      // 图片显示控件
import android.widget.LinearLayout;                   // 线性布局
import android.widget.Switch;                         // 开关控件
import android.widget.TextView;                       // 文本显示控件
import android.widget.Toast;                          // 短暂提示气泡

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;     // MQTT消息投递令牌
import org.eclipse.paho.client.mqttv3.MqttCallback;           // MQTT回调接口
import org.eclipse.paho.client.mqttv3.MqttClient;             // MQTT客户端
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;     // MQTT连接选项
import org.eclipse.paho.client.mqttv3.MqttException;          // MQTT异常
import org.eclipse.paho.client.mqttv3.MqttMessage;            // MQTT消息
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;  // 内存持久化

import org.json.JSONException;    // JSON解析异常
import org.json.JSONObject;       // JSON对象（用于解析传感器数据）

import java.text.DecimalFormat;   // 数字格式化（保留小数位数）
import java.util.concurrent.Executors;                    // 线程池工厂
import java.util.concurrent.ScheduledExecutorService;     // 定时任务服务
import java.util.concurrent.TimeUnit;                     // 时间单位
```

**通俗解释**：
- `package cacom.example.smarthome`：这个Java文件属于`cacom.example.smarthome`包
  - 包名就像文件夹路径，避免不同开发者的类名冲突
  - 包名通常使用公司域名的倒序（如`com.google.xxx`）
  - 这里是`cacom.example.smarthome`
- `import`：就像"引入工具"，告诉编译器我要用哪些类
  - Android相关类：界面、按钮、文本框等UI控件
  - MQTT相关类：来自Eclipse Paho库，用于网络通信
  - JSON相关类：用于解析从服务器收到的数据格式
  - 并发相关类：用于定时重连等后台任务

---

#### 4.3 类声明与成员变量

```java
// 声明MainActivity类，继承自AppCompatActivity
// AppCompatActivity是Android提供的Activity基类，带有兼容性支持
public class MainActivity extends AppCompatActivity {

    // ========== 界面控件变量 ==========
    TextView longitudeVal, latitudeVal, CurrentMode;   // 三个文本显示控件
    Switch Anti_theft_mode;                            // 防盗模式开关
    LinearLayout layout1;                              // 状态显示布局容器

    // ========== MQTT ID生成 ==========
    int math = (int) ((Math.random() * 9 + 1) * (10000));
    // 生成1-9之间的随机数，乘以10000，得到10000-90000之间的随机整数
    // 用于生成唯一的MQTT客户端ID

    String mqttid = "sadjk" + String.valueOf(math);
    // 拼接前缀"sadjk"和随机数，如"sadjk45678"
    // 每个APP实例都有唯一的客户端ID，避免与其他客户端冲突

    // ========== 后台任务与通信对象 ==========
    private ScheduledExecutorService scheduler;    // 定时任务调度器（用于自动重连）
    private MqttClient client;                     // MQTT客户端对象（核心通信对象）
    private Handler handler;                       // 消息处理器（UI线程与后台线程通信）

    // ========== MQTT连接配置 ==========
    private String host = "tcp://47.109.89.8:1883";    // MQTT服务器地址
    private String userName = "root23";                 // MQTT用户名
    private String passWord = "root34";                 // MQTT密码
    private String mqtt_id = mqttid;                    // MQTT客户端ID
    private String mqtt_sub_topic = "";                 // 订阅主题（用户输入）
    private String mqtt_pub_topic = "";                 // 发布主题（用户输入）

    // ========== 其他功能对象 ==========
    private Vibrator myVibrator;                        // 手机震动器
    private SharedPreferences sharedPreferences;        // 本地数据存储（保存用户输入的主题）
    private AlertDialog loginDialog;                    // 登录对话框

    Message msg = new Message();    // 消息对象（用于Handler通信）
```

**变量详解表**：

| 变量名 | 类型 | 用途 |
|--------|------|------|
| `longitudeVal` | TextView | 显示经度数值的文本控件 |
| `latitudeVal` | TextView | 显示纬度数值的文本控件 |
| `CurrentMode` | TextView | 显示当前状态（震动/非法入侵/空） |
| `Anti_theft_mode` | Switch | 防盗模式开关控件 |
| `layout1` | LinearLayout | 状态显示区域容器 |
| `math` | int | 随机数（10000~90000），用于生成唯一MQTT客户端ID |
| `mqttid` | String | MQTT客户端ID，如"sadjk45678" |
| `scheduler` | ScheduledExecutorService | 定时器，每10秒检查一次MQTT连接 |
| `client` | MqttClient | MQTT客户端，负责与服务器的所有通信 |
| `handler` | Handler | 消息处理器，后台线程通过它更新UI |
| `host` | String | MQTT服务器地址：`tcp://47.109.89.8:1883` |
| `userName` | String | MQTT登录用户名：`root23` |
| `passWord` | String | MQTT登录密码：`root34` |
| `mqtt_sub_topic` | String | 订阅主题（从对话框获取） |
| `mqtt_pub_topic` | String | 发布主题（从对话框获取） |
| `myVibrator` | Vibrator | 手机震动器（报警时触发） |
| `sharedPreferences` | SharedPreferences | 本地轻量存储，记住用户输入的主题 |
| `loginDialog` | AlertDialog | 登录对话框对象 |
| `msg` | Message | 用于Handler发送消息的全局对象 |

---

#### 4.4 onCreate() —— Activity创建时的入口函数

```java
@Override
protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);        // 调用父类的onCreate，必须！
    setContentView(R.layout.activity_main);    // 设置界面布局，加载activity_main.xml

    // 获取SharedPreferences对象，名为"UserData"，私有模式
    sharedPreferences = getSharedPreferences("UserData", MODE_PRIVATE);

    // 初始化控件（获取UI控件的引用）
    Control_initialization();

    // 显示登录对话框，用户输入主题后执行lambda中的代码
    showLoginDialog(() -> {
        // 对话框关闭后执行的核心逻辑
        initializeAfterLogin();
    });
}
```

**通俗解释**：
- `onCreate()` 是Activity的"出生函数"，当用户打开APP时，系统会调用这个函数
- `super.onCreate(savedInstanceState)`：先让父类做初始化工作
- `setContentView(R.layout.activity_main)`：把`activity_main.xml`的布局加载到屏幕上
  - `R.layout.activity_main` 是Android自动生成的资源引用
- `getSharedPreferences("UserData", MODE_PRIVATE)`：打开一个叫"UserData"的本地存储文件
  - `MODE_PRIVATE`：只有这个APP能访问这个文件
  - 用于记住用户上次输入的Sub/Pub主题
- `showLoginDialog(...)`：显示登录对话框，传入一个"回调函数"
  - 当用户在对话框中点击"确定"后，会执行括号里的代码
  - `() -> { ... }` 是Java 8的Lambda表达式，简化了匿名内部类的写法

**执行流程**：
```
打开APP
  → onCreate()被调用
    → 加载主界面布局
    → 打开本地存储
    → 初始化控件
    → 显示登录对话框
      → 用户输入Sub/Pub主题，点击确定
        → 保存主题到本地存储
        → 调用initializeAfterLogin()
          → 初始化MQTT连接
          → 开始自动重连定时器
          → 设置事件监听
```

---

#### 4.5 showLoginDialog() —— 显示登录对话框

```java
private void showLoginDialog(Runnable onSuccess) {
    // 1. 加载对话框布局文件（dialog_login.xml），创建视图对象
    View dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_login, null);

    // 2. 从布局中获取两个输入框的引用
    EditText etUsername = dialogView.findViewById(R.id.et_username);
    EditText etPassword = dialogView.findViewById(R.id.et_password);

    // 3. 自动填充已保存的数据（如果有的话）
    String savedUsername = sharedPreferences.getString("username", "");
    String savedPassword = sharedPreferences.getString("password", "");
    etUsername.setText(savedUsername);
    etPassword.setText(savedPassword);

    // 4. 构建对话框
    loginDialog = new AlertDialog.Builder(this)
            .setTitle(savedUsername.isEmpty() ? "首次输入" : "确认主题")  // 动态标题
            .setView(dialogView)                                              // 设置自定义布局
            .setPositiveButton("确定", null)                                   // 确定按钮（先不设置回调）
            .setNegativeButton("取消", (dialog, which) -> finish())           // 取消按钮→退出APP
            .setCancelable(false)                                             // 禁止返回键关闭
            .create();                                                        // 创建对话框对象

    // 5. 自定义确定按钮的逻辑（防止不输入就关闭）
    loginDialog.setOnShowListener(dialogInterface -> {
        Button positiveButton = loginDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        positiveButton.setOnClickListener(v -> {
            // 获取输入的内容并去除首尾空格
            String username = etUsername.getText().toString().trim();
            String password = etPassword.getText().toString().trim();

            // 清除之前的错误提示
            etUsername.setError(null);
            etPassword.setError(null);

            // 输入校验
            boolean hasError = false;
            if (username.isEmpty()) {
                etUsername.setError("不能为空");    // 显示红色错误提示
                hasError = true;
            }
            if (password.isEmpty()) {
                etPassword.setError("不能为空");
                hasError = true;
            }

            // 校验通过后保存数据并继续
            if (!hasError) {
                saveUserData(username, password);
                mqtt_sub_topic = username;      // 将username赋值给订阅主题
                mqtt_pub_topic = password;       // 将password赋值给发布主题
                loginDialog.dismiss();           // 关闭对话框
                onSuccess.run();                 // 执行后续初始化（initializeAfterLogin）
            }
        });
    });

    // 6. 显示对话框
    loginDialog.show();
}
```

**通俗解释**：

这个函数创建一个弹窗，要求用户输入MQTT的Sub主题和Pub主题。

**分步解析**：
1. **加载布局**：`LayoutInflater.from(this).inflate(R.layout.dialog_login, null)` 把`dialog_login.xml`加载成View对象
2. **获取输入框**：通过`findViewById`找到两个EditText控件
3. **自动填充**：从`SharedPreferences`读取上次保存的主题，自动填到输入框里
4. **构建对话框**：
   - `setTitle(...)`：如果是第一次使用显示"首次输入"，否则显示"确认主题"
   - `setView(dialogView)`：把刚才加载的布局放到对话框里
   - `setPositiveButton("确定", null)`：先设null，后面手动处理点击事件
   - `setNegativeButton("取消", ...)`：点击取消就调用`finish()`退出APP
   - `setCancelable(false)`：**重要！** 禁止点击对话框外部或按返回键关闭，强制用户必须输入
5. **自定义确定逻辑**：
   - `setOnShowListener`：当对话框显示时执行的代码
   - 获取"确定"按钮，手动设置点击事件
   - 检查两个输入框是否为空，空的话显示红色错误提示
   - 不空的话保存数据，关闭对话框，执行后续初始化
6. **显示**：`loginDialog.show()` 把对话框显示到屏幕上

---

#### 4.6 saveUserData() & showSavedData() —— 数据保存与显示

```java
// 保存用户数据到本地存储
private void saveUserData(String username, String password) {
    SharedPreferences.Editor editor = sharedPreferences.edit();    // 获取编辑器
    editor.putString("username", username);                         // 存入username
    editor.putString("password", password);                         // 存入password
    editor.apply();                                                  // 异步提交保存
    showSavedData();
}

// 显示已保存的数据（调试用，实际没有显示到界面）
private void showSavedData() {
    String username = sharedPreferences.getString("username", "未保存");
    String password = sharedPreferences.getString("password", "未保存");
    // 这里没有实际使用读取的值，只是读出来了而已
}
```

**通俗解释**：
- `SharedPreferences` 是Android的"轻量级存储"，像一个小型的键值对数据库
  - 适合存少量简单的数据（用户名、配置项等）
  - 数据以XML格式存储在APP的私有目录中
  - 即使APP关闭再打开，数据还在
- `editor.putString("username", username)`：以"username"为键，存入用户输入的值
- `editor.apply()`：异步保存（不阻塞主线程，推荐使用）
- `editor.commit()` 是同步保存（会阻塞，已不推荐使用）

---

#### 4.7 initializeAfterLogin() —— 登录后的核心初始化

```java
private void initializeAfterLogin() {
    // 初始化MQTT客户端
    Mqtt_init();
    // 启动自动重连定时器
    startReconnect();
    // 设置UI事件监听
    Listen_for_events();

    // 创建消息处理器，处理从MQTT后台线程传回的消息
    handler = new Handler(Looper.myLooper()) {
        @SuppressLint("SetTextI18n")
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 1:    // 开机校验更新回传（预留，未使用）
                    break;
                case 2:    // 反馈回传（预留，未使用）
                    break;
                case 3:    // MQTT收到消息回传
                    System.out.println(msg.obj.toString());    // 控制台打印消息内容
                    parseJsonobj(msg.obj.toString());           // 解析JSON并更新UI
                    break;
                case 30:   // 连接失败
                    Toast.makeText(MainActivity.this, 
                            "MQTT服务器连接失败", Toast.LENGTH_SHORT).show();
                    break;
                case 31:   // 连接成功
                    Toast.makeText(MainActivity.this, 
                            "MQTT服务器连接成功,等待硬件数据上报", Toast.LENGTH_SHORT).show();
                    try {
                        client.subscribe(mqtt_sub_topic, 0);    // 订阅主题
                    } catch (MqttException e) {
                        e.printStackTrace();
                    }
                    break;
                default:
                    break;
            }
        }
    };
}
```

**通俗解释**：

这个函数是APP的核心初始化逻辑，在用户输入主题后被调用。

**分步解析**：
1. `Mqtt_init()`：创建MQTT客户端对象，设置连接参数和回调函数
2. `startReconnect()`：启动定时器，每10秒检查一次连接状态，断开就重连
3. `Listen_for_events()`：给防盗模式开关设置监听器
4. `handler = new Handler(...)`：创建消息处理器

**Handler是什么？为什么要用Handler？**

Android有一个"主线程"（也叫UI线程），负责处理用户交互和更新界面。
网络通信等耗时操作必须在"后台线程"中执行，不能阻塞主线程。
但Android规定：**只有主线程才能更新UI**。

Handler就是"线程间通信的信使"：
- 后台线程收到MQTT消息后，通过Handler发送消息
- 主线程的Handler收到消息后，在主线程中更新界面

```
MQTT后台线程              Handler               主线程/UI线程
     |                      |                        |
  收到消息  ──── msg.what=3 ───→  handleMessage()    |
     |                      |      → parseJsonobj()  |
     |                      |      → setText()更新UI ←┘
```

**msg.what 取值含义表**：

| msg.what | 含义 | 处理逻辑 |
|----------|------|----------|
| 1 | 开机校验（预留） | 无操作 |
| 2 | 反馈回传（预留） | 无操作 |
| 3 | 收到MQTT消息 | 解析JSON，更新GPS和状态显示 |
| 30 | 连接失败 | 弹出Toast提示"MQTT服务器连接失败" |
| 31 | 连接成功 | 弹出Toast提示成功，订阅指定Topic |

---

#### 4.8 Mqtt_init() —— MQTT客户端初始化

```java
private void Mqtt_init() {
    try {
        // 创建MQTT客户端对象
        // 参数1: host - 服务器地址 tcp://47.109.89.8:1883
        // 参数2: mqtt_id - 客户端唯一标识，如"sadjk45678"
        // 参数3: MemoryPersistence - 使用内存保存会话状态
        client = new MqttClient(host, mqtt_id, new MemoryPersistence());

        // 设置MQTT连接选项
        MqttConnectOptions options = new MqttConnectOptions();

        // false表示服务器保留客户端的连接记录（断线重连后能恢复会话）
        options.setCleanSession(false);

        // 设置MQTT用户名和密码（用于服务器认证）
        options.setUserName(userName);                    // "root23"
        options.setPassword(passWord.toCharArray());      // "root34"转字符数组

        // 设置连接超时时间为10秒
        options.setConnectionTimeout(10);

        // 设置心跳间隔为20秒
        // 服务器会每隔 1.5 * 20 = 30 秒发送一次心跳包检测客户端是否在线
        options.setKeepAliveInterval(20);

        // 设置回调函数（MQTT事件发生时自动调用）
        client.setCallback(new MqttCallback() {
            @Override
            public void connectionLost(Throwable cause) {
                // 连接丢失时调用（网络断开、服务器故障等）
                System.out.println("connectionLost----------");
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                // 消息发送成功后调用
                System.out.println("deliveryComplete---------" + token.isComplete());
            }

            @Override
            public void messageArrived(String topicName, MqttMessage message) throws Exception {
                // 收到订阅主题的消息时调用（核心！）
                System.out.println("messageArrived----------");
                Message msg = new Message();
                msg.what = 3;                           // 设置消息类型：收到MQTT消息
                msg.obj = message.toString();            // 消息内容存入obj
                handler.sendMessage(msg);                // 通过Handler发送给主线程
            }
        });
    } catch (Exception e) {
        e.printStackTrace();    // 打印异常信息到控制台
    }
}
```

**通俗解释**：

这个函数就像"配置一台对讲机"，设置好频率、呼号等参数。

**MQTT是什么？**
MQTT（Message Queuing Telemetry Transport）是一种轻量级的消息传输协议，
特别适合物联网设备通信。它有3个核心概念：
- **发布者（Publisher）**：发送消息的一方
- **订阅者（Subscriber）**：接收消息的一方
- **代理（Broker）**：中间服务器，负责转发消息

```
STM32(小车) --publish("car/data")--> MQTT服务器 --推送给--> APP(订阅了"car/data")
APP         --publish("car/cmd")-->  MQTT服务器 --推送给--> STM32(订阅了"car/cmd")
```

**关键参数说明**：

| 参数 | 值 | 说明 |
|------|-----|------|
| `host` | `tcp://47.109.89.8:1883` | MQTT服务器IP和端口（1883是MQTT默认端口） |
| `CleanSession` | `false` | 保持会话，断线重连后能收到离线期间的消息 |
| `ConnectionTimeout` | 10秒 | 连接服务器最多等待10秒 |
| `KeepAliveInterval` | 20秒 | 每20秒发一次心跳，告诉服务器"我还活着" |
| `userName` | `root23` | MQTT认证用户名 |
| `passWord` | `root34` | MQTT认证密码 |

**三个回调函数**：

| 回调函数 | 触发时机 | 作用 |
|----------|---------|------|
| `connectionLost()` | 网络断开时 | 打印日志，可在这里触发重连 |
| `deliveryComplete()` | 消息发送成功时 | 确认消息已送达 |
| `messageArrived()` | 收到新消息时 | **核心！** 将消息转发给Handler处理 |

**注意**：`messageArrived()` 是在后台线程中被调用的，所以不能直接更新UI，
必须通过Handler把消息发送到主线程。

---

#### 4.9 Mqtt_connect() —— MQTT连接函数

```java
// MQTT连接函数
private void Mqtt_connect() {
    // 创建新线程执行连接操作（网络操作不能阻塞主线程）
    new Thread(new Runnable() {
        @Override
        public void run() {
            try {
                // 检查客户端是否已连接
                if (!(client.isConnected())) {
                    MqttConnectOptions options = null;    // 这里有个BUG，应该是上面的options
                    client.connect(options);               // 连接服务器（空选项）
                    Message msg = new Message();
                    msg.what = 31;                         // 发送"连接成功"消息
                    handler.sendMessage(msg);
                }
            } catch (Exception e) {
                e.printStackTrace();
                handler.sendMessage(msg);    // 这里有个BUG，msg可能还是上次的
            }
        }
    }).start();    // 启动线程
}
```

**通俗解释**：
- 这个函数在后台线程中执行MQTT连接操作
- `new Thread(...).start()` 创建并启动一个新线程
- `client.isConnected()` 检查是否已连接
- `client.connect(options)` 执行连接（这里options传了null，实际使用的是Mqtt_init中设置的回调）
- 连接成功后发送`msg.what = 31`的消息，触发Handler中的"连接成功"处理逻辑

**⚠️ 代码BUG说明**：
1. `MqttConnectOptions options = null;` —— 这里创建了一个新的null选项，
   而Mqtt_init中已经创建了配置好的options但没有保存为成员变量。
   不过由于Paho库的设计，`client.connect(null)`也能使用默认配置。
2. `handler.sendMessage(msg)` 在catch块中使用了全局的`msg`对象，
   这个msg的what值可能不正确（初始创建的Message，what默认是0）。

---

#### 4.10 startReconnect() —— 自动重连定时器

```java
// MQTT重新连接函数
private void startReconnect() {
    // 创建单线程定时任务调度器
    scheduler = Executors.newSingleThreadScheduledExecutor();

    // 以固定频率执行任务
    scheduler.scheduleAtFixedRate(new Runnable() {
        @Override
        public void run() {
            // 如果客户端未连接，就尝试连接
            if (!client.isConnected()) {
                Mqtt_connect();
            }
        }
    }, 0 * 1000, 10 * 1000, TimeUnit.MILLISECONDS);
    // 参数1: 首次执行的Runnable任务
    // 参数2: 首次延迟时间 = 0秒（立即执行第一次）
    // 参数3: 执行间隔 = 10秒（每10秒检查一次）
    // 参数4: 时间单位 = 毫秒
}
```

**通俗解释**：
- 这个函数创建一个"定时闹钟"，每10秒检查一次MQTT连接状态
- `Executors.newSingleThreadScheduledExecutor()`：创建一个单线程的定时任务执行器
- `scheduleAtFixedRate(...)`：按固定频率重复执行任务
- 如果检测到未连接，就调用`Mqtt_connect()`尝试重新连接
- 这个机制确保了APP在网络不稳定时能够自动恢复连接

**定时器工作流程**：
```
时间轴: 0s    10s    20s    30s    40s    50s ...
         |      |      |      |      |      |
      检查→   检查→  检查→  检查→  检查→  检查...
      未连接? 未连接? 未连接? ...
        ↓      ↓
      连接   已连接则跳过
```

---

#### 4.11 publishmessageplus() —— 发布消息函数（下发命令）

```java
// 订阅函数（实际功能是发布消息/下发命令）
private void publishmessageplus(String topic, String message2) {
    // 安全检查：如果客户端为空或未连接，直接返回
    if (client == null || !client.isConnected()) {
        return;
    }

    // 创建MQTT消息对象
    MqttMessage message = new MqttMessage();
    message.setPayload(message2.getBytes());    // 将字符串转为字节数组

    try {
        // 发布消息
        // 参数1: topic - 要发布的主题
        // 参数2: message2.getBytes() - 消息内容（字节数组）
        // 参数3: 0 - QoS等级（0=最多一次，不保证送达）
        // 参数4: false - 不保留消息
        client.publish(topic, message2.getBytes(), 0, false);
    } catch (MqttException e) {
        e.printStackTrace();
    }
}
```

**通俗解释**：
- 这个函数用于**向小车发送命令**（如开启防盗模式）
- `client == null || !client.isConnected()`：安全检查，未连接时不发送
- `message2.getBytes()`：将字符串转为字节数组（网络传输需要字节形式）
- `client.publish(...)`：发布消息到指定主题

**QoS等级说明**：

| QoS值 | 名称 | 说明 |
|-------|------|------|
| 0 | 最多一次 | 发出去就不管了，不保证送达，速度快 |
| 1 | 至少一次 | 保证送达，但可能重复 |
| 2 | 恰好一次 | 保证送达且不重复，但速度慢 |

这里用的是QoS 0，因为防盗命令丢了可以下次再发，追求速度。

**调用场景**：
- 开启防盗模式时：`publishmessageplus(mqtt_pub_topic, "Anti_theft_mode")`
- 关闭防盗模式时：`publishmessageplus(mqtt_pub_topic, "Display")`

---

#### 4.12 parseJsonobj() —— JSON数据解析函数

```java
private void parseJsonobj(String jsonobj) {
    try {
        // 将字符串解析为JSON对象
        JSONObject jsonObject = new JSONObject(jsonobj);

        // 从JSON中提取字段
        double sensor1 = jsonObject.getDouble("sensor1");    // 经度
        double sensor2 = jsonObject.getDouble("sensor2");    // 纬度
        String sensor3 = jsonObject.getString("sensor3");    // 状态码

        // 使用DecimalFormat保留6位小数
        DecimalFormat df = new DecimalFormat("0.000000");
        String stringsensor1 = df.format(sensor1);    // 如：113.123456
        String stringsensor2 = df.format(sensor2);    // 如：23.123456

        // 更新界面显示
        longitudeVal.setText(stringsensor1);    // 显示经度
        latitudeVal.setText(stringsensor2);     // 显示纬度

        // 根据sensor3的值判断状态
        if (Integer.parseInt(sensor3) == 2) {
            CurrentMode.setText("发生震动");        // 显示振动报警
        } else if (Integer.parseInt(sensor3) == 3) {
            CurrentMode.setText("非法入侵");        // 显示入侵报警
            myVibrator.cancel();                   // 先停止之前的震动
            myVibrator.vibrate(new long[]{100, 100, 100, 1000}, 0);
            // 震动模式：震100ms，停100ms，震100ms，停1000ms，循环
            // 参数2: 0 = 从第0个元素开始循环
        } else {
            CurrentMode.setText("    ");            // 清空状态显示
            myVibrator.cancel();                   // 停止震动
        }
    } catch (JSONException e) {
        e.printStackTrace();    // JSON解析出错时打印错误信息
    }
}
```

**通俗解释**：

这个函数负责**解析从MQTT服务器收到的小车数据**，并更新界面。

**JSON数据格式示例**：
```json
{
    "sensor1": 113.546875,     // 经度坐标
    "sensor2": 23.128472,      // 纬度坐标
    "sensor3": "3"             // 状态码：1=正常，2=震动，3=非法入侵
}
```

**解析流程**：
1. `new JSONObject(jsonobj)`：将收到的字符串转为JSON对象
2. `getDouble("sensor1")`：提取sensor1字段的值（double类型）
3. `getString("sensor3")`：提取sensor3字段的值（字符串类型）
4. `DecimalFormat("0.000000")`：将坐标格式化为6位小数
5. `setText(...)`：更新界面的TextView显示

**状态码含义表**：

| sensor3值 | 含义 | 界面显示 | 手机震动 |
|-----------|------|----------|----------|
| 1 或其他 | 正常 | 空白（"    "） | 不震动 |
| 2 | 发生震动 | "发生震动" | 不震动 |
| 3 | 非法入侵 | "非法入侵" | 震动（100ms震/100ms停/100ms震/1000ms停，循环） |

**震动模式说明**：
```java
myVibrator.vibrate(new long[]{100, 100, 100, 1000}, 0);
// 参数1: long[] 数组 - 奇数索引是震动时长，偶数索引是停顿时长
//  [100, 100, 100, 1000]
//   ↑震   ↑停  ↑震   ↑停
// 参数2: repeat = 0 - 从第0个元素开始循环执行
```

---

#### 4.13 Control_initialization() —— 控件初始化

```java
// 给控件获取id（将代码变量与布局文件中的控件关联起来）
private void Control_initialization() {
    // 获取系统震动服务
    myVibrator = (Vibrator) getSystemService(Service.VIBRATOR_SERVICE);

    // 通过findViewById将布局中的控件与Java变量绑定
    longitudeVal = findViewById(R.id.longitudeVal);      // 经度TextView
    latitudeVal = findViewById(R.id.latitudeVal);         // 纬度TextView
    CurrentMode = findViewById(R.id.CurrentMode);         // 状态TextView

    layout1 = findViewById(R.id.layout1);                 // 状态显示容器
    layout1.setVisibility(View.GONE);                      // 初始状态设为隐藏

    Anti_theft_mode = findViewById(R.id.Anti_theft_mode); // 防盗模式Switch
}
```

**通俗解释**：
- `findViewById(R.id.xxx)`：这是Android的"找对象"方法
  - 布局文件（XML）中定义了各种控件，每个控件有唯一的id
  - `findViewById` 根据id找到对应的控件对象，赋值给Java变量
  - 之后就可以通过Java变量来操作控件了

- `getSystemService(Service.VIBRATOR_SERVICE)`：向系统申请使用震动服务
  - Android系统管理着各种硬件服务（震动、定位、传感器等）
  - APP需要通过`getSystemService`来申请使用权限

- `layout1.setVisibility(View.GONE)`：初始时隐藏状态显示区域
  - `View.VISIBLE` = 可见
  - `View.INVISIBLE` = 不可见但占空间
  - `View.GONE` = 不可见且不占空间

---

#### 4.14 Listen_for_events() —— 事件监听设置

```java
// 监听事件（设置用户交互的响应逻辑）
private void Listen_for_events() {
    // 给防盗模式开关设置状态变化监听器
    Anti_theft_mode.setOnCheckedChangeListener(
        new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            if (isChecked) {
                // 开关被打开：开启防盗模式
                layout1.setVisibility(View.VISIBLE);                        // 显示状态区域
                publishmessageplus(mqtt_pub_topic, "Anti_theft_mode");     // 发送开启命令
            } else {
                // 开关被关闭：关闭防盗模式
                layout1.setVisibility(View.GONE);                            // 隐藏状态区域
                publishmessageplus(mqtt_pub_topic, "Display");             // 发送关闭命令
            }
        }
    });
}
```

**通俗解释**：
- `setOnCheckedChangeListener`：给Switch开关设置"监听器"
  - 监听器就像一个"门卫"，当开关状态变化时自动执行里面的代码
  - `CompoundButton` 是Switch的父类，`isChecked` 表示当前是否被勾选

**两种状态的处理**：

| 操作 | 界面变化 | 发送的命令 | 小车行为 |
|------|----------|-----------|----------|
| 打开开关 | 显示layout1区域 | `"Anti_theft_mode"` | 开启防盗监测（传感器开始工作） |
| 关闭开关 | 隐藏layout1区域 | `"Display"` | 关闭防盗监测 |

---

### 五、APP与STM32小车的通信方式详解

---

#### 5.1 通信架构

```
+---------------+            +---------------------+            +---------------+
|  Android APP  |            |   MQTT Broker       |            |  STM32 单片机  |
|               |  ←WiFi/4G→ |  47.109.89.8:1883   |  ←WiFi/4G→ |               |
|               |   TCP连接   |  (阿里云/腾讯云服务器) |   TCP连接   |  ESP8266/WiFi |
|               |            |                     |            |  模块          |
+---------------+            +---------------------+            +---------------+
      |                                 |                                |
      | subscribe("topicA")            |                                | publish("topicA")
      | ─────────────────────────────→ |                                | ────────────────→
      |                                |                                |
      | publish("topicB", "cmd")       |                                | subscribe("topicB")
      | ─────────────────────────────→ |                                | ←────────────────
```

#### 5.2 数据流向

**上行：小车 → APP（传感器数据上报）**
```
STM32采集传感器数据
    ↓
封装成JSON格式: {"sensor1":113.54, "sensor2":23.12, "sensor3":"3"}
    ↓
ESP8266 WiFi模块通过MQTT发布到Sub Topic
    ↓
MQTT服务器收到消息，转发给所有订阅了该Topic的客户端
    ↓
APP的messageArrived()被调用
    ↓
Handler发送msg.what=3的消息
    ↓
主线程的handleMessage()调用parseJsonobj()
    ↓
更新UI显示GPS坐标和状态
```

**下行：APP → 小车（控制命令下发）**
```
用户在APP上操作（如打开防盗开关）
    ↓
调用publishmessageplus(mqtt_pub_topic, "Anti_theft_mode")
    ↓
MQTT客户端发布消息到Pub Topic
    ↓
MQTT服务器转发给订阅了该Topic的STM32
    ↓
STM32收到"Anti_theft_mode"字符串
    ↓
执行对应操作（开启传感器监测）
```

#### 5.3 MQTT Topic设计

在这个APP中，Topic名称是通过登录对话框由用户自定义的：
- **Sub Topic**（用户在"Sub:"输入框中填写的值）：用于接收小车数据
- **Pub Topic**（用户在"Pub:"输入框中填写的值）：用于发送命令到小车

这种设计的好处是：不同用户可以使用不同的Topic，实现设备隔离。
例如：
- 用户A的Sub Topic = `carA/data`, Pub Topic = `carA/cmd`
- 用户B的Sub Topic = `carB/data`, Pub Topic = `carB/cmd`

#### 5.4 JSON数据格式

**小车上报的数据格式**：
```json
{
    "sensor1": 113.546875,    // GPS经度（double类型，6位小数精度）
    "sensor2": 23.128472,     // GPS纬度（double类型，6位小数精度）
    "sensor3": "3"            // 传感器状态码（字符串类型）
}
```

**APP下发的命令格式**：
| 命令字符串 | 含义 |
|-----------|------|
| `"Anti_theft_mode"` | 开启防盗模式 |
| `"Display"` | 关闭防盗模式 |

---

### 六、APP完整工作流程图

```
┌─────────────────────────────────────────────────────────────┐
│                      用户打开APP                              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  onCreate() 被调用                                          │
│  - 加载 activity_main.xml 布局                               │
│  - 读取本地保存的用户数据                                     │
│  - 调用 Control_initialization() 初始化控件                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  showLoginDialog() 显示登录对话框                             │
│  ┌─────────────────────────┐                                │
│  │ 标题: 首次输入/确认主题    │                                │
│  │ Sub: [___________]      │ ← 输入MQTT订阅主题               │
│  │ Pub: [___________]      │ ← 输入MQTT发布主题               │
│  │ [取消]  [确定]          │                                │
│  └─────────────────────────┘                                │
│  - 检查输入是否为空                                          │
│  - 保存到 SharedPreferences                                  │
└─────────────────────────────────────────────────────────────┘
                              │
                    用户点击[确定]
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│            initializeAfterLogin() 登录后初始化                 │
│  1. Mqtt_init()     - 创建MQTT客户端，设置回调                │
│  2. startReconnect() - 启动自动重连定时器（每10秒）           │
│  3. Listen_for_events() - 设置开关监听器                     │
│  4. 创建Handler - 处理后台线程消息                            │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                MQTT 连接与数据交互                             │
│                                                             │
│  [连接成功] → Toast: "MQTT服务器连接成功"                     │
│           → client.subscribe(mqtt_sub_topic, 0)             │
│           → 等待小车数据上报...                               │
│                                                             │
│  [收到数据] → messageArrived() 被调用                        │
│           → Handler发送 msg.what=3                           │
│           → parseJsonobj() 解析JSON                          │
│           → 更新经度、纬度、状态显示                           │
│                                                             │
│  [连接断开] → connectionLost() 被调用                        │
│           → 定时器每10秒检测并自动重连                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  用户操作防盗开关                              │
│                                                             │
│  [打开开关] → layout1.setVisibility(VISIBLE)                 │
│           → publishmessageplus(pub_topic, "Anti_theft_mode") │
│           → 小车开启传感器监测                                │
│                                                             │
│  [关闭开关] → layout1.setVisibility(GONE)                    │
│           → publishmessageplus(pub_topic, "Display")         │
│           → 小车关闭传感器监测                                │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    状态报警处理                                │
│                                                             │
│  sensor3 = "2" → CurrentMode.setText("发生震动")             │
│                                                             │
│  sensor3 = "3" → CurrentMode.setText("非法入侵")              │
│              → 手机震动（100ms震,100ms停,100ms震,1000ms停）   │
│                                                             │
│  sensor3 = 其他 → CurrentMode.setText("    ")                │
│              → 停止震动                                      │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、代码问题与改进建议

#### 7.1 已知BUG

1. **Mqtt_connect()中options为null**
   - 代码：`MqttConnectOptions options = null; client.connect(options);`
   - 影响：实际使用了默认连接选项，用户名密码认证可能不生效
   - 修复：将options提升为成员变量，或在connect时创建正确的options

2. **全局msg对象在异常处理中使用不当**
   - 代码：`handler.sendMessage(msg);`
   - 影响：catch块中的msg是全局对象，what值可能不是预期的30（连接失败）
   - 修复：在catch块中创建新的Message对象，设置正确的what值

3. **用户名密码概念混淆**
   - 对话框中的"username"和"password"实际上是MQTT的Topic名称
   - 容易造成误解，建议改为"subTopic"和"pubTopic"

#### 7.2 改进建议

1. **安全性**：MQTT密码硬编码在代码中（`root23`/`root34`），应改为用户输入或加密存储
2. **服务器地址**：IP地址硬编码，应支持用户自定义或域名配置
3. **异常处理**：网络异常时用户体验可以更好（如显示重连倒计时）
4. **日志系统**：使用Android的Log类代替System.out.println
5. **GPS信息显示**：可以增加地图显示功能（集成高德/百度地图SDK）
6. **历史记录**：保存传感器数据历史，支持查看趋势图
7. **通知功能**：APP在后台时通过系统通知推送报警

---

### 八、文件清单与项目结构

```
car2/                                    # 项目根目录
├── settings.gradle                      # Gradle项目设置
├── gradle.properties                    # Gradle属性配置
├── build.gradle                         # 项目级构建配置
├── gradlew / gradlew.bat               # Gradle wrapper脚本
├── gradle/                              # Gradle wrapper配置
└── app/                                 # APP模块
    ├── build.gradle                     # APP构建配置
    ├── proguard-rules.pro               # 代码混淆规则
    └── src/main/                        # 主源代码目录
        ├── AndroidManifest.xml          # 应用清单
        ├── java/cacom/example/smarthome/ # Java源码目录
        │   └── MainActivity.java         # 主Activity代码（核心）
        └── res/                         # 资源文件目录
            ├── layout/                  # 布局文件
            │   ├── activity_main.xml    # 主界面布局
            │   └── dialog_login.xml     # 登录对话框布局
            ├── values/                  # 值资源
            │   ├── strings.xml          # 字符串
            │   └── colors.xml           # 颜色
            ├── mipmap-xxx/              # 图标图片
            └── drawable/                # 图形资源（圆角、渐变等）
```

---

### 九、总结

这个汽车防盗监测APP虽然代码量不大（只有1个Activity，约378行），
但完整地实现了一个**物联网应用的核心功能**：

1. **MQTT通信**：通过Eclipse Paho库实现了与云端MQTT服务器的连接、订阅、发布
2. **JSON数据解析**：接收并解析小车上传的传感器数据
3. **UI交互**：使用Switch开关控制防盗模式，TextView显示GPS坐标和状态
4. **本地存储**：使用SharedPreferences记住用户输入的Topic
5. **报警反馈**：通过震动和文字提示给用户报警

**学习价值**：
- 理解Android Activity生命周期（onCreate）
- 理解Handler线程间通信机制
- 理解MQTT物联网通信协议
- 理解JSON数据格式和解析
- 理解Android布局（LinearLayout、ConstraintLayout）
- 理解SharedPreferences本地存储

这是一个非常适合物联网/Android入门学习的项目。

---

*分析完成时间：2024年*
*分析者：AI代码分析助手*



---

## 第六章：配置文件与中断处理详解

> 本章讲解系统的底层配置和中断处理。你将学习到：STM32如何启用外设、中断是什么、NVIC如何管理中断优先级、SysTick如何提供系统心跳、配置文件如何决定编译哪些模块。这些是理解整个系统的基础。

---

### 第一部分：核心概念扫盲（初学者必读）

在分析代码之前，先理解几个关键概念：

#### 1. 什么是中断（Interrupt）？
**通俗解释**：想象你正在写作业，突然门铃响了，你去开门处理完事情后再回来继续写作业。门铃就是一个"中断"——它打断了你当前的工作，让你去处理更紧急的事情，处理完后再回到原来的工作。

在单片机中，**中断**就是当某个事件发生时（如定时器到期、按键按下、收到数据等），单片机暂停当前执行的程序，转而去执行对应的事件处理程序（中断服务函数），处理完后再回到原来的程序继续执行。

#### 2. 什么是 NVIC？
**NVIC** = Nested Vectored Interrupt Controller（嵌套向量中断控制器）。

**通俗解释**：NVIC 是 STM32 内部的"中断大管家"。它负责：
- 管理所有中断的开关（使能/禁用）
- 设置中断的优先级（哪个中断更紧急）
- 当多个中断同时发生时，决定先处理哪个

#### 3. 什么是 SysTick？
**通俗解释**：SysTick 是 STM32 内置的一个"系统心跳定时器"。它就像心脏一样，以固定频率跳动（通常每1毫秒跳动一次），用于提供系统延时、任务调度等基础时间服务。每次跳动就会产生一次 SysTick 中断。

#### 4. 什么是外设（Peripheral）？
**通俗解释**：外设就是单片机内部集成的各种功能模块。STM32 就像一个小型电脑主机，除了 CPU 之外，还集成了很多功能模块，如：
- **GPIO**：控制引脚的高低电平（点灯、读取按键）
- **USART**：串口通信（连接WiFi模块、蓝牙模块）
- **TIM**：定时器（定时、PWM输出）
- **ADC**：模数转换（读取传感器模拟信号）
- **SPI/I2C**：通信接口（连接OLED显示屏等）

#### 5. 什么是标准外设库（SPL）？
ST 公司为 STM32 提供了一套官方函数库，把寄存器的复杂操作封装成了简单的函数调用。`stm32f10x_xxx.h` 就是这些外设的头文件，包含了对应外设的所有函数声明和宏定义。

---

### 第二部分：代码文件分析

---

#### 文件 1：stm32f10x_conf.h（库配置文件）

**文件路径**：`1-程序源文件/Stm32Code/User/stm32f10x_conf.h`

##### 2.1 文件整体功能说明

这个文件是 **STM32 标准外设库的配置文件**，它的作用是**选择哪些外设模块被编译进项目**。通过包含（`#include`）不同的外设头文件，告诉编译器："我这个项目要用到哪些外设功能"。

**生活比喻**：这就像你去自助餐厅，这个文件就是你拿的"盘子"——你把想吃的食物（需要的外设）都放到盘子里，厨师（编译器）就知道要给你准备哪些菜。

##### 2.2 逐行代码分析与注释

```c
/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_conf.h 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Library configuration file.
  ******************************************************************************
  */
```
**说明**：这是文件头注释，说明了这是 ST 官方的标准外设库模板配置文件，版本 V3.5.0。

```c
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H
```
**说明**：这是**头文件卫士**（Header Guard），防止同一个头文件被重复包含。`#ifndef` 检查是否已经定义了 `__STM32F10x_CONF_H`，如果没有才继续编译。这是 C 语言的常用技巧，避免重复定义错误。

```c
/* Includes ------------------------------------------------------------------*/
/* Uncomment/Comment the line below to enable/disable peripheral header file inclusion */
#include "stm32f10x_adc.h"      // 【ADC 模数转换器】用于将模拟信号（如传感器电压）转换为数字值
#include "stm32f10x_bkp.h"      // 【备份寄存器】用于在断电时保存少量关键数据
#include "stm32f10x_can.h"      // 【CAN 总线】用于汽车等场景的串行通信
#include "stm32f10x_cec.h"      // 【HDMI CEC 接口】消费电子产品控制接口
#include "stm32f10x_crc.h"      // 【CRC 校验单元】用于数据完整性校验
#include "stm32f10x_dac.h"      // 【DAC 数模转换器】将数字信号转换为模拟电压输出
#include "stm32f10x_dbgmcu.h"   // 【调试支持】用于调试时获取MCU状态信息
#include "stm32f10x_dma.h"      // 【DMA 直接内存访问】不经过CPU直接传输数据，提高效率
#include "stm32f10x_exti.h"     // 【外部中断】处理外部引脚触发的中断（如按键、传感器信号）
#include "stm32f10x_flash.h"    // 【Flash 存储器】用于读写单片机内部的Flash（保存程序或数据）
#include "stm32f10x_fsmc.h"     // 【FSMC 存储器控制器】用于外接存储器芯片（如SRAM、LCD）
#include "stm32f10x_gpio.h"     // 【GPIO 通用输入输出】控制引脚的输入输出状态（点灯、读取按键等）
#include "stm32f10x_i2c.h"      // 【I2C 通信接口】用于连接OLED、传感器等外设（两根线通信）
#include "stm32f10x_iwdg.h"     // 【独立看门狗】用于检测程序是否死机，死机时自动复位
#include "stm32f10x_pwr.h"      // 【电源控制】管理低功耗模式、电源监控
#include "stm32f10x_rcc.h"      // 【RCC 时钟控制器】配置所有外设的时钟源和频率（重要！）
#include "stm32f10x_rtc.h"      // 【实时时钟】提供日期和时间功能，掉电可继续运行
#include "stm32f10x_sdio.h"     // 【SDIO 接口】用于读写 SD 卡
#include "stm32f10x_spi.h"      // 【SPI 通信接口】用于连接各种传感器、显示屏等（全双工高速通信）
#include "stm32f10x_tim.h"      // 【定时器】用于定时、PWM输出、输入捕获等
#include "stm32f10x_usart.h"    // 【USART 串口】用于与WiFi模块、蓝牙模块、GPS模块通信
#include "stm32f10x_wwdg.h"     // 【窗口看门狗】另一种看门狗，在特定时间窗口内喂狗才不会复位
#include "misc.h"               // 【NVIC和SysTick高级函数】中断控制器和系统滴答定时器的辅助函数
```

##### 2.3 本项目使用的外设分析

从 `汽车防盗2.txt` 的方案描述来看，本项目实际使用到的外设包括：

| 外设 | 用途 | 对应模块 |
|------|------|----------|
| GPIO | 控制LED指示灯、读取按键、蜂鸣器控制 | `stm32f10x_gpio.h` |
| USART | 与ESP-01S WiFi模块通信、GPS模块通信 | `stm32f10x_usart.h` |
| TIM | 定时器功能（延时、PWM蜂鸣器） | `stm32f10x_tim.h` |
| EXTI | 震动传感器外部中断检测 | `stm32f10x_exti.h` |
| I2C/SPI | OLED液晶显示通信 | `stm32f10x_i2c.h` / `stm32f10x_spi.h` |
| RCC | 时钟配置（所有外设都需要） | `stm32f10x_rcc.h` |
| ADC | 可能用于传感器模拟量读取 | `stm32f10x_adc.h` |

**注意**：这个项目采用了"全包含"的策略，把所有外设头文件都包含进来了。这在开发阶段很常见——先全部引入，确保编译不会出错。在实际产品中，通常会只包含实际需要的外设，以减小代码体积。

##### 2.4 assert_param 调试宏分析

```c
/* Uncomment the line below to expanse the "assert_param" macro in the 
   Standard Peripheral Library drivers code */
/* #define USE_FULL_ASSERT    1 */
```
**说明**：这是一个调试开关。如果取消注释（去掉前面的 `//`），就会启用**参数断言检查**。在开发阶段开启这个功能，可以在传入错误参数时及时发现；在正式发布时关闭，可以节省代码空间和运行时间。

```c
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif
```

**逐行解释**：

| 代码 | 解释 |
|------|------|
| `#ifdef USE_FULL_ASSERT` | 如果定义了 USE_FULL_ASSERT 宏，就使用下面的调试版本 |
| `assert_param(expr)` | 这是一个宏，用于检查参数是否合法 |
| `(expr) ? (void)0 : assert_failed(...)` | 三元运算符：如果表达式为真，什么都不做；如果为假，调用 `assert_failed` |
| `__FILE__` | C语言预定义宏，表示当前文件名（字符串） |
| `__LINE__` | C语言预定义宏，表示当前行号（整数） |
| `assert_failed(file, line)` | 断言失败时调用的函数，会打印出错的文件名和行号 |
| `#else` | 如果没定义 USE_FULL_ASSERT |
| `#define assert_param(expr) ((void)0)` | 断言宏不做任何事情，直接忽略参数检查 |

**通俗解释**：`assert_param` 就像一个"安检门"。开启 `USE_FULL_ASSERT` 时，每次调用库函数都会检查参数是否合法，不合法就报警并告诉你哪里错了；关闭时，安检门敞开，不检查直接通过，运行更快但不安全。

---

#### 文件 2：stm32f10x_it.h（中断处理头文件）

**文件路径**：`1-程序源文件/Stm32Code/User/stm32f10x_it.h`

##### 3.1 文件整体功能说明

这个文件是**中断处理函数的声明头文件**。它声明了 STM32 所有系统异常（Cortex-M3 内核级中断）的处理函数原型。其他 C 文件如果需要使用这些中断函数，就包含这个头文件。

##### 3.2 逐行代码分析与注释

```c
/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.h 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   This file contains the headers of the interrupt handlers.
  ******************************************************************************
  */
```
**说明**：文件头注释，说明这是中断处理函数的声明文件。

```c
#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H
```
**说明**：头文件卫士，防止重复包含。

```c
#ifdef __cplusplus
 extern "C" {
#endif 
```
**说明**：如果是 C++ 编译器，用 `extern "C"` 包裹函数声明。这告诉 C++ 编译器：这些函数是用 C 语言编写的，调用时不要使用 C++ 的名称修饰（name mangling）。这样 C++ 代码才能正确调用这些 C 函数。

```c
#include "stm32f10x.h"
```
**说明**：包含 STM32 的主头文件，这个文件定义了所有寄存器地址、位掩码等基础内容。

```c
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);         // 【非屏蔽中断】最高优先级中断，无法被关闭，通常用于紧急故障
void HardFault_Handler(void);   // 【硬件故障】当程序访问非法内存或执行非法指令时触发
void MemManage_Handler(void);   // 【内存管理故障】MPU（内存保护单元）访问违规时触发
void BusFault_Handler(void);    // 【总线故障】当CPU访问总线上的设备失败时触发
void UsageFault_Handler(void);  // 【用法故障】执行未定义指令、非对齐访问、除零等错误时触发
void SVC_Handler(void);         // 【SVCall 中断】通过 SVC 指令触发，常用于操作系统系统调用
void DebugMon_Handler(void);    // 【调试监控器】用于调试时暂停程序执行
void PendSV_Handler(void);      // 【可挂起的系统调用】常用于操作系统任务切换
void SysTick_Handler(void);     // 【系统滴答中断】SysTick定时器到期时触发，用于系统延时和时间管理
```

##### 3.3 中断优先级说明

这些中断的优先级（从大到小排列）：

| 优先级 | 中断名称 | 说明 |
|--------|----------|------|
| -3（最高） | Reset | 复位中断（固定最高） |
| -2 | NMI | 非屏蔽中断 |
| -1 | HardFault | 硬件故障中断 |
| 可编程 | MemManage | 内存管理故障 |
| 可编程 | BusFault | 总线故障 |
| 可编程 | UsageFault | 用法故障 |
| 可编程 | SVCall | 系统调用 |
| 可编程 | DebugMon | 调试监控 |
| 可编程 | PendSV | 可挂起系统调用 |
| 可编程 | SysTick | 系统滴答 |

**注意**：数字越小优先级越高。Reset、NMI、HardFault 的优先级是固定的，其他可以通过 NVIC 配置。

---

#### 文件 3：stm32f10x_it.c（中断处理实现文件）

**文件路径**：`1-程序源文件/Stm32Code/User/stm32f10x_it.c`

##### 4.1 文件整体功能说明

这个文件是**中断服务函数的实现文件**。它包含了所有系统异常处理函数的**具体代码**。当对应的中断事件发生时，CPU 就会自动跳转到这些函数执行。

##### 4.2 逐行代码分析与注释

```c
/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  */
```
**说明**：文件头注释，说明这是中断服务程序的主文件。

```c
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
```
**说明**：包含中断处理头文件，引入函数声明。

```c
extern volatile unsigned int sysTickUptime;
```
**说明**：
- `extern`：声明一个在其他文件中定义的全局变量
- `volatile`：**重要修饰符**，告诉编译器这个变量可能会被中断程序意外修改，每次使用时都要从内存重新读取，不要用缓存的值
- `unsigned int`：无符号整数类型
- `sysTickUptime`：系统运行时间计数器，每毫秒加1

**为什么需要 `volatile`**：想象编译器为了优化性能，可能会把变量暂时放在 CPU 寄存器中。但如果这个变量在中断里被修改了，寄存器里的值就过时了。`volatile` 就是告诉编译器："这个变量随时可能变，别偷懒，每次都去内存里取最新值。"

```c
/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/
```
**说明**：从这里开始是 Cortex-M3 内核异常处理函数的代码。

##### 4.3 NMI_Handler（非屏蔽中断处理函数）

```c
/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}
```

**说明**：
- `NMI` = Non-Maskable Interrupt（非屏蔽中断）
- 这是**无法被关闭**的中断，优先级仅次于复位
- 这个函数是**空的**，说明本项目没有使用 NMI
- 在 STM32F103 中，NMI 通常连接到时钟安全系统（CSS），当外部晶振故障时触发

**通俗解释**：NMI 就像火警警报，无论你愿不愿意都要响应。这里留个空函数，相当于"现在还没用到这个功能，先占个位置，以后需要时再填代码"。

##### 4.4 HardFault_Handler（硬件故障处理函数）

```c
/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}
```

**说明**：
- **Hard Fault** 是最严重的错误之一，当程序出现以下情况时触发：
  - 访问不存在的内存地址
  - 从非指令区域取指令
  - 总线错误、内存管理错误没有使能处理时
- `while (1) {}`：进入**死循环**，程序停在这里不再执行
- 这种处理方式比较简单粗暴，实际项目中可以加入 LED 闪烁、错误信息记录等调试手段

**通俗解释**：HardFault 就像电脑蓝屏。当程序"发疯"做了不可能的事情（如访问不存在的内存），CPU 就执行这个函数。`while(1)` 相当于让程序"原地冻结"，不再乱跑造成更大破坏。你可以在这里加一个 LED 闪烁来告诉自己："出大事了！"

##### 4.5 MemManage_Handler（内存管理故障处理函数）

```c
/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}
```

**说明**：
- **MemManage Fault** 在使用 MPU（内存保护单元）时出现
- 当程序访问了被 MPU 禁止访问的内存区域时触发
- STM32F103C8T6 **没有 MPU**（这是高端型号才有的功能），所以这个中断实际上不会触发
- 同样进入死循环处理

##### 4.6 BusFault_Handler（总线故障处理函数）

```c
/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}
```

**说明**：
- **Bus Fault** 发生在 CPU 通过总线访问外设或内存失败时
- 常见原因：访问未映射的地址、外设未时钟使能就访问、在调试模式下访问睡眠中的外设
- 同样进入死循环

##### 4.7 UsageFault_Handler（用法故障处理函数）

```c
/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}
```

**说明**：
- **Usage Fault** 是程序执行错误，包括：
  - 执行未定义的指令
  - 非对齐的内存访问（如从奇数地址读取32位数据）
  - 除以零（需要使能才能检测）
- 这些通常是代码 bug 导致的

##### 4.8 SVC_Handler（系统调用处理函数）

```c
/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}
```

**说明**：
- `SVC` = Supervisor Call（监督者调用）
- 通过执行 `SVC` 汇编指令触发
- 常用于操作系统中，用户程序通过 SVC 请求操作系统服务（如任务切换、资源申请）
- 本项目没有使用操作系统，所以留空

##### 4.9 DebugMon_Handler（调试监控器处理函数）

```c
/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}
```

**说明**：
- 用于调试时监控程序状态
- 在调试模式下，当程序暂停、单步执行等操作时触发
- 正常运行时不会触发

##### 4.10 PendSV_Handler（可挂起的系统调用处理函数）

```c
/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
}
```

**说明**：
- `PendSV` = Pending Supervisor Call（可挂起的系统调用）
- 这是操作系统**任务切换**的核心中断
- 它的特殊之处在于可以"延迟执行"——当更高优先级的中断处理完后才执行
- 本项目没有操作系统，所以留空

##### 4.11 SysTick_Handler（系统滴答中断处理函数）

```c
/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
  sysTickUptime++;
}
```

**说明**：
- 这是**最重要的中断函数**！
- SysTick 定时器每 1 毫秒触发一次（通常配置为 1ms）
- `sysTickUptime++`：每次中断将系统运行时间计数器加 1
- 这意味着 `sysTickUptime` 记录了系统上电以来的毫秒数

**通俗解释**：这就是系统的"心跳"。每毫秒"咚"地跳一下，计数器加1。通过这个计数器，程序就可以实现精确延时（如"等500毫秒"就是等计数器增加500）。

**实际应用示例**：
```c
// 延时 500 毫秒的伪代码
uint32_t start = sysTickUptime;
while (sysTickUptime - start < 500) {
    // 等待，什么都不做
}
// 现在已经过了 500 毫秒
```

##### 4.12 外设中断占位函数

```c
/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
```

**说明**：这段注释告诉开发者：如果你需要使用外设中断（如串口接收中断、定时器中断等），在这里添加对应的处理函数。中断函数的名称必须和启动文件（`startup_stm32f10x_xx.s`）中定义的向量表一致。

```c
/*void PPP_IRQHandler(void)
{
}*/
```
**说明**：这是一个被注释掉的模板函数，展示了如何添加外设中断处理函数。`PPP` 代表外设名称的占位符。例如：
- `TIM2_IRQHandler`：定时器2中断
- `USART1_IRQHandler`：串口1中断
- `EXTI0_IRQHandler`：外部中断线0

---

### 第三部分：项目文档分析

---

#### 文档 4：第一次拿到设备使用请看.txt

**文档内容**：

> 1.硬件会配送供电线 但是没有供电头
> 供电头：5v-1A 或者5v-2A 充电宝 电脑 都可以
> 第一次通电 屏幕不亮 请安一下单片机上面的复位按键
> 我们的资料整理的很详细，没多少基础的同学也都可以完全学习使用，所以大家认真研读每个文件夹的资料，都是有用的，可以重点关注文件夹1、2、3、4、5这几个文件里面的资料，跟实物是完全对应的。

> 没有APP的请忽略
> APP只能使用安卓手机安装
> WiFi链接教程：[视频链接]
> 蓝牙链接教程：[视频链接]

##### 文档要点分析

| 要点 | 说明 |
|------|------|
| 供电要求 | 5V/1A 或 5V/2A，可用充电宝或电脑USB供电 |
| 首次使用 | 如果OLED屏幕不亮，按单片机上的复位按键 |
| 资料重点 | 重点关注文件夹 1（程序源码）、2（原理图PCB）、3（元器件清单）、4（方案及视频）、5（思维导图） |
| APP支持 | 仅支持安卓手机，需要WiFi或蓝牙连接 |

---

#### 文档 5：汽车防盗2.txt（项目方案说明）

**文档内容**：

> 2、震动+GPS+三个指示灯+蜂鸣器+正常模式+防盗模式+wifi+app
> 1. STM32F103C8T6单片机进行数据处理
> 2. OLED液晶显示GPS实时数据
> 3. SW-18010P震动传感器检测是否震动
> 4. 北斗GPS检测当前经纬度
> 5. 按键切换模式：按键1设置为正常模式，按键2设置为防盗模式
> 6. 正常模式下，第一个指示灯常亮
> 7. 防盗模式下，oled屏幕上显示防盗模式且第二个指示灯常亮
> 震动传感器检测到震动时，蜂鸣器嘀一声，第三个指示灯慢速闪烁，oled屏幕上显示"发生震动"
> 在2秒内震动次数>=3次时，蜂鸣器开启，第三个指示灯快速闪烁，oled屏幕上显示"非法闯入"
> 8. esp-01s WIFI模块（连接app 远程查看数据+远程控制（防盗模式）开关）

##### 项目功能架构图

```
+------------------+     +------------------+     +------------------+
|   输入设备        |     |   STM32F103C8T6  |     |   输出设备        |
|                  |     |    主控制器       |     |                  |
|  SW-18010P震动   | --> |                  | --> |   OLED显示屏      |
|  传感器           |     |   数据处理中心    |     |   (显示状态/GPS)  |
|                  |     |                  |     |                  |
|  北斗GPS模块      | --> |                  | --> |   3个LED指示灯    |
|  (经纬度)         |     |                  |     |   (状态指示)      |
|                  |     |                  |     |                  |
|  按键1(正常模式)  | --> |                  | --> |   蜂鸣器          |
|  按键2(防盗模式)  | --> |                  |     |   (报警提示)      |
|                  |     |                  |     |                  |
|  ESP-01S WiFi    | --> |                  | --> |   ESP-01S WiFi   |
|  (APP远程控制)    |     |                  |     |   (上传数据)      |
+------------------+     +------------------+     +------------------+
```

##### 项目工作模式说明

**模式一：正常模式**
- 触发方式：按下按键1
- OLED显示：正常模式状态
- LED1（第一个指示灯）：常亮
- 系统状态：不监测震动，即使检测到震动也不报警

**模式二：防盗模式**
- 触发方式：按下按键2
- OLED显示：防盗模式状态
- LED2（第二个指示灯）：常亮
- 监测功能开启：
  - **轻度检测**：震动传感器检测到震动 → 蜂鸣器"嘀"一声 + LED3慢闪 + OLED显示"发生震动"
  - **严重报警**：2秒内震动次数 >= 3次 → 蜂鸣器持续响起 + LED3快速闪烁 + OLED显示"非法闯入"

**远程功能（通过WiFi和APP）**：
- 远程查看当前GPS数据和系统状态
- 远程控制防盗模式的开关

---

#### 文档 6：视频链接.txt（代码讲解）

**内容**：
> https://www.bilibili.com/video/BV1VQJQzME8Y/

**说明**：这是 B 站上的代码大概讲解视频，对整个项目的代码结构和逻辑进行了总体讲解。

---

#### 文档 7：原理图讲解视频.txt

**内容**：
> https://www.bilibili.com/video/BV1RUJQzME1K/

**说明**：这是 B 站上的硬件原理图讲解视频，对电路连接、元器件功能进行了详细说明。

---

#### 文档 8：C语言.txt（学习路线）

**内容**：
> https://www.bilibili.com/video/BV1q54y1q79w/

**说明**：这是推荐的 C 语言基础学习视频链接，适合零基础的同学先学习 C 语言基础，再上手 STM32 开发。

---

#### 文档 9：stm32基础.txt（学习路线）

**内容**：
> https://www.bilibili.com/video/BV1Xs411g7Aj/

**说明**：这是推荐的 STM32 基础入门学习视频链接，在掌握了 C 语言之后，可以通过这个视频学习 STM32 的基础知识和开发方法。

---

#### 文档 10：项目实战.txt（学习路线）

**内容**：
> https://www.bilibili.com/video/BV1qa411d7DW/

**说明**：这是项目实战的学习视频链接，在学习了 C 语言和 STM32 基础之后，可以通过这个视频学习如何完成一个完整的项目。

---

### 第四部分：学习路线建议

根据项目提供的学习资料，建议按照以下路线学习：

#### 阶段一：C 语言基础（1-2 周）
1. 观看 `C语言.txt` 中的视频教程
2. 掌握变量、数据类型、运算符
3. 掌握条件判断（if/else）、循环（for/while）
4. 掌握函数、数组、指针
5. 掌握结构体（struct）

#### 阶段二：STM32 基础（2-3 周）
1. 观看 `stm32基础.txt` 中的视频教程
2. 了解 STM32 的基本架构
3. 学习 GPIO 操作（点灯、读取按键）
4. 学习时钟配置（RCC）
5. 学习中断和定时器
6. 学习串口通信（USART）

#### 阶段三：项目实战（1-2 周）
1. 观看 `项目实战.txt` 中的视频教程
2. 阅读本项目的代码讲解视频（`视频链接.txt`）
3. 对照原理图讲解视频（`原理图讲解视频.txt`）理解硬件连接
4. 动手下载代码到开发板，观察现象
5. 尝试修改代码，实现自己的功能

---

### 第五部分：总结

#### 项目硬件清单

| 序号 | 器件名称 | 型号/规格 | 功能 |
|------|----------|-----------|------|
| 1 | 主控芯片 | STM32F103C8T6 | 数据处理和控制中心 |
| 2 | 显示屏 | OLED 液晶屏 | 显示系统状态和GPS数据 |
| 3 | 震动传感器 | SW-18010P | 检测车辆震动 |
| 4 | GPS模块 | 北斗GPS | 获取当前经纬度 |
| 5 | WiFi模块 | ESP-01S | 连接APP，远程控制 |
| 6 | 蜂鸣器 | 有源蜂鸣器 | 声音报警提示 |
| 7 | LED指示灯 | 3个LED | 状态指示 |
| 8 | 按键 | 2个按键 | 模式切换 |

#### 配置文件总结

| 文件 | 作用 |
|------|------|
| `stm32f10x_conf.h` | 选择需要编译的外设模块 |
| `stm32f10x_it.h` | 声明中断处理函数 |
| `stm32f10x_it.c` | 实现中断处理函数（包括系统心跳计数） |

#### 关键代码点

1. **SysTick_Handler** 是整个系统的时间基础，每毫秒执行一次，维护系统运行时间
2. **HardFault_Handler** 等错误处理采用死循环策略，防止程序跑飞
3. 所有外设头文件都被包含，采用"全引入"策略简化开发
4. `assert_param` 默认关闭，如需调试可开启 `USE_FULL_ASSERT`

---

*分析完成。本文档涵盖了 STM32 汽车防盗系统的配置文件、中断处理代码和项目文档的详细说明。*



---

# 第七章：项目使用指南

> 恭喜你已经完成了这个汽车防盗系统的全部学习！本章将手把手教你如何使用这个系统，从硬件准备到日常操作，再到遇到问题时如何排查。即使这是你第一次接触嵌入式设备，按照本章的步骤一步步来，也能轻松上手。

---

## 7.1 硬件准备

### 7.1.1 你需要准备哪些东西？

把你的汽车防盗系统想象成一台小型电脑，它需要"电源"才能工作。以下是完整的硬件清单：

| 物品 | 规格要求 | 说明 |
|------|---------|------|
| **电源** | 5V/1A 或 5V/2A | 给整个系统供电，下面会详细介绍 |
| **STM32开发板** | 已烧录好程序 | 系统的"大脑"，本项目已焊接好所有模块 |
| **GPS天线** | 有源GPS天线 | 接收卫星信号，用于定位 |
| **Android手机** | Android 9.0及以上 | 安装APP，远程监控车辆状态 |

### 7.1.2 电源要求详解

**为什么必须是5V？**

STM32单片机的工作电压是3.3V，但开发板上有稳压芯片，可以将5V转换为3.3V。如果直接用3.3V供电，可能因为电压不足导致工作不稳定；如果用超过5V的电源（比如9V或12V），则可能烧毁芯片。

**电流要求：1A还是2A？**

| 场景 | 推荐电流 | 原因 |
|------|---------|------|
| 只用开发板，不连手机热点 | 5V/1A | 系统本身功耗不大，1A足够 |
| 需要WiFi连接（ESP8266工作） | 5V/2A | WiFi模块在发射数据时功耗较大 |

**通俗理解**：电压就像水压，必须匹配（5V）；电流就像水管的粗细，设备需要多少就会取多少，电源标称的2A表示"最大能提供2A"，不是说一定会输出2A。所以用5V/2A的电源接一个小设备是安全的，但用5V/0.5A的电源接大设备就可能"供不上水"。

### 7.1.3 可用的电源类型

以下电源都可以使用，根据你的实际情况选择最方便的：

**① 手机充电头 + USB线**
- 最推荐的方式，稳定可靠
- 任何手机的USB充电头都可以（上面标有5V的就是）
- 用Micro-USB或Type-C线（根据你的开发板接口）连接

**② 电脑USB口**
- 直接用USB数据线把开发板插到电脑上
- 电脑USB口标准输出是5V/500mA（USB 2.0）或5V/900mA（USB 3.0）
- 如果不用WiFi功能，电脑USB口一般够用
- 优点：可以同时用串口调试助手看输出信息

**③ 充电宝**
- 户外测试时的最佳选择
- 选择输出为5V/1A或5V/2.1A的接口
- 注意：有些充电宝在电流太小时会自动断电，如果开发板功耗低可能导致充电宝误判为"已充满"而断电

**④ 锂电池 + 升压模块**
- 如果想做 portable（便携式）版本
- 需要3.7V锂电池 + 5V升压模块（如MT3608）
- 适合安装在车辆上长期使用

### 7.1.4 首次通电注意事项

**重要提示**：

1. **检查接线**：通电前再次检查所有模块是否插好，特别是ESP8266 WiFi模块和GPS模块
2. **天线要接好**：GPS天线一定要接到GPS模块的天线接口上，否则无法定位
3. **第一次通电**：如果屏幕不亮，不要慌张！按一下单片机开发板上的 **复位按键（RST）**。复位按键通常标有"RST"字样，是一个小按钮

**为什么第一次可能不亮？**

这就像电脑有时候开机黑屏，按一下重启键就好了。原因可能是：
- 电源上电时序导致初始化失败
- OLED屏幕需要一次复位才能正常工作
- 某些模块还没准备好，STM32就已经完成了初始化

**复位方法**：找到开发板上的RST按钮，快速按一下然后松开（就像按门铃一样，不要长按）。你会看到LED灯闪烁一下，然后OLED屏幕应该会正常显示。

---

## 7.2 首次使用步骤

本节将带你完成从开箱到正常使用的全过程。按照步骤一步步来，大约5分钟就能搞定。

### 步骤1：给系统上电

1. 找到开发板上的电源接口（Micro-USB或Type-C）
2. 用USB线连接开发板和电源（充电头/电脑USB口/充电宝）
3. 观察开发板上的电源指示灯（通常是红色LED）是否亮起
4. 如果电源灯不亮，检查USB线是否插紧，或者换一个USB口试试

**电源指示灯亮了但屏幕不亮怎么办？**

按照7.1.4节的方法，按一下RST复位按键。

### 步骤2：观察OLED屏幕显示

上电复位后，OLED屏幕会显示**第一页**的内容：

```
+------------------------+
|      汽车防盗          |  ← 第1行：系统名称
|  经度: XXX.XXXXXX      |  ← 第2行：经度值
|  纬度: XXX.XXXXXX      |  ← 第3行：纬度值
+------------------------+
```

**各字段含义**：
- **汽车防盗**：系统的名称，告诉你现在运行的是防盗监测系统
- **经度**: GPS定位的经度坐标（东西方向位置），首次定位可能需要几分钟
- **纬度**: GPS定位的纬度坐标（南北方向位置）

**首次使用GPS的注意事项**：
- GPS模块**第一次启动**（冷启动）时，需要搜索卫星信号，这个过程可能需要 **2~5分钟**
- 如果在室内，GPS信号可能很弱或收不到，建议到**室外开阔地带**进行首次定位
- 看到经纬度显示为0.000000或不变，说明GPS还没定位成功，耐心等待
- 首次定位成功后，后续启动会快很多（热启动约10~30秒）

### 步骤3：按按键切换显示页面

开发板上有 **4个按键**，分别标有 KEY1、KEY2、KEY3、KEY4（或 SW1、SW2、SW3、SW4）。

每个按键对应一个显示页面，按一下就会切换到对应的页面：

| 按键 | 页面名称 | 显示内容 |
|------|---------|---------|
| **KEY1** | 数据显示页 | 汽车防盗标题 + 经度 + 纬度 |
| **KEY2** | 防盗模式页 | 防盗模式标题 + 振动/入侵状态 |
| **KEY3** | MQTT状态页 | WiFi连接状态 + MQTT连接参数 |
| **KEY4** | 主题参数页 | 订阅主题(Sub) + 发布主题(Pub) |

**切换页面的方法**：

按一下对应的按键，屏幕会先清屏，然后显示新页面的内容。

**示例**：当前显示第一页（数据显示），按一下 **KEY2**，屏幕会切换到防盗模式页面：

```
+------------------------+
|      防盗模式          |  ← 第1行：页面标题
|                        |  ← 第2行：（空，预留）
|  发生震动              |  ← 第3行：检测到振动时显示
+------------------------+
```

### 步骤4：连接WiFi（如果需要远程监控）

如果你需要通过手机APP远程监控车辆状态，需要让开发板连接到一个WiFi网络。

**WiFi连接方式**：

本系统使用ESP8266 WiFi模块进行网络连接。WiFi的名称和密码已经预先写在程序中（在`esp8266_drv.h`文件中定义）。

**默认WiFi配置**：
- WiFi热点名称（SSID）：`abcd`
- WiFi密码：`12345678`

**连接步骤**：

1. 用另一部手机（或路由器）开启WiFi热点
2. 将热点名称改为 `abcd`，密码设为 `12345678`
3. 开发板上电后，会自动搜索并连接这个WiFi
4. 连接成功后，ESP8266会自动连接到MQTT服务器

**如何知道WiFi是否连接成功？**

按 **KEY3** 切换到MQTT状态页面，如果看到连接参数显示，说明WiFi和MQTT都已连接成功。

**如果想修改WiFi名称和密码**：

需要修改代码中的宏定义，然后重新编译下载程序。具体步骤见7.5节"开发环境搭建"。

```c
// 在 esp8266_drv.h 文件中
#define ID         "abcd"        // 改成你的WiFi名称
#define PASSWORD   "12345678"    // 改成你的WiFi密码
```

---

## 7.3 操作说明

### 7.3.1 4个按键的功能说明

本节详细介绍每个按键的具体功能和使用场景。

```
        [KEY1]        [KEY2]        [KEY3]        [KEY4]
         |             |             |             |
    +---------+   +---------+   +---------+   +---------+
    | 数据页面 |   | 防盗模式 |   | MQTT状态|   | 主题参数 |
    |display=0|   |display=1|   |display=2|   |display=3|
    +---------+   +---------+   +---------+   +---------+
         |             |             |             |
    回到主页面     开启防盗       查看网络       查看主题
    关闭报警       监测入侵       连接状态       参数信息
```

#### KEY1 —— 数据显示页 / 复位键

**功能**：
- 切换到数据显示页面（显示GPS经度和纬度）
- **关闭防盗模式**（如果正在防盗监测中，按KEY1会退出防盗模式）
- **关闭报警**（如果蜂鸣器在响、LED在闪，按KEY1会停止报警）

**什么时候用**：
- 想查看车辆当前位置时
- 防盗模式误触发，想关闭报警时
- 想从其他页面回到主页面时

**按下后的效果**：
- OLED清屏，显示"汽车防盗"标题
- 显示当前经度和纬度
- LED停止闪烁
- 蜂鸣器停止鸣叫
- `System.Switch3 = 0`（报警标志位清零）

#### KEY2 —— 防盗模式页

**功能**：
- 切换到防盗模式页面
- **启动防盗监测功能**（振动检测）

**什么时候用**：
- 停车后想开启防盗监测时
- 想查看防盗模式的工作状态时

**按下后的效果**：
- OLED清屏，显示"防盗模式"标题
- LED1熄灭，LED2常亮（表示当前处于防盗模式）
- 振动传感器开始工作，检测车辆振动
- 如果在2秒内检测到3次及以上振动，触发报警

**防盗模式的检测逻辑**：

```
检测到第1次振动
    → 蜂鸣器"嘀"一声（Short_Beep）
    → LED开始慢速闪烁（SLOW_BLINK）
    → OLED显示"发生震动"
    → 记录第一次振动的时间

如果在2秒内又检测到2次及以上振动（总共≥3次）
    → 蜂鸣器持续鸣叫（BEEP_ON）
    → LED开始快速闪烁（FAST_BLINK）
    → OLED显示"非法入侵"
    → 通过MQTT发送报警信息到手机APP

检测到第1次振动后10秒
    → 自动停止报警
    → LED熄灭
    → 蜂鸣器关闭
    → 清空振动计数
```

#### KEY3 —— MQTT连接状态页

**功能**：
- 切换到MQTT连接状态页面
- 查看WiFi和MQTT的连接情况

**什么时候用**：
- 想知道WiFi是否连接成功时
- 想查看MQTT连接参数时
- APP收不到数据，排查网络问题时

**按下后的效果**：
- OLED清屏，显示MQTT连接状态
- 显示的内容包括：
  - WiFi连接状态（是否已连接到WiFi路由器）
  - MQTT服务器连接状态
  - 设备名称（device_name）
  - 设备ID

#### KEY4 —— 订阅主题参数页

**功能**：
- 切换到主题参数页面
- 查看MQTT订阅主题（Sub）和发布主题（Pub）

**什么时候用**：
- 配置APP时需要知道主题名称
- 想确认MQTT主题是否正确时

**按下后的效果**：
- OLED清屏，显示主题参数
- 显示的内容包括：
  - Sub主题（订阅主题，APP向这个主题发命令）
  - Pub主题（发布主题，设备向这个主题发数据）

### 7.3.2 如何开启/关闭防盗模式

**开启防盗模式**：

方法一：按开发板上的 **KEY2** 按键
- 屏幕切换到"防盗模式"页面
- 系统开始监测振动

方法二：通过手机APP远程开启
- 打开APP，打开"防盗模式"开关
- APP通过MQTT发送命令给开发板
- 开发板自动进入防盗模式

**关闭防盗模式**：

方法一：按开发板上的 **KEY1** 按键
- 屏幕切换回"数据显示"页面
- 停止振动检测，关闭报警

方法二：通过手机APP远程关闭
- 打开APP，关闭"防盗模式"开关
- APP通过MQTT发送命令给开发板
- 开发板退出防盗模式，回到数据显示页面

**防盗模式的工作流程图**：

```
                    ┌─────────────────┐
                    │    正常模式      │
                    │ (KEY1数据显示页) │
                    └────────┬────────┘
                             │ 按KEY2 / APP开启
                             ▼
                    ┌─────────────────┐
                    │    防盗模式      │
                    │ (KEY2防盗监测页) │
                    │   LED2常亮      │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
        ┌─────────┐   ┌──────────┐   ┌──────────┐
        │ 无振动   │   │ 1次振动   │   │ ≥3次/2秒 │
        │ 正常等待 │   │ 慢闪+嘀声 │   │  非法入侵 │
        └─────────┘   └──────────┘   └──────────┘
                             │              │
                             ▼              ▼
                        10秒后自动恢复 ←─┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  按KEY1关闭防盗  │
                    │  回到正常模式    │
                    └─────────────────┘
```

### 7.3.3 如何查看GPS坐标

**在开发板上查看**：

1. 按 **KEY1** 切换到数据显示页面
2. 看屏幕上的"经度"和"纬度"两行
3. 格式示例：`经度: 113.562341`，表示东经113.562341度

**在手机APP上查看**：

1. 打开APP，确保已连接到MQTT服务器
2. 在主界面的"经度"和"纬度"卡片中查看坐标
3. 坐标会实时更新（每秒一次）

**GPS坐标的含义**：

| 数值 | 含义 | 范围 |
|------|------|------|
| 经度（Longitude） | 东西方向位置 | -180° ~ +180°（东经为正） |
| 纬度（Latitude） | 南北方向位置 | -90° ~ +90°（北纬为正） |

**示例坐标**：
- 经度: 113.562341°E（东经113度）→ 在中国广州附近
- 纬度: 23.162451°N（北纬23度）→ 在中国广州附近

**如果GPS坐标显示为0或不变**：
- GPS正在搜索卫星，请耐心等待（首次可能需要2-5分钟）
- 检查GPS天线是否接好
- 到室外开阔地带使用

### 7.3.4 LED灯状态含义

开发板上有多个LED指示灯，每个灯有不同的含义：

#### LED1 —— 正常模式指示灯

| 状态 | 含义 |
|------|------|
| **常亮** | 系统正常工作中（数据显示页面） |
| **熄灭** | 当前处于防盗模式 |

#### LED2 —— 防盗模式指示灯

| 状态 | 含义 |
|------|------|
| **常亮** | 防盗模式已开启，正在监测 |
| **熄灭** | 防盗模式已关闭 |

#### LED3 —— 报警/状态指示灯

| 状态 | 含义 |
|------|------|
| **熄灭** | 无异常，系统正常 |
| **慢速闪烁** | 检测到1次振动（预警状态） |
| **快速闪烁** | 检测到非法入侵（报警状态） |

**LED状态变化总结表**：

| 系统状态 | LED1 | LED2 | LED3 |
|---------|------|------|------|
| 正常模式（数据显示页） | 常亮 | 熄灭 | 熄灭 |
| 防盗模式（等待中） | 熄灭 | 常亮 | 熄灭 |
| 防盗模式（1次振动） | 熄灭 | 常亮 | 慢速闪烁 |
| 防盗模式（非法入侵） | 熄灭 | 常亮 | 快速闪烁 |

---

## 7.4 APP使用说明

### 7.4.1 Android手机安装APP

**APP基本信息**：
- APP名称：**汽车防盗监测**
- 包名：`cacom.example.smarthome`
- 最低支持Android版本：Android 9.0（API 28）
- 需要权限：网络权限、震动权限

**安装步骤**：

1. **获取APK文件**
   - 从项目文件夹中找到 `app-release.apk`（或debug版本的APK）
   - 或者自己用Android Studio编译项目生成APK

2. **允许安装未知来源应用**
   - Android手机默认只允许安装应用商店的APP
   - 需要在手机设置中开启"允许安装未知来源应用"
   - 设置路径：`设置 → 安全 → 未知来源`（不同手机路径可能略有不同）

3. **安装APK**
   - 把APK文件发送到手机上（通过微信、QQ、数据线等方式）
   - 在手机上点击APK文件，按提示完成安装
   - 安装完成后，桌面上会出现"汽车防盗监测"的图标

**首次打开APP**：

点击桌面图标打开APP，会弹出一个对话框要求输入MQTT主题。

### 7.4.2 连接WiFi教程

为了让开发板能够连接互联网并与APP通信，需要配置WiFi连接。

**视频教程**：

详细操作步骤请观看视频教程：

> **WiFi连接教程视频**：[https://www.bilibili.com/video/BV1kh8LzGEdn](https://www.bilibili.com/video/BV1kh8LzGEdn)

**文字版教程**：

1. 准备一部手机作为WiFi热点
2. 打开手机的"个人热点"功能
3. 将热点名称设置为 `abcd`，密码设置为 `12345678`
4. 开发板上电后会自动搜索并连接这个热点
5. 连接成功后，ESP8266会自动连接到MQTT服务器

### 7.4.3 蓝牙连接教程

**视频教程**：

> **蓝牙连接教程视频**：[https://www.bilibili.com/video/BV1dA8LzhE2f](https://www.bilibili.com/video/BV1dA8LzhE2f)

### 7.4.4 输入MQTT主题的步骤

首次打开APP时，会弹出主题输入对话框：

```
+---------------------------+
|      首次输入              |
|                           |
|  Sub: [_______________]   |  ← 订阅主题输入框
|  Pub: [_______________]   |  ← 发布主题输入框
|                           |
|      [取消]   [确定]       |
+---------------------------+
```

**Sub和Pub是什么？**

在MQTT通信中，Topic（主题）就像"微信群的名字"：
- **Sub（订阅主题）**：你加入这个群，可以收到群里发的消息
- **Pub（发布主题）**：你往这个群发消息，群里的人都能看到

**如何获取主题名称**：

1. 在开发板上按 **KEY4**，切换到主题参数页面
2. 屏幕上会显示两个主题名称，格式类似：
   - Sub: `sub123456`
   - Pub: `pub123456`
3. 把这两个主题分别填到APP的输入框中
4. 点击"确定"保存

**重要**：Sub和Pub主题必须与开发板上显示的一致，否则APP和设备之间无法通信！

**主题保存功能**：

APP会自动保存你输入的主题，下次打开时不需要重新输入。对话框标题会显示"确认主题"，如果主题没变，直接点"确定"即可。

### 7.4.5 如何开启防盗模式

1. 打开APP，确保已连接到MQTT服务器（顶部会显示连接状态）
2. 找到"防盗模式"一行，右侧有一个开关按钮
3. 向右滑动开关，开启防盗模式

```
+-------------------------------------+
|  汽车防盗监测         连接成功 ✓      |
+-------------------------------------+
|                                     |
|  [经度卡片]       [纬度卡片]         |
|                                     |
|  | 防盗模式                [○——→]  |  ← 开关向右 = 开启
|                                     |
+-------------------------------------+
```

**开启后的效果**：
- 开发板上的OLED屏幕会切换到"防盗模式"页面
- 开发板开始监测振动传感器
- APP上的状态显示区域会出现（显示振动/入侵状态）

**关闭防盗模式**：

向左滑动开关即可关闭，开发板会回到数据显示页面。

### 7.4.6 如何查看报警信息

当开发板检测到异常情况时，会通过MQTT发送报警信息到APP。

**报警信息类型**：

| 报警类型 | APP显示 | 触发条件 |
|---------|--------|---------|
| 发生震动 | 状态区显示"发生震动" | 检测到1次振动 |
| 非法入侵 | 状态区显示"非法入侵" + 手机震动 | 2秒内检测到≥3次振动 |

**报警信息查看方式**：

1. **APP界面显示**：在主界面的"状态显示区域"（"防盗模式"开关下方）可以看到当前状态
2. **手机震动**：当检测到"非法入侵"时，手机会震动报警（需要APP有震动权限）
3. **GPS坐标同步**：报警时，APP上的经度和纬度会实时更新，显示车辆当前位置

**状态显示区域的位置**：

```
+-------------------------------------+
|  | 防盗模式                [开关]   |
|                                     |
|  +-------------------------------+  |
|  |                               |  |
|  |       发生震动 / 非法入侵       |  ← 状态显示区域
|  |                               |  |
|  +-------------------------------+  |
+-------------------------------------+
```

---

## 7.5 开发环境搭建

如果你想修改代码、添加新功能，或者重新编译下载程序，需要搭建开发环境。本节详细介绍每个步骤。

### 7.5.1 Keil MDK-ARM安装说明

**什么是Keil MDK-ARM？**

Keil MDK-ARM（Microcontroller Development Kit）是嵌入式开发领域最流行的集成开发环境（IDE）之一，专门用于ARM Cortex-M系列单片机的开发。你可以把它理解为专门给单片机写程序的"Visual Studio"。

**安装步骤**：

#### 步骤1：下载Keil MDK-ARM

1. 访问Keil官方网站：https://www.keil.com/download
2. 找到 MDK-ARM 版本，点击下载
3. 目前最新版本是 MDK-ARM 5.38 或更高版本
4. 文件大小约 800MB

#### 步骤2：安装Keil MDK-ARM

1. 双击下载的安装包，启动安装向导
2. 点击 "Next" 继续
3. **同意许可协议**：勾选 "I agree to all the terms..."，点击 "Next"
4. **选择安装路径**：
   - 默认路径是 `C:\Keil_v5`
   - 建议保持默认，不要修改
   - 点击 "Next"
5. **填写个人信息**：
   - First Name / Last Name：随便填
   - Company Name：随便填
   - Email：填一个有效的邮箱
   - 点击 "Next"
6. **等待安装完成**：这个过程大约需要5-10分钟
7. 安装完成后，取消勾选 "Show Release Notes"，点击 "Finish"

#### 步骤3：安装STM32设备支持包（Device Family Pack）

Keil安装好后，还需要安装对应芯片的支持包，才能识别STM32F103系列芯片。

**方法一：在线安装（推荐，需要联网）**

1. 打开Keil，点击菜单栏的 `Project → Manage → Pack Installer`
2. 在Pack Installer窗口左侧找到 `STMicroelectronics → STM32F1 Series`
3. 展开后找到 `Keil::STM32F1xx_DFP`
4. 点击右侧的 `Install` 按钮
5. 等待下载和安装完成

**方法二：离线安装（如果无法联网）**

1. 从Keil官网下载离线包：STM32F1xx_DFP.x.x.x.pack
2. 下载地址：https://www.keil.com/dd2/pack/
3. 双击下载的 `.pack` 文件，自动安装

#### 步骤4：注册License（破解）

Keil是商业软件，需要注册才能编译超过32KB的代码。以下为学习用途的注册方法：

1. 以管理员身份运行Keil
2. 点击菜单栏 `File → License Management`
3. 复制Computer ID（CID）
4. 使用注册机生成License ID（LIC）
5. 将LIC粘贴到License Management窗口的 "New License ID Code (LIC)" 框中
6. 点击 "Add LIC"，显示 "LIC Added Successfully" 即注册成功

### 7.5.2 STM32库文件配置

STM32开发有两种方式：使用标准外设库（SPL）或使用HAL库。本项目使用的是**标准外设库**。

**什么是标准外设库？**

标准外设库（Standard Peripheral Library）是ST公司官方提供的C语言函数库，封装了对STM32各种硬件外设（GPIO、USART、TIM等）的操作。使用库函数比直接操作寄存器更容易理解和使用。

**库文件配置步骤**：

#### 步骤1：获取STM32标准外设库

1. 访问ST官网：https://www.st.com
2. 搜索 "STM32F10x Standard Peripheral Library"
3. 下载 `STM32F10x_StdPeriph_Lib_V3.5.0.zip`
4. 解压到本地目录，如 `D:\STM32F10x_StdPeriph_Lib_V3.5.0`

#### 步骤2：创建项目文件夹结构

在电脑上创建一个新的项目文件夹，建议按以下结构组织：

```
Car_AntiTheft/              ← 项目根目录
├── Libraries/              ← 库文件目录
│   ├── CMSIS/              ← CMSIS核心文件
│   │   ├── CM3/
│   │   │   ├── CoreSupport/        ← 核心支持文件 (core_cm3.c/h)
│   │   │   └── DeviceSupport/      ← 设备支持文件 (system_stm32f10x.c/h, startup_stm32f10x_hd.s)
│   │   │       └── ST/
│   │   │           └── STM32F10x/
│   │   └── STM32F10x.h             ← 头文件
│   └── STM32F10x_StdPeriph_Driver/ ← 标准外设驱动
│       ├── inc/                      ← 头文件 (.h)
│       │   ├── stm32f10x_gpio.h
│       │   ├── stm32f10x_usart.h
│       │   ├── stm32f10x_rcc.h
│       │   ├── stm32f10x_tim.h
│       │   └── ...
│       └── src/                      ← 源文件 (.c)
│           ├── stm32f10x_gpio.c
│           ├── stm32f10x_usart.c
│           ├── stm32f10x_rcc.c
│           ├── stm32f10x_tim.c
│           └── ...
├── User/                    ← 用户代码目录
│   ├── main.c               ← 主函数
│   ├── stm32f10x_it.c       ← 中断服务函数
│   ├── stm32f10x_conf.h     ← 外设配置文件
│   └── system_core/         ← 系统核心文件
│       ├── sys.c
│       ├── sys.h
│       ├── delay.c
│       ├── delay.h
│       ├── usart.c
│       └── usart.h
├── Hardware/                ← 硬件驱动目录
│   ├── bsp_oled_iic.c
│   ├── bsp_oled_iic.h
│   ├── oledFont.h
│   ├── gps.c
│   ├── gps.h
│   ├── bsp_beep.c
│   ├── bsp_beep.h
│   ├── led.c
│   ├── led.h
│   ├── key.c
│   ├── key.h
│   ├── shake.c
│   ├── shake.h
│   ├── esp8266_drv.c
│   ├── esp8266_drv.h
│   ├── mqtt_drv.c
│   └── mqtt_drv.h
├── Startup/                 ← 启动文件
│   └── startup_stm32f10x_hd.s
└── Project/                 ← Keil工程文件
    └── Car_AntiTheft.uvprojx
```

#### 步骤3：在Keil中配置工程

1. **新建工程**：
   - 打开Keil，点击 `Project → New μVision Project`
   - 选择 `Project` 文件夹，输入工程名 `Car_AntiTheft`，点击保存

2. **选择芯片型号**：
   - 在设备选择窗口中，展开 `STMicroelectronics → STM32F1 Series → STM32F103 → STM32F103ZE`
   - 选择 `STM32F103ZE`（或你的开发板实际使用的型号，如C8T6、RCT6等）
   - 点击 "OK"
   - 弹出的 "Manage Run-Time Environment" 窗口直接点击 "Cancel"

3. **添加文件到工程**：
   - 在左侧工程窗口中，右键点击 `Target 1`，选择 `Manage Project Items`
   - 创建以下文件组（Groups）：
     - `Startup`：添加启动文件 `startup_stm32f10x_hd.s`
     - `CMSIS`：添加 `core_cm3.c`、`system_stm32f10x.c`
     - `StdPeriph`：添加所有库文件 `stm32f10x_xxx.c`
     - `User`：添加 `main.c`、`stm32f10x_it.c`
     - `System`：添加 `sys.c`、`delay.c`、`usart.c`等系统文件
     - `Hardware`：添加所有硬件驱动文件

4. **配置编译选项**：
   - 点击 `Options for Target` 按钮（魔术棒图标）
   - 在 `Target` 标签页：
     - IROM1: Start `0x8000000`, Size `0x80000`（512KB）
     - IRAM1: Start `0x20000000`, Size `0x10000`（64KB）
   - 在 `Output` 标签页：
     - 勾选 "Create HEX File"
   - 在 `C/C++` 标签页：
     - Define: 填入 `STM32F10X_HD, USE_STDPERIPH_DRIVER`
     - Include Paths: 添加所有头文件路径
   - 在 `Debug` 标签页：
     - 选择调试器（ST-Link Debugger 或 J-Link）

#### 步骤4：配置头文件

`stm32f10x_conf.h` 是外设配置文件，用于启用需要使用的标准外设库模块：

```c
// stm32f10x_conf.h
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

// 启用的外设模块
#include "stm32f10x_adc.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_can.h"
#include "stm32f10x_cec.h"
#include "stm32f10x_crc.h"
#include "stm32f10x_dac.h"
#include "stm32f10x_dbgmcu.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_fsmc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_iwdg.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_sdio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_wwdg.h"
#include "misc.h"

// 断言开关（调试时开启，发布时关闭）
#define USE_FULL_ASSERT    1

#endif
```

### 7.5.3 代码编译和下载步骤

#### 编译代码

1. 在Keil中打开工程文件（`.uvprojx`）
2. 点击工具栏上的 `Rebuild` 按钮（双箭头图标）
3. 等待编译完成
4. 查看Build Output窗口：
   - 如果显示 `0 Error(s), 0 Warning(s)`，编译成功！
   - 如果有错误，双击错误信息会跳转到对应代码行，修改后重新编译

**常见编译错误及解决方法**：

| 错误信息 | 原因 | 解决方法 |
|---------|------|---------|
| `cannot open source input file "xxx.h"` | 头文件路径未添加 | 在Options→C/C++→Include Paths中添加 |
| `undefined symbol xxx` | 函数未定义或文件未添加 | 检查对应的.c文件是否已添加到工程 |
| `multiple definition of xxx` | 函数重复定义 | 检查是否有同名函数 |
| `expected expression` | 语法错误 | 检查缺少的分号、括号等 |

#### 下载程序到开发板

**使用ST-Link下载器（推荐）**：

1. **硬件连接**：
   - 将ST-Link的SWD接口与开发板连接：
     - ST-Link SWDIO → 开发板 SWDIO
     - ST-Link SWCLK → 开发板 SWCLK
     - ST-Link GND → 开发板 GND
     - ST-Link 3.3V → 开发板 3.3V（可选，如果开发板已独立供电可不接）

2. **Keil配置**：
   - 点击 `Options for Target → Debug`
   - 选择 `ST-Link Debugger`
   - 点击 `Settings`：
     - Port: `SW`（Serial Wire）
     - Max Clock: `4MHz`
     - 如果右侧显示芯片信息，说明连接成功

3. **下载程序**：
   - 点击 `Download` 按钮（向下箭头图标）
   - 或者按快捷键 `F8`
   - 等待下载完成，看到 "Programming Done. Verify OK." 即成功

**使用串口下载（没有ST-Link时）**：

1. **硬件连接**：
   - 用USB转TTL模块连接开发板的USART1：
     - USB-TTL TXD → 开发板 PA10（USART1_RX）
     - USB-TTL RXD → 开发板 PA9（USART1_TX）
     - USB-TTL GND → 开发板 GND

2. **设置启动模式**：
   - 将开发板上的BOOT0引脚接高电平（3.3V）
   - BOOT1引脚接低电平（GND）
   - 按复位键，进入系统存储器启动模式（用于串口下载）

3. **使用FlyMcu或STMFlashLoader下载**：
   - 打开FlyMcu软件
   - 选择串口号（USB-TTL对应的COM口）
   - 波特率选择 `115200`
   - 选择编译生成的 `.hex` 文件
   - 勾选 "编程前重装文件"、"编程后执行"、"使用Ramisp"
   - 点击 "开始编程(P)" 按钮
   - 下载完成后，将BOOT0接回GND，按复位键正常启动

### 7.5.4 串口调试助手使用

**什么是串口调试助手？**

串口调试助手是一个电脑上的软件，用来查看单片机通过串口发送出来的文字信息。就像单片机的"打印屏幕"，可以把程序运行中的状态、变量值等信息打印出来，方便调试。

**常用的串口调试助手**：

| 软件名称 | 特点 | 下载地址 |
|---------|------|---------|
| XCOM V2.0 | 功能全面，界面简洁 | 正点原子官网 |
| SSCOM | 老牌工具，稳定可靠 | 网络上搜索 |
| 野火多功能调试助手 | 支持多种协议 | 野火官网 |
| putty | 开源免费，功能强大 | putty.org |

**使用步骤（以XCOM为例）**：

1. **硬件连接**：
   - 用USB转TTL模块连接开发板的USART1（PA9/PA10）
   - 将USB-TTL插到电脑上

2. **安装驱动**：
   - 如果电脑第一次使用这个USB-TTL模块，可能需要安装CH340或CP2102驱动
   - 驱动安装成功后，在设备管理器中能看到对应的COM口号

3. **打开XCOM**：
   - 选择正确的COM口号
   - 波特率设置为 `9600`（USART1的波特率）
   - 数据位: 8，停止位: 1，校验位: 无
   - 点击 "打开串口"

4. **查看输出**：
   - 开发板复位后，会在串口助手上打印调试信息
   - 可以看到GPS数据、WiFi连接状态、MQTT通信日志等

**串口调试助手界面说明**：

```
+------------------------------------------------------+
| XCOM V2.0 串口调试助手                                |
+------------------------------------------------------+
| 端口: [COM3 ▼]  波特率: [9600 ▼]  数据位: [8 ▼]     |
| 停止位: [1 ▼]   校验: [无 ▼]     流控: [无 ▼]       |
| [打开串口]  [关闭串口]                               |
+------------------------------------------------------+
|                                                      |
| $GNRMC,123519,A,4807.038,N,01131.000,E...          | ← 收到的数据
| WiFi Connected!                                      |
| MQTT Connected!                                      |
| GPS: 113.562341, 23.162451                           |
|                                                      |
+------------------------------------------------------+
| [发送] [清空接收] [保存数据]                          |
+------------------------------------------------------+
```

---

## 7.6 常见问题解答（FAQ）

本节收集了使用过程中最常见的问题，按照问题现象分类，帮助你快速定位和解决。

### 7.6.1 屏幕不亮怎么办？

**问题现象**：给开发板上电后，OLED屏幕完全不显示，一片漆黑。

**排查步骤**：

**Step 1：检查电源**
- 用万用表测量开发板上的3.3V引脚，看是否有3.3V电压
- 如果没有电压，检查USB线是否完好（有些USB线只有充电功能，没有数据线）
- 换一个USB口或换一根USB线试试

**Step 2：检查程序是否运行**
- 观察开发板上的LED灯是否有反应
- 如果有LED闪烁，说明程序在运行，可能是OLED的问题
- 如果所有LED都不亮，可能是程序没有正确下载或芯片损坏

**Step 3：按复位键**
- 找到开发板上的RST按键，快速按一下
- 如果屏幕亮了，说明是初始化时序问题，属于正常现象

**Step 4：检查OLED硬件连接**
- 确认OLED模块的4根线是否插好：
  - VCC → 3.3V
  - GND → GND
  - SCL → PA12
  - SDA → PA11
- 重新插拔OLED排线

**Step 5：检查I2C地址**
- 有些OLED模块的I2C地址是0x3C，有些是0x3D
- 在代码中修改地址尝试：
  ```c
  // bsp_oled_iic.c 中
  #define OLED_ADDRESS 0x78  // 0x3C<<1 = 0x78，如果是0x3D则改为0x7A
  ```

**Step 6：重新烧录程序**
- 可能是程序损坏或烧录不完整
- 用ST-Link重新下载程序

### 7.6.2 WiFi连接不上怎么办？

**问题现象**：按KEY3查看MQTT状态，显示WiFi未连接；或者ESP8266没有响应。

**排查步骤**：

**Step 1：检查ESP8266模块**
- 确认ESP8266模块正确插在开发板上
- 模块上的蓝色LED在上电时会闪烁一下，表示模块正常
- 如果没有闪烁，检查模块是否插反或损坏

**Step 2：检查WiFi热点**
- 确认WiFi热点已开启，名称和密码与代码中定义的一致
- 注意区分大小写！`abcd` 和 `ABCD` 是不同的
- 确认WiFi是2.4GHz频段，ESP8266**不支持5GHz**
- 热点不要设置隐藏SSID

**Step 3：检查WiFi名称和密码**
- 打开代码中的 `esp8266_drv.h` 文件：
  ```c
  #define ID   "abcd"        // 检查是否与你的热点名称一致
  #define PASSWORD   "12345678"  // 检查密码是否正确
  ```
- 修改后重新编译下载

**Step 4：使用串口调试查看AT指令响应**
- 连接USART1到电脑，打开串口调试助手（波特率9600）
- 查看ESP8266的AT指令执行日志
- 如果看到 "ERROR" 或 "FAIL"，说明AT指令执行失败

**Step 5：复位ESP8266**
- 代码中可以通过PA1引脚控制ESP8266的硬件复位
- 也可以手动断电重启整个系统

**Step 6：检查ESP8266固件版本**
- 有些ESP8266模块的AT指令集版本较老，可能不支持某些指令
- 可以通过串口发送 `AT+GMR` 查询固件版本
- 如果版本太老，可能需要刷写新固件（较复杂，建议换模块试试）

### 7.6.3 GPS没有数据怎么办？

**问题现象**：屏幕上的经度和纬度一直显示0.000000，或者数值不更新。

**排查步骤**：

**Step 1：检查GPS天线**
- 确认GPS天线已正确接到GPS模块的天线接口
- 天线接头要拧紧
- 如果是陶瓷有源天线，确认天线完好无损

**Step 2：到室外测试**
- GPS信号在室内非常弱，尤其是高楼内
- 把设备拿到室外开阔地带测试
- 远离高楼、树木、金属物体

**Step 3：等待足够时间**
- 冷启动（长时间未使用）首次定位需要2-5分钟
- 热启动约10-30秒
- 耐心等待，可以观察串口输出的NMEA数据

**Step 4：检查GPS模块波特率**
- 用串口调试助手查看GPS模块输出的原始数据
- 连接GPS模块的TX引脚到USB-TTL的RXD
- 波特率通常为9600
- 应该能看到类似 `$GNRMC,123519,A,...` 的NMEA数据

**Step 5：检查代码中的串口配置**
- 确认代码中USART1的波特率与GPS模块一致（通常是9600）
- 检查GPS模块的RX是否接到了PA10（USART1_RX）

**Step 6：检查NMEA数据格式**
- 不同的GPS模块输出的NMEA语句前缀可能不同
- 常见前缀：`$GNRMC`、`$GPRMC`、`$BDRMC`
- 代码中使用了 `strstr(buf, "$GNRMC")` 来查找
- 如果你的模块输出的是 `$GPRMC`，需要修改代码

### 7.6.4 APP收不到消息怎么办？

**问题现象**：APP上经度和纬度不更新，或者报警信息收不到。

**排查步骤**：

**Step 1：检查MQTT连接**
- 打开APP，看是否显示"MQTT服务器连接成功"
- 如果显示连接失败，检查手机网络是否正常
- MQTT服务器地址 `tcp://47.109.89.8:1883` 是否能访问

**Step 2：检查主题是否正确**
- 确认APP中输入的Sub和Pub主题与开发板上显示的一致
- 按开发板上的 **KEY4** 查看主题参数
- Sub和Pub不能填反了！

**Step 3：检查设备端MQTT连接**
- 按开发板上的 **KEY3** 查看MQTT连接状态
- 确认WiFi已连接、MQTT已连接
- 如果MQTT未连接，可能是WiFi问题（参考7.6.2）

**Step 4：检查MQTT服务器**
- 确认MQTT服务器 `47.109.89.8:1883` 正常运行
- 可以用MQTT.fx或MQTT Explorer等工具测试服务器连通性

**Step 5：检查数据格式**
- 设备发送的数据是JSON格式
- 确保JSON格式正确，例如：
  ```json
  {"sensor1":113.562341,"sensor2":23.162451,"sensor3":"1"}
  ```
- 如果JSON格式错误，APP无法解析

**Step 6：防火墙/网络问题**
- 某些公共WiFi或公司网络可能屏蔽了1883端口
- 尝试用手机4G/5G数据流量测试
- 或者换一个有公网IP的MQTT服务器

**Step 7：查看APP日志**
- 在Android Studio中运行APP，查看Logcat日志
- 检查是否有连接异常、JSON解析异常等错误信息

---


---

# 附录

> 附录部分汇总了项目的完整信息，包括所有文件清单、硬件连接表、学习路线推荐和参考资料。你可以把这部分当作"速查手册"，需要时随时翻阅。

---

## 附录A：完整文件清单

本附录列出了项目的所有文件，按文件夹组织。总文件数量约40个，涵盖STM32固件代码、Android APP代码、文档资料三大类。

### A.1 STM32固件代码（单片机程序）

STM32固件代码是运行在STM32F103单片机上的程序，负责控制所有硬件模块、处理传感器数据、连接WiFi和MQTT服务器。

#### A.1.1 系统核心文件

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `main.c` | `User/` | **程序入口**，包含main函数，初始化所有外设，主循环 |
| `sys.h` | `System/` | 系统头文件，定义数据类型别名（u8, u16, u32等）、寄存器操作宏 |
| `sys.c` | `System/` | 系统时钟配置、GPIO操作函数、中断配置 |
| `delay.h` | `System/` | 延时函数头文件，声明毫秒/微秒延时函数 |
| `delay.c` | `System/` | SysTick延时实现，提供精确的delay_ms()和delay_us() |
| `stm32f10x_conf.h` | `User/` | 标准外设库配置文件，启用需要用到的外设模块 |
| `stm32f10x_it.c` | `User/` | **中断服务函数**，USART1/2接收中断、定时器中断 |

#### A.1.2 OLED显示驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `bsp_oled_iic.h` | `Hardware/` | OLED头文件，定义I2C引脚(PA11/PA12)、显示函数声明 |
| `bsp_oled_iic.c` | `Hardware/` | OLED驱动实现，I2C通信、SSD1306初始化、字符/汉字显示 |
| `oledFont.h` | `Hardware/` | **字库文件**，包含6x8/8x16 ASCII字体和16x16汉字字库 |

#### A.1.3 GPS模块驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `gps.h` | `Hardware/` | GPS头文件，NMEA数据结构体定义、函数声明 |
| `gps.c` | `Hardware/` | GPS驱动实现，NMEA协议解析（GNRMC语句）、经纬度转换 |

#### A.1.4 蜂鸣器驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `bsp_beep.h` | `Hardware/` | 蜂鸣器头文件，定义PA8引脚、控制宏 |
| `bsp_beep.c` | `Hardware/` | 蜂鸣器驱动，初始化、短响函数Short_Beep() |

#### A.1.5 LED驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `led.h` | `Hardware/` | LED头文件，引脚定义、亮灭宏、闪烁模式枚举 |
| `led.c` | `Hardware/` | LED驱动，初始化、闪烁控制（常亮/慢闪/快闪/熄灭） |

#### A.1.6 按键驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `key.h` | `Hardware/` | 按键头文件，4个按键的引脚定义、按键标志位声明 |
| `key.c` | `Hardware/` | 按键驱动，扫描检测、消抖处理、标志位置位 |

#### A.1.7 振动传感器驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `shake.h` | `Hardware/` | 振动传感器头文件，引脚定义、外部中断声明 |
| `shake.c` | `Hardware/` | 振动传感器驱动，EXTI中断、振动计数 |

#### A.1.8 串口通信驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `my_usart1.h` | `Hardware/` | USART1头文件，PA9/PA10引脚、GPS通信、波特率9600 |
| `my_usart1.c` | `Hardware/` | USART1驱动，GPS数据接收中断、printf打印 |
| `my_usart2.h` | `Hardware/` | USART2头文件，PA2/PA3引脚、WiFi通信、波特率115200 |
| `my_usart2.c` | `Hardware/` | USART2驱动，ESP8266数据收发、格式化输出 |

#### A.1.9 WiFi模块驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `esp8266_drv.h` | `Hardware/` | ESP8266头文件，WiFi参数、AT指令结构体、MQTT参数结构体 |
| `esp8266_drv.c` | `Hardware/` | ESP8266驱动，**核心状态机**，AT指令自动执行、WiFi连接管理 |

#### A.1.10 MQTT协议驱动

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `mqtt_drv.h` | `Hardware/` | MQTT头文件，CONNECT/PUBLISH/SUBSCRIBE报文函数声明 |
| `mqtt_drv.c` | `Hardware/` | MQTT协议实现，报文封装、数据发布、心跳维持 |

#### A.1.11 主菜单与业务逻辑

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `menu.h` | `User/` | 菜单系统头文件，模式标志位、函数声明 |
| `menu.c` | `User/` | **核心业务逻辑**，页面切换、防盗模式逻辑、自动控制、数据显示 |

### A.2 Android APP代码

Android APP安装在用户的手机上，通过MQTT协议与STM32单片机通信，实现远程监控。

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `MainActivity.java` | `app/src/main/java/cacom/example/smarthome/` | **APP核心代码**，界面显示、MQTT连接、数据解析、防盗控制 |
| `activity_main.xml` | `app/src/main/res/layout/` | 主界面布局，GPS卡片、防盗开关、状态显示区 |
| `dialog_login.xml` | `app/src/main/res/layout/` | 登录对话框布局，Sub/Pub主题输入框 |
| `strings.xml` | `app/src/main/res/values/` | 字符串资源，APP名称"汽车防盗监测" |
| `colors.xml` | `app/src/main/res/values/` | 颜色资源，黑白配色定义 |
| `AndroidManifest.xml` | `app/src/main/` | APP清单文件，权限声明（INTERNET、VIBRATE） |
| `build.gradle (project)` | `项目根目录/` | 项目级构建配置，Gradle插件版本 |
| `build.gradle (app)` | `app/` | APP级构建配置，依赖库（MQTT Paho、Material Design） |
| `settings.gradle` | `项目根目录/` | 项目设置，模块声明 |
| `gradle.properties` | `项目根目录/` | Gradle属性，AndroidX配置、JVM参数 |

### A.3 文档资料

| 文件名 | 路径 | 功能说明 |
|--------|------|---------|
| `README.md` | `项目根目录/` | 项目总说明文档 |
| `system_core_analysis.md` | `docs/` | 系统核心代码分析文档 |
| `peripheral_analysis.md` | `docs/` | 外设驱动代码分析文档 |
| `communication_analysis.md` | `docs/` | 通信模块代码分析文档 |
| `app_analysis.md` | `docs/` | Android APP代码分析文档 |
| `config_and_docs_analysis.md` | `docs/` | 配置文件与文档分析 |

### A.4 文件总览图

```
项目根目录/
│
├── 1-程序源文件_Stm32Code/          ← STM32固件代码
│   ├── SYS_main.c                    ← 主函数入口
│   ├── SYS_sys.c / SYS_sys.h         ← 系统核心
│   ├── SYS_menu.c / SYS_menu.h       ← 菜单与业务逻辑
│   ├── SYS_time2.c / SYS_time2.h     ← 定时器2
│   ├── SYS_usart1.c / SYS_usart1.h   ← 串口1驱动
│   ├── SYS_usart2.c / SYS_usart2.h   ← 串口2驱动
│   ├── SYS_bridgeflash.c / .h        ← Flash读写
│   ├── SYS_my_usart/
│   │   ├── my_usart1.c / my_usart1.h ← USART1驱动
│   │   └── my_usart2.c / my_usart2.h ← USART2驱动
│   └── ...
│
├── 2-APP源文件_AndroidApp/          ← Android APP代码
│   ├── app/src/main/java/.../MainActivity.java   ← APP主代码
│   ├── app/src/main/res/layout/activity_main.xml ← 主界面
│   ├── app/src/main/res/layout/dialog_login.xml  ← 登录对话框
│   ├── app/src/main/AndroidManifest.xml          ← APP清单
│   ├── app/build.gradle              ← APP构建配置
│   └── ...
│
├── 3-硬件设计文件/                  ← 原理图、PCB等
│   └── ...
│
└── 4-文档资料/                      ← 项目文档
    └── ...
```

---

## 附录B：硬件连接表

本附录详细列出STM32开发板与所有外部模块的引脚连接关系，包括接口类型、信号方向和电气参数。接线时请仔细核对本表，避免接错导致设备损坏。

### B.1 整体连接架构图

```
                    +---------------------+
                    |   STM32F103ZET6     |
                    |    单片机开发板      |
                    |                     |
      GPS模块 ←———| USART1 (PA9/PA10)  |
   (串口9600)       |                     |
                    | USART2 (PA2/PA3)  |———→ ESP8266 WiFi模块
                    |                     |      (串口115200)
     OLED显示屏 ←——| I2C (PA11/PA12)    |
     (I2C通信)      |                     |
                    | GPIO (PA8)         |———→ 蜂鸣器
                    |                     |
      4个按键  ———→| GPIO (PC0~PC3)     |
                    |                     |
   振动传感器  ———→| EXTI (PB0)         |
                    |                     |
       多个LED ←——| GPIO (PC4~PC7)     |
                    |                     |
                    | GPIO (PA1)         |———→ ESP8266复位
                    +---------------------+
```

### B.2 详细引脚连接表

#### B.2.1 OLED显示屏（SSD1306）

| OLED引脚 | STM32引脚 | 端口 | 信号方向 | 说明 |
|---------|-----------|------|---------|------|
| VCC | 3.3V | - | 电源输入 | 供电电压3.3V |
| GND | GND | - | 地线 | 电源地 |
| SCL | PA12 | GPIOA_Pin_12 | STM32→OLED | I2C时钟线 |
| SDA | PA11 | GPIOA_Pin_11 | 双向 | I2C数据线 |

**通信协议**：I2C（软件模拟）
**I2C设备地址**：0x3C（写地址0x78）
**分辨率**：128 x 64像素
**驱动芯片**：SSD1306

#### B.2.2 GPS模块

| GPS引脚 | STM32引脚 | 端口 | 信号方向 | 说明 |
|---------|-----------|------|---------|------|
| VCC | 3.3V/5V | - | 电源输入 | 根据模块要求供电 |
| GND | GND | - | 地线 | 电源地 |
| TX | PA10 | GPIOA_Pin_10 | GPS→STM32 | GPS数据输出 |
| RX | PA9 | GPIOA_Pin_9 | STM32→GPS | GPS指令输入（可选） |

**通信接口**：USART1
**波特率**：9600 bps
**数据格式**：8位数据位，1位停止位，无校验
**协议格式**：NMEA 0183（使用GNRMC语句）

#### B.2.3 ESP8266 WiFi模块

| ESP8266引脚 | STM32引脚 | 端口 | 信号方向 | 说明 |
|------------|-----------|------|---------|------|
| VCC | 3.3V | - | 电源输入 | **必须是3.3V！5V会烧毁！** |
| GND | GND | - | 地线 | 电源地 |
| TX | PA3 | GPIOA_Pin_3 | ESP8266→STM32 | WiFi数据输出 |
| RX | PA2 | GPIOA_Pin_2 | STM32→ESP8266 | WiFi指令输入 |
| RST | PA1 | GPIOA_Pin_1 | STM32→ESP8266 | 硬件复位控制（低电平复位） |
| CH_PD/EN | 3.3V | - | 电源输入 | 芯片使能，接3.3V常高 |

**通信接口**：USART2
**波特率**：115200 bps
**数据格式**：8位数据位，1位停止位，无校验
**控制方式**：AT指令集
**工作模式**：STA模式（Station，作为客户端连接WiFi路由器）

#### B.2.4 蜂鸣器

| 蜂鸣器引脚 | STM32引脚 | 端口 | 信号方向 | 说明 |
|-----------|-----------|------|---------|------|
| VCC | 3.3V | - | 电源输入 | 供电 |
| I/O | PA8 | GPIOA_Pin_8 | STM32→蜂鸣器 | 控制信号，高电平响 |
| GND | GND | - | 地线 | 电源地 |

**蜂鸣器类型**：有源蜂鸣器（通电即响，无需PWM驱动）
**控制方式**：GPIO推挽输出，高电平ON，低电平OFF

#### B.2.5 LED指示灯

| LED编号 | STM32引脚 | 端口 | 控制方式 | 功能说明 |
|---------|-----------|------|---------|---------|
| LED1 | PC4 | GPIOC_Pin_4 | GPIO输出 | 正常模式指示灯 |
| LED2 | PC5 | GPIOC_Pin_5 | GPIO输出 | 防盗模式指示灯 |
| LED3 | PC6 | GPIOC_Pin_6 | GPIO输出 | 报警状态指示灯（可闪烁） |
| LED4 | PC7 | GPIOC_Pin_7 | GPIO输出 | 保留/扩展用 |

**LED连接方式**：共阳极或共阴极（根据开发板设计）
**控制方式**：GPIO推挽输出
**闪烁模式**：支持常亮、慢速闪烁（约1Hz）、快速闪烁（约4Hz）、熄灭

#### B.2.6 按键

| 按键编号 | STM32引脚 | 端口 | 信号方向 | 功能说明 |
|---------|-----------|------|---------|---------|
| KEY1 | PC0 | GPIOC_Pin_0 | 按键→STM32 | 切换到数据显示页，关闭防盗/报警 |
| KEY2 | PC1 | GPIOC_Pin_1 | 按键→STM32 | 切换到防盗模式页，开启防盗监测 |
| KEY3 | PC2 | GPIOC_Pin_2 | 按键→STM32 | 切换到MQTT连接状态页 |
| KEY4 | PC3 | GPIOC_Pin_3 | 按键→STM32 | 切换到主题参数页 |

**按键检测方式**：轮询扫描（或外部中断）
**消抖方式**：软件延时消抖（约10-20ms）
**输入模式**：上拉输入（按键按下为低电平）

#### B.2.7 振动传感器（SW-420）

| 传感器引脚 | STM32引脚 | 端口 | 信号方向 | 说明 |
|-----------|-----------|------|---------|------|
| VCC | 3.3V/5V | - | 电源输入 | 供电 |
| GND | GND | - | 地线 | 电源地 |
| DO | PB0 | GPIOB_Pin_0 | 传感器→STM32 | 数字输出，振动检测信号 |

**检测方式**：外部中断（EXTI Line0）
**触发方式**：下降沿触发（振动时产生低电平脉冲）
**灵敏度**：通过传感器上的蓝色电位器调节

#### B.2.8 其他引脚

| 功能 | STM32引脚 | 端口 | 说明 |
|------|-----------|------|------|
| 系统复位按键 | NRST | - | 开发板上的RST按键，低电平复位 |
| 调试接口 SWDIO | PA13 | - | ST-Link调试数据线 |
| 调试接口 SWCLK | PA14 | - | ST-Link调试时钟线 |

### B.3 引脚使用总览表

以下表格汇总了STM32所有被使用的引脚，方便快速查找：

| 引脚编号 | 端口 | 复用功能 | 连接设备 | 信号类型 |
|---------|------|---------|---------|---------|
| PA0 | GPIOA_Pin_0 | - | 保留/ADC | 模拟输入 |
| PA1 | GPIOA_Pin_1 | - | ESP8266 RST | 数字输出 |
| PA2 | GPIOA_Pin_2 | USART2_TX | ESP8266 RX | 复用推挽输出 |
| PA3 | GPIOA_Pin_3 | USART2_RX | ESP8266 TX | 浮空输入 |
| PA8 | GPIOA_Pin_8 | - | 蜂鸣器 | 推挽输出 |
| PA9 | GPIOA_Pin_9 | USART1_TX | GPS RX | 复用推挽输出 |
| PA10 | GPIOA_Pin_10 | USART1_RX | GPS TX | 浮空输入 |
| PA11 | GPIOA_Pin_11 | USB_DM/I2C_SDA | OLED SDA | 推挽输出（软件I2C） |
| PA12 | GPIOA_Pin_12 | USB_DP/I2C_SCL | OLED SCL | 推挽输出（软件I2C） |
| PA13 | GPIOA_Pin_13 | SWDIO | ST-Link | 调试接口 |
| PA14 | GPIOA_Pin_14 | SWCLK | ST-Link | 调试接口 |
| PB0 | GPIOB_Pin_0 | EXTI0 | 振动传感器DO | 外部中断输入 |
| PC0 | GPIOC_Pin_0 | - | KEY1 | 上拉输入 |
| PC1 | GPIOC_Pin_1 | - | KEY2 | 上拉输入 |
| PC2 | GPIOC_Pin_2 | - | KEY3 | 上拉输入 |
| PC3 | GPIOC_Pin_3 | - | KEY4 | 上拉输入 |
| PC4 | GPIOC_Pin_4 | - | LED1 | 推挽输出 |
| PC5 | GPIOC_Pin_5 | - | LED2 | 推挽输出 |
| PC6 | GPIOC_Pin_6 | - | LED3 | 推挽输出 |
| PC7 | GPIOC_Pin_7 | - | LED4 | 推挽输出 |

### B.4 接线检查清单

首次接线或排查问题时，按以下清单逐项检查：

**电源检查：**
- [ ] STM32开发板供电正常（USB或外部5V）
- [ ] 所有模块的VCC和GND已正确连接
- [ ] ESP8266的供电是3.3V（不是5V！）

**信号线检查：**
- [ ] OLED的SCL接PA12，SDA接PA11
- [ ] GPS的TX接PA10，RX接PA9
- [ ] ESP8266的TX接PA3，RX接PA2（交叉连接！）
- [ ] 蜂鸣器信号接PA8
- [ ] 4个按键分别接PC0~PC3
- [ ] 振动传感器DO接PB0
- [ ] LED分别接PC4~PC7
- [ ] ESP8266的RST接PA1

**特别注意（易错点）：**
- **串口交叉连接**：STM32的TX要接设备的RX，STM32的RX要接设备的TX！
- **ESP8266必须用3.3V供电**：接5V会永久损坏模块！
- **GPS天线必须接好**：没有天线GPS无法定位
- **I2C需要上拉电阻**：如果OLED不亮，检查是否有4.7KΩ上拉电阻（有些模块已集成）

---

## 附录C：学习路线推荐

本附录为想要深入学习嵌入式开发的读者提供系统化的学习路线，从C语言基础到STM32入门，再到项目实战，每个阶段都推荐了学习资源和练习目标。

### C.1 整体学习路线图

```
阶段一: C语言基础          阶段二: STM32入门           阶段三: 项目实战
(2~4周)                   (4~8周)                    (4~8周)
    |                         |                          |
    ▼                         ▼                          ▼
+---------+             +-----------+              +-------------+
| 语法基础 |             | GPIO控制   |              | 系统架构设计 |
| 数据类型 |     →      | 时钟配置   |      →      | 模块化编程   |
| 流程控制 |             | 中断与定时器|              | 通信协议实现 |
| 函数指针 |             | 串口通信   |              | 物联网应用   |
| 结构体  |             | ADC/DMA   |              | APP开发    |
+---------+             +-----------+              +-------------+
    |                         |                          |
    ▼                         ▼                          ▼
推荐资源:                 推荐资源:                   推荐资源:
- 翁凯C语言               - 正点原子教程               - 本项目代码
- C Primer Plus           - 野火STM32教程             - GitHub开源项目
- LeetCode简单题          - 官方参考手册               - 技术博客
```

### C.2 阶段一：C语言基础（2~4周）

**目标**：掌握C语言核心语法，能够独立编写简单的控制台程序。

#### C.2.1 学习内容

| 主题 | 知识点 | 重要程度 |
|------|--------|---------|
| **基本语法** | 变量、数据类型、运算符、表达式 | ⭐⭐⭐⭐⭐ |
| **流程控制** | if/else、switch、for、while、break/continue | ⭐⭐⭐⭐⭐ |
| **数组与字符串** | 一维/二维数组、字符串操作函数 | ⭐⭐⭐⭐⭐ |
| **函数** | 函数定义与调用、参数传递、返回值、递归 | ⭐⭐⭐⭐⭐ |
| **指针** | 指针概念、指针运算、指针与数组、函数指针 | ⭐⭐⭐⭐⭐ |
| **结构体** | struct定义、typedef、结构体指针、嵌套 | ⭐⭐⭐⭐ |
| **文件操作** | fopen/fclose、fread/fwrite、fscanf/fprintf | ⭐⭐⭐ |
| **预处理器** | 宏定义、条件编译、头文件包含 | ⭐⭐⭐⭐ |
| **内存管理** | malloc/free、堆栈概念、内存泄漏 | ⭐⭐⭐⭐ |

#### C.2.2 推荐学习资源

**视频教程**：
- **翁凯C语言入门**（浙江大学）
  - 平台：中国大学MOOC（慕课）/ B站
  - 链接：https://www.icourse163.org/course/ZJU-199001
  - 特点：讲解清晰，适合零基础，有大量编程练习
  - 推荐理由：国内最受欢迎的C语言入门课程

- **郝斌C语言自学教程**
  - 平台：B站
  - 搜索关键词："郝斌C语言"
  - 特点：讲解细致，语速适中，适合自学

**书籍**：
- **《C Primer Plus（第6版）》**（Stephen Prata 著）
  - 适合人群：零基础入门
  - 特点：讲解非常详细，每个概念都有示例代码
  - 学习建议：前10章必须精读，后面的章节可作为参考书

- **《C程序设计语言（第2版）》**（K&R，Brian Kernighan & Dennis Ritchie 著）
  - 适合人群：有一定基础后阅读
  - 特点：C语言之父写的权威教材，精炼深刻
  - 推荐理由：经典中的经典，值得反复阅读

**在线练习平台**：
- **LeetCode**（https://leetcode.cn）
  - 建议完成"简单"难度的数组、字符串、指针相关题目20道
- **洛谷**（https://www.luogu.com.cn）
  - 适合新手的在线评测系统，题目分类清晰

#### C.2.3 阶段一练习目标

完成以下练习，检验C语言学习成果：

1. **计算器程序**：实现加减乘除四则运算
2. **学生管理系统**：使用结构体数组管理学生信息（增删改查）
3. **字符串处理函数**：自己实现strlen、strcpy、strcmp等函数
4. **链表操作**：实现单链表的创建、插入、删除、遍历
5. **文件读写**：将学生信息保存到文件，从文件读取

### C.3 阶段二：STM32入门（4~8周）

**目标**：理解单片机工作原理，掌握STM32的基本外设使用，能够独立完成简单的硬件控制程序。

#### C.3.1 学习内容

| 主题 | 知识点 | 重要程度 |
|------|--------|---------|
| **单片机基础** | 什么是单片机、STM32系列介绍、开发环境搭建 | ⭐⭐⭐⭐⭐ |
| **GPIO操作** | 输入/输出模式、推挽/开漏、上拉/下拉、寄存器操作 | ⭐⭐⭐⭐⭐ |
| **时钟系统** | 系统时钟、AHB/APB总线、时钟树配置 | ⭐⭐⭐⭐ |
| **中断系统** | NVIC、外部中断、中断优先级、中断服务函数 | ⭐⭐⭐⭐⭐ |
| **定时器** | 定时/计数模式、PWM输出、输入捕获 | ⭐⭐⭐⭐ |
| **串口通信** | USART配置、波特率、中断接收、printf重定向 | ⭐⭐⭐⭐⭐ |
| **I2C/SPI通信** | 软件模拟I2C、硬件I2C、SPI通信 | ⭐⭐⭐⭐ |
| **ADC转换** | 模数转换、多通道采样、DMA传输 | ⭐⭐⭐ |
| **看门狗** | 独立看门狗、窗口看门狗 | ⭐⭐⭐ |

#### C.3.2 推荐学习资源

**视频教程**：
- **正点原子STM32F103入门教程**
  - 平台：B站 / 正点原子官网
  - 链接：http://www.alientek.com（官网）
  - 特点：国内最火的STM32教程，资料全面，配套开发板
  - 推荐理由：从零开始教，每个外设有详细的代码讲解和实验演示

- **野火STM32F103教程**
  - 平台：B站 / 野火官网
  - 链接：https://www.firebbs.cn
  - 特点：教程详尽，原理讲解深入，适合想深入理解的读者
  - 推荐理由：除了教你怎么做，还告诉你为什么这么做

**书籍**：
- **《STM32库开发实战指南》**（刘火良 杨森 著）
  - 适合人群：有C语言基础，想快速上手STM32
  - 特点：基于标准库，从GPIO到FreeRTOS，循序渐进
  - 推荐理由：国内STM32领域的经典教材

- **《STM32F10xxx参考手册》**（ST官方）
  - 适合人群：需要深入了解某个外设时查阅
  - 特点：官方权威文档，详细描述了每个寄存器的功能
  - 获取方式：ST官网搜索 "STM32F103 reference manual"

**在线资源**：
- **ST中文论坛**：https://www.stmcu.com.cn
  - ST官方中文社区，有问题可以在这里提问
- **正点原子论坛**：http://www.openedv.com
  - 国内最大的STM32学习者社区

#### C.3.3 阶段二练习项目

完成以下项目，检验STM32学习成果：

1. **LED流水灯**：控制4个LED依次亮灭，学习GPIO输出
2. **按键控制LED**：按下按键改变LED状态，学习GPIO输入和中断
3. **串口回显**：电脑发送什么，单片机就回传什么，学习USART通信
4. **定时器闪烁**：用定时器中断实现LED每1秒闪烁，学习定时器
5. **PWM调光**：用PWM调节LED亮度，学习PWM输出
6. **OLED显示**：在OLED屏幕上显示文字和数字，学习I2C通信
7. **综合项目**：做一个数字时钟，OLED显示时间，按键设置闹钟

### C.4 阶段三：项目实战（4~8周）

**目标**：综合运用所学知识，完成一个完整的物联网项目，包括硬件设计、固件开发、上位机/APP开发。

#### C.4.1 推荐实战项目

**初级项目**：

| 项目名称 | 涉及技术 | 难度 |
|---------|---------|------|
| **温湿度监测系统** | DHT11/DHT22传感器、OLED显示、串口输出 | ⭐⭐ |
| **智能台灯** | PWM调光、光敏传感器、按键控制 | ⭐⭐ |
| **简易计算器** | 4x4矩阵键盘、OLED显示、数学运算 | ⭐⭐⭐ |

**中级项目**：

| 项目名称 | 涉及技术 | 难度 |
|---------|---------|------|
| **智能家居控制器** | WiFi通信、MQTT协议、继电器控制、手机APP | ⭐⭐⭐⭐ |
| **GPS定位器** | GPS模块、OLED显示、数据记录到Flash | ⭐⭐⭐⭐ |
| **环境监测站** | 多传感器（温湿度/PM2.5/光照）、WiFi上传、云端显示 | ⭐⭐⭐⭐ |

**高级项目**：

| 项目名称 | 涉及技术 | 难度 |
|---------|---------|------|
| **本项目：汽车防盗系统** | GPS、振动传感器、WiFi、MQTT、Android APP | ⭐⭐⭐⭐⭐ |
| **智能小车** | 电机驱动、循迹/避障、PID控制、蓝牙遥控 | ⭐⭐⭐⭐⭐ |
| **无人机飞控** | MPU6050姿态传感器、PID算法、PWM电机控制、无线通信 | ⭐⭐⭐⭐⭐ |

#### C.4.2 实战学习建议

1. **先读懂别人的代码**
   - 从GitHub或Gitee上找开源的STM32项目
   - 推荐搜索关键词：`STM32`、`IoT`、`MQTT`、`GPS Tracker`
   - 理解代码结构、模块划分、通信协议

2. **在此基础上修改**
   - 添加一个新功能（如增加一个传感器）
   - 优化现有功能（如改进报警算法）
   - 换一种通信方式（如把WiFi换成蓝牙）

3. **最终独立完成自己的项目**
   - 从需求分析开始
   - 自己设计硬件连接
   - 自己编写全部代码
   - 调试、优化、完善

#### C.4.3 推荐实践平台

- **GitHub**（https://github.com）
  - 搜索 `STM32`、`embedded`、`IoT` 等关键词
  - Star数量多的项目通常质量较高
  - 推荐仓库：STM32CubeProjects、Awesome-Embedded

- **Gitee**（https://gitee.com）
  - 国内访问速度快
  - 搜索 `STM32` 有大量中文项目

### C.5 学习资源汇总表

| 阶段 | 类型 | 名称 | 链接/获取方式 |
|------|------|------|-------------|
| C语言 | 视频 | 翁凯C语言 | https://www.icourse163.org/course/ZJU-199001 |
| C语言 | 视频 | 郝斌C语言 | B站搜索"郝斌C语言" |
| C语言 | 书籍 | C Primer Plus | 各大电商平台购买 |
| C语言 | 书籍 | C程序设计语言(K&R) | 各大电商平台购买 |
| C语言 | 练习 | LeetCode | https://leetcode.cn |
| C语言 | 练习 | 洛谷 | https://www.luogu.com.cn |
| STM32 | 视频 | 正点原子教程 | http://www.alientek.com |
| STM32 | 视频 | 野火教程 | https://www.firebbs.cn |
| STM32 | 书籍 | STM32库开发实战指南 | 各大电商平台购买 |
| STM32 | 文档 | 官方参考手册 | ST官网搜索下载 |
| STM32 | 社区 | ST中文论坛 | https://www.stmcu.com.cn |
| 实战 | 代码 | GitHub开源项目 | https://github.com |
| 实战 | 代码 | Gitee开源项目 | https://gitee.com |

---

## 附录D：参考资料

本附录汇总了项目开发过程中用到的所有参考资料，包括官方文档、视频教程、推荐书籍和在线资源。

### D.1 官方文档

#### D.1.1 STM32官方资料

| 文档名称 | 说明 | 获取链接 |
|---------|------|---------|
| **STM32F103xE Reference Manual (RM0008)** | STM32F103系列的完整参考手册，包含所有外设的寄存器描述 | https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf |
| **STM32F103ZE Datasheet** | STM32F103ZE芯片的数据手册，包含引脚定义、电气参数 | https://www.st.com/resource/en/datasheet/stm32f103ze.pdf |
| **Cortex-M3 Technical Reference Manual** | ARM Cortex-M3内核的技术参考手册 | https://developer.arm.com/documentation/ddi0337/latest |
| **STM32 Standard Peripheral Library** | ST官方标准外设库下载（V3.5.0） | ST官网搜索 "STSW-STM32054" |

#### D.1.2 ESP8266资料

| 文档名称 | 说明 | 获取方式 |
|---------|------|---------|
| **ESP8266 AT指令集** | 乐鑫官方AT指令参考手册 | 乐鑫官网：https://docs.espressif.com/projects/esp-at/en/latest/AT_Command_Set/index.html |
| **ESP8266 技术规格书** | 硬件规格、电气参数 | 乐鑫官网下载 |
| **ESP-01S模块引脚说明** | 常见ESP8266模块的引脚定义 | 淘宝/天猫购买页面通常附带 |

#### D.1.3 GPS模块资料

| 文档名称 | 说明 | 获取方式 |
|---------|------|---------|
| **NMEA 0183协议标准** | GPS数据输出格式标准 | 搜索 "NMEA 0183 Protocol Specification" |
| **GNRMC语句格式说明** | 推荐最小定位信息语句的字段含义 | 本项目第三章有详细说明 |

#### D.1.4 MQTT协议资料

| 文档名称 | 说明 | 获取链接 |
|---------|------|---------|
| **MQTT Version 3.1.1 Specification** | MQTT协议官方规范（OASIS标准） | http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html |
| **MQTT中文版介绍** | MQTT协议中文入门介绍 | https://mqtt.org/zh-chs/getting-started/ |

### D.2 视频教程

#### D.2.1 STM32入门视频

| 教程名称 | 平台 | 说明 |
|---------|------|------|
| **正点原子STM32F103入门视频** | B站 / 官网 | 最经典的STM32中文视频教程，适合零基础 |
| **野火STM32入门视频** | B站 / 官网 | 讲解深入，原理分析透彻 |
| **江科大STM32教程** | B站 | 近年较新的教程，使用HAL库 |

#### D.2.2 本项目配套视频

| 视频名称 | 链接 | 说明 |
|---------|------|------|
| **WiFi连接教程** | https://www.bilibili.com/video/BV1kh8LzGEdn | 教你怎么配置WiFi热点让开发板连接 |
| **蓝牙连接教程** | https://www.bilibili.com/video/BV1dA8LzhE2f | 教你怎么通过蓝牙连接设备 |

#### D.2.3 Android开发视频

| 教程名称 | 平台 | 说明 |
|---------|------|------|
| **Android开发基础** | B站搜索 | 学习Android APP开发的基础知识 |
| **MQTT Android开发** | B站搜索 | 学习如何在Android中使用MQTT |

### D.3 推荐书籍

#### D.3.1 C语言与嵌入式基础

| 书名 | 作者 | 出版社 | 推荐理由 |
|------|------|--------|---------|
| **《C Primer Plus（第6版）》** | Stephen Prata | 人民邮电出版社 | 零基础入门最佳选择，讲解细致 |
| **《C程序设计语言（第2版）》** | Kernighan & Ritchie | 机械工业出版社 | C语言经典权威教材 |
| **《C和指针》** | Kenneth Reek | 人民邮电出版社 | 深入理解指针，嵌入式开发必备 |
| **《嵌入式C语言自我修养》** | 王利涛 | 电子工业出版社 | 从C语言到嵌入式开发的过渡好书 |

#### D.3.2 STM32开发

| 书名 | 作者 | 出版社 | 推荐理由 |
|------|------|--------|---------|
| **《STM32库开发实战指南》** | 刘火良、杨森 | 机械工业出版社 | 国内STM32开发经典教材 |
| **《STM32F10xxx参考手册》** | ST官方 | ST官网下载 | 开发必备权威参考 |
| **《精通STM32F4》** | 刘火涛 | 人民邮电出版社 | 进阶学习STM32F4系列 |
| **《ARM Cortex-M3权威指南》** | Joseph Yiu | 北京航空航天大学出版社 | 深入理解Cortex-M3内核 |

#### D.3.3 物联网与通信

| 书名 | 作者 | 出版社 | 推荐理由 |
|------|------|--------|---------|
| **《物联网技术导论》** |  various | various | 了解物联网整体架构 |
| **《MQTT协议快速入门》** | various | various | 学习MQTT通信协议 |

#### D.3.4 Android开发

| 书名 | 作者 | 出版社 | 推荐理由 |
|------|------|--------|---------|
| **《第一行代码——Android》** | 郭霖 | 人民邮电出版社 | 国内最好的Android入门书 |
| **《Android开发艺术探索》** | 任玉刚 | 电子工业出版社 | Android进阶提升 |

### D.4 在线资源与社区

#### D.4.1 技术社区

| 社区名称 | 网址 | 用途 |
|---------|------|------|
| **ST中文论坛** | https://www.stmcu.com.cn | ST官方中文社区，提问和下载资料 |
| **正点原子论坛** | http://www.openedv.com | 国内最大的STM32学习者社区 |
| **野火论坛** | https://www.firebbs.cn | 野火官方社区 |
| **21ic论坛** | https://bbs.21ic.com | 综合电子工程师论坛 |
| **电子发烧友** | https://bbs.elecfans.com | 电子技术综合论坛 |
| **知乎STM32话题** | https://www.zhihu.com/topic/19565838 | 阅读高质量回答 |
| **CSDN** | https://www.csdn.net | 中文技术博客平台 |
| **掘金** | https://juejin.cn | 技术文章分享平台 |

#### D.4.2 在线工具

| 工具名称 | 网址 | 用途 |
|---------|------|------|
| **MQTT在线测试工具** | http://www.hivemq.com/demos/websocket-client/ | 在线测试MQTT通信 |
| **JSON在线解析** | https://www.json.cn/ | 验证和格式化JSON数据 |
| **在线进制转换** | 搜索引擎搜索"进制转换" | 方便的进制转换计算 |
| **Keil官方下载** | https://www.keil.com/download | 下载Keil MDK |
| **Android Studio** | https://developer.android.com/studio | 下载Android开发IDE |

#### D.4.3 元器件采购

| 平台 | 网址 | 用途 |
|------|------|------|
| **立创商城** | https://www.szlcsc.com | 电子元器件采购 |
| **淘宝** | https://www.taobao.com | 开发板、模块购买 |
| **天猫** | https://www.tmall.com | 品牌开发板购买 |
| **嘉立创** | https://www.jlc.com | PCB打样和元器件 |

### D.5 快速参考卡片

#### D.5.1 常用缩写对照表

| 缩写 | 全称 | 中文含义 |
|------|------|---------|
| **MCU** | Microcontroller Unit | 微控制器/单片机 |
| **GPIO** | General Purpose Input/Output | 通用输入输出 |
| **USART** | Universal Synchronous Asynchronous Receiver Transmitter | 通用同步异步收发器 |
| **I2C** | Inter-Integrated Circuit | 集成电路互连总线 |
| **SPI** | Serial Peripheral Interface | 串行外设接口 |
| **PWM** | Pulse Width Modulation | 脉冲宽度调制 |
| **ADC** | Analog-to-Digital Converter | 模数转换器 |
| **DMA** | Direct Memory Access | 直接存储器访问 |
| **NVIC** | Nested Vectored Interrupt Controller | 嵌套向量中断控制器 |
| **EXTI** | External Interrupt | 外部中断 |
| **RTC** | Real-Time Clock | 实时时钟 |
| **FLASH** | Flash Memory | 闪存 |
| **MQTT** | Message Queuing Telemetry Transport | 消息队列遥测传输 |
| **TCP** | Transmission Control Protocol | 传输控制协议 |
| **IP** | Internet Protocol | 互联网协议 |
| **WiFi** | Wireless Fidelity | 无线保真 |
| **SSID** | Service Set Identifier | 服务集标识符（WiFi名称）|
| **NMEA** | National Marine Electronics Association | 美国国家海洋电子协会（GPS协议）|
| **OLED** | Organic Light-Emitting Diode | 有机发光二极管 |
| **AP** | Access Point | 无线接入点 |
| **STA** | Station | 站点（WiFi客户端模式）|
| **JSON** | JavaScript Object Notation | JavaScript对象表示法 |
| **APK** | Android Package | Android安装包 |
| **IDE** | Integrated Development Environment | 集成开发环境 |
| **ISR** | Interrupt Service Routine | 中断服务程序 |
| **FIFO** | First In First Out | 先进先出 |
| **LIFO** | Last In First Out | 后进先出 |
| **LSB** | Least Significant Bit | 最低有效位 |
| **MSB** | Most Significant Bit | 最高有效位 |
| **CRC** | Cyclic Redundancy Check | 循环冗余校验 |

#### D.5.2 常用数值对照

| 数值 | 说明 |
|------|------|
| 72 MHz | STM32F103主频 |
| 9600 bps | GPS模块波特率 |
| 115200 bps | ESP8266 WiFi模块波特率 |
| 3.3V | STM32和大部分模块的工作电压 |
| 5V | 开发板供电电压 |
| 128 x 64 | OLED屏幕分辨率（像素）|
| 0x3C | OLED的I2C设备地址 |
| 1883 | MQTT默认TCP端口号 |
| 400 bytes | USART1接收缓冲区大小 |
| 1024 bytes | USART2接收缓冲区大小 |
| 2秒 | 防盗模式振动判断间隔 |
| 10秒 | 防盗模式报警持续时间 |
| 3次 | 触发非法入侵报警的振动次数阈值 |

---

> **至此，整本说明书已经全部完成。** 感谢你的耐心阅读！希望这本书能帮助你顺利完成汽车防盗系统的学习和实践。如果在使用过程中遇到任何问题，欢迎回到第七章的"常见问题解答"部分查找解决方案，或者参考附录中的学习资源继续深入探索。祝学习愉快！

