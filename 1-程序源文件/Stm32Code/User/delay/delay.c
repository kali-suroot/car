#include "sys.h"


/*使用到的 sysTickUptime 计数
需要在stm32f10x_it.c最上面添加  extern  volatile unsigned int       sysTickUptime;
void SysTick_Handler(void)里面添加  	sysTickUptime++;
*/
volatile unsigned int sysTickUptime;
#define usTicks (SystemCoreClock / 1000000)
unsigned int GetSysTime_us(void)
{
  register uint32_t ms, cycle_cnt;
  do
  {
    ms = sysTickUptime;
    cycle_cnt = SysTick->VAL;
  } while (ms != sysTickUptime);

  return ((ms * 1000) + 1000 - cycle_cnt / usTicks);
}

void delay_init()
{

  uint32_t prioritygroup = 0x00U;

  SysTick_Config(SystemCoreClock / (1000U));
  prioritygroup = NVIC_GetPriorityGrouping();

  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(prioritygroup, 0, 0));
}

void delay_us(u32 nus)
{
  uint32_t tickstart = GetSysTime_us();
  uint32_t wait = nus;

  while ((GetSysTime_us() - tickstart) < wait);
}

/**
 * @brief Provides a tick value in millisecond.
 * @note  This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval tick value
 */
uint32_t HAL_GetTick(void)
{
  return sysTickUptime;
}

void HAL_Delay(uint32_t Delay)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = Delay;

  while ((HAL_GetTick() - tickstart) < wait)
  {
  }
}

void delay_ms(u16 nms)
{
  HAL_Delay(nms);
}
