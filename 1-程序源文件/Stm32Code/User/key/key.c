#include "sys.h"

u8 isKey1 = 0;
u8 isKey2 = 0;
u8 isKey3 = 0;
u8 isKey4 = 0;

/*
******按键初始化函数*****************************************
*	功能:	按键初始化函数
*	入口参数:	无
*	返回参数:	无
*	说明:		KEY1-PB12		KEY2-PB13   KEY3-PB14    KEY4-PB15
************************************************************
*/
void KEY_Init(void) //IO初始化
{
    GPIO_InitTypeDef GPIO_InitStructure; // 定义GPIO初始化结构体

    /* 使能按键GPIO时钟 */
    RCC_APB2PeriphClockCmd(KEY_GPIO_CLK, ENABLE); // 开启按键所在GPIO端口的时钟（APB2总线）

    /* 配置GPIO初始化参数 */
    GPIO_InitStructure.GPIO_Pin  = KEY1_GPIO_PIN | KEY2_GPIO_PIN | KEY3_GPIO_PIN | KEY4_GPIO_PIN; // 选择所有按键的引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置为上拉输入模式（默认高电平，按键按下时拉低）

    /* 初始化各按键GPIO */
    GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);  // 初始化KEY1所在端口
    GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);  // 初始化KEY2所在端口
    GPIO_Init(KEY3_GPIO_PORT, &GPIO_InitStructure);  // 初始化KEY3所在端口
    GPIO_Init(KEY4_GPIO_PORT, &GPIO_InitStructure);  // 初始化KEY4所在端口
}


/* KEY1按键事件处理函数 */
void Key1Press(void)
{
    if(isKey1 == 0)  // 检查按键状态标志位
        isKey1=1;    // 置位标志位（标记按键被按下）
}

/* KEY2按键事件处理函数 */
void Key2Press(void)
{
    if(isKey2 == 0)  // 检查按键状态标志位
        isKey2=1;    // 置位标志位（标记按键被按下）
}

/* KEY3按键事件处理函数 */
void Key3Press(void)
{
    if(isKey3 == 0)  // 检查按键状态标志位
        isKey3=1;    // 置位标志位（标记按键被按下）
}

/* KEY4按键事件处理函数 */
void Key4Press(void)
{
    if(isKey4 == 0)  // 检查按键状态标志位
        isKey4=1;    // 置位标志位（标记按键被按下）
}

/*
******按键扫描函数*****************************************
*	功能:	按键扫描函数
*	入口参数:	无
*	返回参数:	无
*	说明:		KEY1-PB12		KEY2-PB13   KEY3-PB14    KEY4-PB15
************************************************************
*/
void KeyScan(void)
{
    static int keyCount = 0;
    static int keyState = 0;
    if (KEY1 == 0 && keyState == 0) //按键按下
    {
        keyCount++;
        if (keyCount > 2 && KEY1 == 0 && keyState == 0) //加两次类似延迟10ms，不好解释
        {
            /*ToDo:按键按下执行的操作*/
            Key1Press();
            keyState = 1;
        }
    }
    else if (KEY2 == 0 && keyState == 0)
    {
        keyCount++;
        if (keyCount > 2 && KEY2 == 0 && keyState == 0)
        {
            /*ToDo:按键按下执行的操作*/
            Key2Press();
            keyState = 1;
        }
    }
    else if (KEY3 == 0 && keyState == 0)
    {
        keyCount++;
        if (keyCount > 2 && KEY3 == 0 && keyState == 0)
        {
            /*ToDo:按键按下执行的操作*/
            Key3Press();
            keyState = 1;
        }
    }
    else if (KEY4 == 0 && keyState == 0)
    {
        keyCount++;
        if (keyCount > 2 && KEY4 == 0 && keyState == 0)
        {
            /*ToDo:按键按下执行的操作*/
            Key4Press();
            keyState = 1;
        }
    }

    
    else if (KEY1 == 1 && KEY2 == 1 && KEY3 == 1 && KEY4 == 1 &&keyState == 1) //当所有按键都处于抬起状态，状态刷新
		{
        keyCount = 0;
        keyState = 0;
    }
}
