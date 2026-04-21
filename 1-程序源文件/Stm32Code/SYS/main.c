#include "sys.h"

SENSOR SensorData;   // 传感器结构体定义
SYSTEM System;       // 系统标志位结构体定义

int main(void)
{
    delay_init();		// 延时函数初始化
	  BdgFlashInit();		// flash初始化包含阈值参数初始化与开关
    NVIC_Config();		// 中断优先级配置  
    KEY_Init();			// 按键初始化
    Beep_Init();		// 蜂鸣器初始化
    oled_Init();		// oled初始化
    LED_GPIO_Config();	// LED灯初始化
    Shake_Init();		// 震动初始化
    My_USART1();		// Usart1初始化，用于gps
    My_USART2(); 		// Usart2初始化，与ESP8266通信用，波特率：115200
    WIFI_RESET_init();	// ESP8266复位引脚初始化
    Ali_MsessageInit();	// 服务器连接相关的变量初始化
    TIM2_Init(499, 7199); // 定时器2初始化，定时扫描按键，获取震动状态并计数
    oled_Clear();

    /*******************************************/
    while (1) {
        ESP8266_run_handle(); // 连接MQTT服务器并与之通信
        // 获取并传递经纬度数据
        get_gps();
        SensorData.longitudeVal = GPS_Data.longitude;
        SensorData.latitudeVal = GPS_Data.latitude;
        Mode_selection(); // 模式判定 （按键1选择模式）
       
    }
}
