#include "sys.h"

uint8_t ShakeCount = 0; // 震动计数

/***************************************
 * 函数名：TIM2_Init
 * 功能：初始化定时器TIM2，配置基本参数并启动
 * 参数：
 *   - arr: 自动重装载值（决定定时周期）
 *   - psc: 预分频系数（时钟分频）
 * 说明：
 *   1. TIM2挂载在APB1总线，时钟需单独使能
 *   2. 定时器模式为向上计数
 *   3. 使能定时器更新中断
 **************************************/
void TIM2_Init(u32 arr, u32 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	
    /* 使能TIM2时钟（APB1总线） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* 配置定时器基础参数 */
    TIM_TimeBaseStructure.TIM_Period = arr;          // 自动重装载值（计数到arr后溢出）
    TIM_TimeBaseStructure.TIM_Prescaler = psc;      // 预分频系数（降低输入时钟频率）
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;     // 时钟分频（无额外分频）
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);  // 应用配置

    /* 使能TIM2更新中断（计数器溢出时触发） */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    /* 启动定时器TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}

/***************************************
 * 函数名：TIM2_IRQHandler
 * 功能：TIM2中断服务函数
 * 说明：
 *   1. 检测到更新中断（计数器溢出）时执行
 *   2. 调用按键扫描函数KeyScan()
 *   3. 必须清除中断标志位以防重复进入中断
 **************************************/
void TIM2_IRQHandler(void)
{
    /* 检查TIM2更新中断是否触发 */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        KeyScan(); // 执行按键扫描（需确保KeyScan()函数已实现）
		
        SensorData.VibrationVal = Shake_GetValue(); // 获取震动状态
		if(SensorData.VibrationVal == 0 && displayFlag == 1) ShakeCount ++;
		
        /* 清除TIM2更新中断标志位（重要！） */
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
