/*  *******************************************************************
//  文件说明：FLASH存储管理头文件，定义存储参数及配置结构
*******************************************************************  */
#ifndef __BRIDGE_FLASH_H_
#define __BRIDGE_FLASH_H_
#include "sys.h"
/*-- FLASH物理参数定义 --------------------------------------------*/
#define FLASH_PAGE_SIZE 0x400U     // 单页容量1KB (1024字节)
#define FLASH_ADDR_BASE 0x08000000 // MCU FLASH起始地址(STM32F1系列)

/*-- 自定义存储区域划分 --------------------------------------------*/
#define FLASH_ADDR_START (FLASH_ADDR_BASE + 58 * FLASH_PAGE_SIZE) // 自定义存储区起始地址(58页)
#define FLASH_ADDR_END (FLASH_ADDR_BASE + 64 * FLASH_PAGE_SIZE)   // 自定义存储区结束地址(64页)

/*-- 存储操作参数 -------------------------------------------------*/
#define SIZE_OF_HALF_WORD 2                             // 半字长度(字节)
#define UINT_OF_WRITE SIZE_OF_HALF_WORD                 // 写入操作最小单位
#define PAGE_BUF_SIZE (FLASH_PAGE_SIZE / UINT_OF_WRITE) // 页缓存数组大小

/*-- STM32标准参数定义 --------------------------------------------*/
#define STM_SECTOR_SIZE 1024      // Flash扇区标准尺寸
#define STM_FLASH_BASE 0x08000000 // 标准Flash基地址
#define STM_FLASH_SIZE 64         // Flash总容量64KB

/*-- 实际使用地址定义 ----------------------------------------------*/
#define FLASH_START_ADD 0x0800F000 // 配置数据存储起始地址
                                   // 地址范围：0x0800F000-0x0800FFFF (4KB空间)
                                   // 结构体占用：500字节(config_buff)+2字节标志+10字节保留=512字节

/*-- 配置状态标志 -------------------------------------------------*/
#define FLASH_FLAG_READY 0X5555 // 魔数标识，表示已完成初始配置

/*-- 压缩存储结构体定义 --------------------------------------------*/
#pragma pack(1) // 单字节对齐，确保结构体紧凑存储
typedef struct
{
    unsigned short is_config;       // 配置状态标志(2字节)
    unsigned char config_buff[500]; // 核心配置数据存储区(500字节)
    unsigned char reservl[10];      // 保留字段，用于后续扩展或对齐(10字节)
} STR_CONFIG_DATA;
#pragma pack(4) // 恢复默认4字节对齐

/*-- 全局配置实例声明 ----------------------------------------------*/
extern STR_CONFIG_DATA strConfig_info; // 系统配置数据实例

/*-- 函数声明 -----------------------------------------------------*/
void BdgFlashInit(void);   // Flash存储系统初始化
void write_config(void);   // 写入配置到FLASH
void read_config(void);    // 从FLASH读取配置
void save_Threshold(void); // 阈值变更保存(差异写入)
void Threshold_Init(void);  // 阈值系统初始化
#endif
