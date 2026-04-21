#include "sys.h"

//模式切换标志位
uint8_t mode_selection = 0;
//页面显示标志位
uint8_t displayFlag = 0;

/*****************贯穿整个系统的函数 在主函数只需要调用这一个函数*/

void Mode_selection(void)
{
	Automatic(); // 自动控制
	
	Action(); // 调用执行设备函数
	
	Display(); // 数据显示
}

/*
自动控制逻辑判断
*/
void Automatic(void)
{
	// 正常模式灯光1常亮，防盗模式灯光2常亮
	if(displayFlag != 1){
		System.Switch1 = 1; // 灯光1开关标志位置1
		System.Switch2 = 0; // 灯光2开关标志位置0
	}else{
		System.Switch1 = 0; // 灯光1开关标志位置0
		System.Switch2 = 1; // 灯光2开关标志位置1
	}
}

/****
根据执行设备的标志位来开关执行设备
****/
void Action(void)
{
	if (System.Switch1 == 1) { // 灯光1开关标志位为1
        LED1_ON; // 打开灯光1
    } else {
        LED1_OFF; // 关闭灯光1
	}
	
    if (System.Switch2 == 1) { // 灯光2开关标志位为1
        LED2_ON; // 打开灯光2
    } else {
        LED2_OFF; // 关闭灯光2
    }
	
	if (System.Switch3 == 1) { // 报警标志位为1
		BEEP_ON; // 打开蜂鸣器
    } else {
		BEEP_OFF; // 关闭蜂鸣器
    }
	
	LED_Update();
}

/****
传感器数据显示
****/

void Display(void)
{	
	// 按下按键1，回到数据显示界面
	if (isKey1) {
		isKey1 = 0;
		oled_Clear();
		displayFlag = 0;
		LED_SetBlinkMode(STEADY_OFF); // 保持熄灭
		System.Switch3 = 0; // 报警开关标志位置0
	}
	
	// 按下按键2，进入防盗模式界面
	if (isKey2) {
		isKey2 = 0;
		oled_Clear();
		displayFlag = 1;
	}
	
	// 按下按键3，显示MQTT连接状态
	if(isKey3){
		isKey3 = 0;
		oled_Clear();
		displayFlag = 2;
	}
	
	// 按下按键4，显示订阅主题的参数
	if(isKey4){
		isKey4 = 0;
		oled_Clear();
		displayFlag = 3;
	}
	
	// 显示第一页
	if(displayFlag == 0)
	{
		//汽车防盗
		oled_ShowCHinese(16 * 2, 2 * 0, 0);
		oled_ShowCHinese(16 * 3, 2 * 0, 1);
		oled_ShowCHinese(16 * 4, 2 * 0, 2);
		oled_ShowCHinese(16 * 5, 2 * 0, 3);
		
		//经度
		oled_ShowCHinese(16 * 0, 2 * 1, 49);
		oled_ShowCHinese(16 * 1, 2 * 1, 51);
		oled_ShowString(16 * 2, 2 * 1, ":", 16);
		OLED_ShowFNum(16 * 3, 2 * 1, SensorData.longitudeVal, 16);//经度
		
		//纬度
		oled_ShowCHinese(16 * 0, 2 * 2, 50);
		oled_ShowCHinese(16 * 1, 2 * 2, 51);
		oled_ShowString(16 * 2, 2 * 2, ":", 16);
		OLED_ShowFNum(16 * 3, 2 * 2, SensorData.latitudeVal, 16);//纬度
	}
	
	// 显示第二页 
	if (displayFlag == 1) {
	   Anti_theft_mode();
	}
	
	// 调用显示mqtt连接状态参数
	if (displayFlag == 2) {
	   MqttCon_Display();
	}
	
	// 调用显示订阅主题的参数
	if (displayFlag == 3) {
	    Topic_Display();   
	}
}

void Anti_theft_mode(void)
{
	uint32_t CurrentTime = HAL_GetTick(); 	// 获取当前系统时钟
	static uint32_t First_ShakeCount_Time; 	// 第一次震动的时间
	static uint32_t IntervalTime = 2000; 	// 非法闯入判断间隔时间（2s）
	static uint32_t Execution_Time = 10000; // 检测到震动后的执行时间（10s）
	static uint8_t Alarm_ShakeCount = 3; 	// 非法闯入判断间隔时间内所需的次数
	static uint8_t Current_ShakeCount = 0;	// 当前震动计数
	
	//防盗模式
	oled_ShowCHinese(16 * 2, 2 * 0, 32);
	oled_ShowCHinese(16 * 3, 2 * 0, 33);
	oled_ShowCHinese(16 * 4, 2 * 0, 6);
	oled_ShowCHinese(16 * 5, 2 * 0, 7);
	
	// 非法闯入判断间隔时间 > 2s，清空震动计数
	if(Current_ShakeCount != 0 && CurrentTime - First_ShakeCount_Time > IntervalTime){
		ShakeCount = 0;
		Current_ShakeCount = 0;
	}
	
	// 震动计数为1时，蜂鸣器嘀一声，第三个指示灯慢速闪烁，oled屏幕上显示 发生震动
	else if(ShakeCount == 1){
		if(Current_ShakeCount == 0){
			First_ShakeCount_Time = CurrentTime; // 记录第一次震动时间
			Current_ShakeCount = 1;
		}
		
		if(Led_Controller.CurrentMode != SLOW_BLINK && Led_Controller.CurrentMode != FAST_BLINK){
			LED_SetBlinkMode(SLOW_BLINK); // 慢速闪烁
			Short_Beep(); // 蜂鸣器嘀一声
			
			// 发生震动
			oled_ShowCHinese(16 * 2, 2 * 2, 36);
			oled_ShowCHinese(16 * 3, 2 * 2, 37);
			oled_ShowCHinese(16 * 4, 2 * 2, 38);
			oled_ShowCHinese(16 * 5, 2 * 2, 39);
		}
	}
	
	// 在2秒内震动次数 ≥ 3次，或者红外光电检测到有人时
	else if(((ShakeCount >= Alarm_ShakeCount && CurrentTime - First_ShakeCount_Time <= IntervalTime)
		) && Led_Controller.CurrentMode != FAST_BLINK){
		
		Current_ShakeCount = 3;
		LED_SetBlinkMode(FAST_BLINK); // 快速闪烁
		System.Switch3 = 1; // 报警开关标志位置1
			
		// 非法入侵
		oled_ShowCHinese(16 * 2, 2 * 2, 12);
		oled_ShowCHinese(16 * 3, 2 * 2, 13);
		oled_ShowCHinese(16 * 4, 2 * 2, 14);
		oled_ShowCHinese(16 * 5, 2 * 2, 15);
	}
	
	// 检测到震动后执行10s后结束
	if(CurrentTime - First_ShakeCount_Time > Execution_Time){
		LED_SetBlinkMode(STEADY_OFF); // 保持熄灭
		System.Switch3 = 0; // 报警开关标志位置0
		oled_ShowString(16 * 2, 2 * 2, "          ", 16);
	}
}
