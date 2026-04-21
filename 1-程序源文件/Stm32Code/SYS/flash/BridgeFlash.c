/*  *******************************************************************
//  文件说明：FLASH存储管理模块，用于系统配置参数的存储与读取
*******************************************************************  */
#include "sys.h"

STR_CONFIG_DATA strConfig_info; // 系统配置信息结构体实例

/*  *****************************************************
//  函数名称：BdgFlashInit
//  功能描述：初始化Flash存储系统
*****************************************************  */
void BdgFlashInit(void)
{
    FLASH_Unlock();  // 解锁FLASH控制器
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // 清除状态标志
    FLASH_Lock();    // 重新上锁FLASH
    read_config();    // 从FLASH读取配置信息
	  Mqtt_Parameter_init();  // 初始化MQTT参数
    Threshold_Init(); // 初始化传感器阈值
}

/*  *****************************************************
//  函数名称：FLASH_ErasePage_user
//  功能描述：擦除指定FLASH页
//  输入参数：page_address - 需要擦除的页地址
//  返回参数：1-成功 0-失败
//  注意事项：擦除前会自动解锁，操作完成后重新上锁
*****************************************************  */
unsigned char FLASH_ErasePage_user(uint32_t page_address) 
{
    FLASH_Status status;
    FLASH_Unlock(); // 解锁FLASH
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    
    status = FLASH_ErasePage(page_address); // 执行页擦除
    if(status != FLASH_COMPLETE) {
        FLASH_Lock();
        return 0;
    }
    
    FLASH_Lock(); // 重新上锁
    return 1;
}

/*  *****************************************************
//  函数名称：FLASH_WriteData
//  功能描述：向FLASH写入数据
//  输入参数：
//  address - 起始地址（必须为偶数）
//  data - 要写入的数据数组
//  length - 数据长度（以16位为单位）
//  返回参数：1-成功 0-失败
//  注意事项：写入前会自动擦除目标页
*****************************************************  */
unsigned char FLASH_WriteData(uint32_t address, uint16_t *data, uint32_t length) 
{
    FLASH_Status status;
    uint32_t i;
    
    if(address % 2 != 0) return 0; // 检查地址对齐
    
    FLASH_ErasePage_user(address); // 先擦除目标页
    
    FLASH_Unlock();
    for(i = 0; i < length; i++) {
        status = FLASH_ProgramHalfWord(address + i*2, data[i]); // 以半字为单位写入
        if(status != FLASH_COMPLETE) {
            FLASH_Lock();
            return 0;
        }
    }
    FLASH_Lock();
    return 1;
}

/*  *****************************************************
//  函数名称：FLASH_ReadData
//  功能描述：从FLASH读取数据
//  输入参数：
//  address - 起始地址
//  data - 数据存储缓冲区
//  length - 要读取的数据长度（以16位为单位）
*****************************************************  */
void FLASH_ReadData(uint32_t address, uint16_t *data, uint32_t length) 
{
    for(uint16_t i = 0; i < length; i++) {
        data[i] = *((uint16_t*)(address + i*2)); // 直接内存访问读取半字数据
    }
}

unsigned short test_flashdata[10]={0x01,0x02,3,4,5,6,7,8,9,10}; // FLASH测试数据

/*  *****************************************************
//  函数名称：write_config
//  功能描述：将配置结构体写入FLASH
//  注意事项：使用半字写入方式，结构体大小需为偶数
*****************************************************  */
void write_config(void)
{
    FLASH_WriteData(FLASH_START_ADD, (unsigned short *)&strConfig_info, sizeof(STR_CONFIG_DATA)/2);
}

/*  *****************************************************
//  函数名称：read_config
//  功能描述：从FLASH读取配置到结构体
//  注意事项：结构体定义需与存储格式一致
*****************************************************  */
void read_config(void)
{
    FLASH_ReadData(FLASH_START_ADD, (unsigned short *)&strConfig_info, sizeof(STR_CONFIG_DATA)/2);
}

/*  *****************************************************
//  函数名称：save_Threshold
//  功能描述：比较并保存当前阈值到FLASH
//  实现逻辑：
//  1. 逐字节比较当前阈值与存储的配置
//  2. 发现差异时执行全量写入
//  优点：减少不必要的FLASH写入操作
*****************************************************  */
void save_Threshold(void)
{
	
}

/*  *****************************************************
//  函数名称：Threshold_Init
//  功能描述：传感器阈值系统初始化
//  初始化策略：
//  1. 首次使用时设置默认值并保存到FLASH
//  2. 后续启动时从FLASH读取配置
*****************************************************  */
void Threshold_Init(void)
{
	// 检查是否首次配置
	if(strConfig_info.is_config != FLASH_FLAG_READY) {
		strConfig_info.is_config = FLASH_FLAG_READY;
		// 标记已配置并保存
		memcpy((unsigned char *)&strConfig_info.config_buff[100],
			strMqtt_Inof.device_name, 100);
		write_config();		
	} else {
		// 从FLASH加载配置
		memcpy(strMqtt_Inof.device_name,
			(unsigned char *)&strConfig_info.config_buff[100],
			100);
	}
}
