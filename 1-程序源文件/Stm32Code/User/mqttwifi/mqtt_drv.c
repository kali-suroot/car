#include "sys.h"

/*-------------------------------------------
 * MQTT协议及传感器相关全局变量定义
 *-------------------------------------------*/

unsigned char TEMIP_MAX;         // 温度/IP相关阈值上限（具体用途需结合上下文）
unsigned char TEMIP_MIN;         // 温度/IP相关阈值下限
unsigned char mqtt_buff[400];    // MQTT协议数据收发主缓冲区

/* MQTT状态标志位 */
unsigned char ConnectPack_flag = 0;  // MQTT连接成功标志位（1=已连接）
unsigned char SubscribePack_flag = 0;// 主题订阅成功标志位（1=已订阅）
unsigned char Ping_flag = 0;         // 心跳包状态标志（1=待发送）

/* 传感器数据相关 */
int sensorMax = 0;                // 传感器测量最大值缓存
uint16_t ReadData = 0;            // 传感器原始数据读取缓存（16位ADC值）
float tempx;                      // 浮点型临时计算变量（单位根据场景变化）



/* 通用字符串缓冲区 */
char str[20] = "";               // 通用临时字符串存储（LCD显示/数据转换用）

/* MQTT接收缓冲区及指针 */
unsigned char  mqtt_RxBuf[7][250]; // 接收缓冲矩阵（7通道*250字节）
unsigned char *mqtt_RxInPtr;       // 接收缓冲区写入指针
unsigned char *mqtt_RxOutPtr;      // 接收缓冲区读取指针
unsigned char *mqtt_RxEndPtr;      // 接收缓冲区结束地址指针

/* 设备标识缓冲区 */
char dest[5] = {0};               // 短地址存储（如设备ID片段）


/* MQTT连接参数 */
char clientid[128];               // 客户端ID存储（格式如"Device_123"）
int  clientid_size;               // 客户端ID实际长度
char lex[256];                    // 主题(Topic)存储缓冲区
char username[128];               // MQTT用户名
int  username_size;               // 用户名实际长度
char passward[128];               // MQTT密码（明文存储需注意安全）
int  passward_size;               // 密码实际长度

/*-------------------------------------------
 * 函数名：mqtt_TxData
 * 功能：通过串口逐字节发送MQTT协议数据包
 * 参数：data - 数据包指针（协议格式：data[0]和data[1]为长度高位/低位字节，后续为实际数据）
 * 实现逻辑：
 *   1. 数据包前2字节组合为16位长度值（大端格式）
 *   2. 根据长度值循环发送后续字节数据
 *-------------------------------------------*/
void mqtt_TxData(unsigned char *data)
{
    //send_data_to_dev(data, strlen(data));  // 原方案：直接发送（已废弃，因长度计算不可靠）
    int i;

    /* 解析协议头中的长度字段（大端模式） */
    for(i = 1; i <= (data[0] * 256 + data[1]); i++) // data[0]<<8 | data[1] 组合成数据长度
    {
        //USART_SendData(USART2,data[i+1]);  // 底层寄存器操作方案（被封装替代）
        send_data_to_dev((char *)&data[i + 1], 1); // 逐字节发送有效数据（从data[2]开始）
    }
}

/*-------------------------------------------
 * 函数名：MQTT_Buff_Init
 * 功能：初始化MQTT协议相关缓冲区并启动连接流程
 * 实现逻辑：
 *   1. 清除MQTT连接状态标志
 *   2. 初始化接收缓冲区环形队列指针
 *   3. 发送MQTT CONNECT协议包建立连接
 *   4. 发送SUBSCRIBE协议包订阅主题
 *-------------------------------------------*/
void MQTT_Buff_Init(void)
{
    strEsp8266_info.MQTT_Connect_flag = 0;  // 重置MQTT连接状态标志（0=未连接）

    /* 接收缓冲区环形队列初始化 */
    mqtt_RxInPtr = mqtt_RxBuf[0];    // 接收写入指针指向缓冲区首地址
    mqtt_RxOutPtr = mqtt_RxInPtr;    // 接收读取指针与写入指针同步（空缓冲区）
    mqtt_RxEndPtr = mqtt_RxBuf[6];   // 缓冲区结束地址指向第七通道首地址（环形队列设计）

    Mqtt_ConnectMessege();           // 构造并发送MQTT连接请求（协议类型、心跳间隔等）
    // Mqtt_Subscribe(strMqtt_Inof.sub, 1); // 订阅动态生成的主题（QoS等级=1）
}

/*-----------------------------------------------------------
 * 函数名：Mqtt_Subscribe
 * 功能：构造并发送MQTT SUBSCRIBE协议报文
 * 协议结构：
 *   | 固定头(0x82) | 剩余长度 | 报文标识符(0x000A) | 主题长度 | 主题内容 | QoS等级 |
 * 参数：
 *   topic_name - 需要订阅的主题字符串
 *   QoS - 服务质量等级（0/1/2）
 *-----------------------------------------------------------*/
void Mqtt_Subscribe(char *topic_name, int QoS)
{
    int payload_size;       // 有效载荷总长度（主题+QoS）
    int remaining_size;     // 剩余长度字段值
    int len;                // 单字节编码值
    int all_size = 1;       // 当前写入位置（从固定头后开始）
    int variable_size = 2;  // 报文标识符固定占2字节

    /* 计算协议字段长度 */
    payload_size = 2 + strlen(topic_name) + 1; // 主题长度字段(2) + 主题内容 + QoS(1)
    remaining_size = variable_size + payload_size; // 剩余长度=报文标识符+有效载荷

    /* 固定头构造 */
    mqtt_buff[0] = 0x82; // SUBSCRIBE类型(8) + QoS=1(最低位0010)

    /* 剩余长度编码（变长编码算法） */
    while(remaining_size > 0)
    {
        len = remaining_size % 128;    // 取低7位
        remaining_size = remaining_size / 128; // 右移7位

        if(remaining_size > 0)         // 如果还有后续字节
        {
            len = len | 0x80;          // 设置最高位为1
        }

        mqtt_buff[all_size++] = len;   // 写入编码后的字节
    }

    /* 报文标识符（示例值0x000A，实际应动态生成） */
    mqtt_buff[all_size + 0] = 0x00;    // 标识符高位
    mqtt_buff[all_size + 1] = 0x0A;    // 标识符低位

    /* 主题长度（大端编码） */
    mqtt_buff[all_size + 2] = strlen(topic_name) / 256; // 长度高位
    mqtt_buff[all_size + 3] = strlen(topic_name) % 256; // 长度低位

    /* 主题内容拷贝 */
    memcpy(&mqtt_buff[all_size + 4], topic_name, strlen(topic_name));

    /* QoS等级写入 */
    mqtt_buff[all_size + 4 + strlen(topic_name)] = QoS;

    /* 提交数据到发送处理（含完整报文长度计算） */
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size);
}
/*-----------------------------------------------------------
 * 函数名：Mqtt_ConnectMessege
 * 功能：构造并发送MQTT CONNECT协议报文
 * 协议结构：
 *   | 固定头(0x10) | 剩余长度 | 协议名(4字节) | 协议等级 | 连接标志 | 心跳间隔 |
 *   | 客户端ID长度 | 客户端ID | 用户名长度 | 用户名 | 密码长度 | 密码 |
 *-----------------------------------------------------------*/
void Mqtt_ConnectMessege(void)
{
    int remaining_size;    // 剩余长度字段值
    int len;               // 单字节编码值
    int variable_size;     // 固定协议头长度
    int payload_size;      // 有效载荷总长度
    int all_size = 1;      // 当前写入位置（从固定头后开始）

    variable_size = 10;    // 协议头固定部分长度（协议名+等级+标志+心跳）
    payload_size = 2 + clientid_size  + 2 + username_size  + 2 + passward_size; // 客户端ID+用户名+密码及其长度字段
    remaining_size = variable_size + payload_size;         // 总剩余长度=固定头+有效载荷

    /* 固定头构造 */
    mqtt_buff[0] = 0x10;  // CONNECT报文类型(0001 0000)

    /* 剩余长度编码（变长编码算法） */
    while(remaining_size > 0)
    {
        len = remaining_size % 128;    // 取低7位
        remaining_size = remaining_size / 128;

        if(remaining_size > 0)
        {
            len = len | 0x80;          // 设置最高位为1（表示后续字节）
        }

        mqtt_buff[all_size++] = len;   // 写入编码后的字节
    }

    /* 协议描述段 */
    mqtt_buff[all_size + 0] = 0x00;    // 协议名长度高位(固定4字节)
    mqtt_buff[all_size + 1] = 0x04;    // 协议名长度低位
    mqtt_buff[all_size + 2] = 0x4D;    // 'M' ASCII码
    mqtt_buff[all_size + 3] = 0x51;    // 'Q' ASCII码
    mqtt_buff[all_size + 4] = 0x54;    // 'T' ASCII码
    mqtt_buff[all_size + 5] = 0x54;    // 'T' ASCII码（协议名"MQTT"）
    mqtt_buff[all_size + 6] = 0x04;    // 协议等级4（MQTT v3.1.1）

    /* 连接标志段 */
    mqtt_buff[all_size + 7] = 0xC2;    // 标志位：bit7=1(清理会话) bit1=1(密码) bit0=1(用户名)
    mqtt_buff[all_size + 8] = 0x00;    // 心跳间隔高位（100秒=0x0064）
    mqtt_buff[all_size + 9] = 0x64;    // 心跳间隔低位

    /* 客户端ID段 */
    mqtt_buff[all_size + 10] = clientid_size / 256;      // ID长度高位
    mqtt_buff[all_size + 11] = clientid_size % 256;      // ID长度低位
    memcpy(&mqtt_buff[all_size + 12], clientid, clientid_size); // 拷贝ID内容

    /* 用户名段 */
    mqtt_buff[all_size + 12 + clientid_size] = username_size / 256;    // 用户名长度高位
    mqtt_buff[all_size + 13 + clientid_size] = username_size % 256;    // 用户名长度低位
    memcpy(&mqtt_buff[all_size + 14 + clientid_size], username, username_size); // 拷贝用户名

    /* 密码段 */
    mqtt_buff[all_size + 14 + clientid_size + username_size] = passward_size / 256;    // 密码长度高位
    mqtt_buff[all_size + 15 + clientid_size + username_size] = passward_size % 256;    // 密码长度低位
    memcpy(&mqtt_buff[all_size + 16 + clientid_size + username_size], passward, passward_size); // 拷贝密码

    /* 提交完整报文到发送队列 */
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size); // 总长度=固定头+编码字节+协议内容
}
/*-----------------------------------------------------------
 * 函数名：mqtt_PublishQs0
 * 功能：构造并发送QoS0级别的MQTT PUBLISH报文
 * 协议结构：
 *   | 固定头(0x30) | 剩余长度 | 主题长度 | 主题内容 | 消息载荷 |
 * 参数：
 *   topic    - 发布主题字符串
 *   data     - 消息载荷数据指针
 *   data_len - 消息载荷数据长度
 * 注：QoS0不需要报文标识符（无消息确认机制）
 *-----------------------------------------------------------*/
void mqtt_PublishQs0(char *topic, char *data, int data_len)
{
    int all_size = 1;       // 当前写入位置（固定头后开始）
    int variable_size;      // 可变头长度（主题长度字段+主题内容）
    int payload_size;       // 有效载荷长度
    int remaining_size;     // 剩余长度字段值
    int len;                // 单字节编码值

    /* 计算协议字段长度 */
    variable_size = 2 + strlen(topic); // 主题长度字段(2字节)+主题内容
    payload_size = data_len;           // 消息载荷长度
    remaining_size = variable_size + payload_size; // 总剩余长度

    /* 固定头构造 */
    mqtt_buff[0] = 0x30;  // PUBLISH报文类型(0011 0000)，QoS=0，保留位=0

    /* 剩余长度编码（MQTT变长编码规范） */
    while(remaining_size > 0)
    {
        len = remaining_size % 128;    // 取低7位
        remaining_size = remaining_size / 128;

        if(remaining_size > 0)
        {
            len = len | 0x80;          // 设置最高位表示后续字节
        }

        mqtt_buff[all_size++] = len;   // 写入编码字节
    }

    /* 主题长度字段（大端编码） */
    mqtt_buff[all_size + 0] = strlen(topic) / 256; // 长度高位
    mqtt_buff[all_size + 1] = strlen(topic) % 256; // 长度低位

    /* 主题内容拷贝 */
    memcpy(&mqtt_buff[all_size + 2], topic, strlen(topic));

    /* 消息载荷拷贝 */
    memcpy(&mqtt_buff[all_size + 2 + strlen(topic)], data, data_len);

    /* 提交完整报文（总长度=固定头+剩余长度编码字节+可变头+载荷） */
    mqtt_DealTxData(mqtt_buff, all_size + variable_size + payload_size);
}

/*-------------------------------------------
 * 函数名：mqtt_DealTxData
 * 功能：封装MQTT协议数据并提交发送
 * 协议格式：
 *   | 数据长度高位(2字节) | 原始数据内容 |
 *-------------------------------------------*/
unsigned char Tx_Buff[400]; // 发送数据静态缓冲区

void mqtt_DealTxData(unsigned char *data, int size)
{
    /* 当前实现：静态缓冲区协议封装 */
    Tx_Buff[0] = size / 256;   // 数据长度高位（大端格式）
    Tx_Buff[1] = size % 256;   // 数据长度低位
    memcpy(&Tx_Buff[2], data, size); // 拷贝协议数据到缓冲区（偏移2字节）
    mqtt_TxData(Tx_Buff);      // 调用底层发送函数（含长度头和数据体）
}


/*-----------------------------------------------------------
 * 函数名：mqtt_DealCmdSet
 * 功能：缓存接收到的命令数据并设置处理标志
 * 协议格式：
 *   | 保留2字节 | 原始数据内容 | （历史方案含长度头，当前已简化）
 * 注意：
 *   - 当前实现未包含长度字段，需确保外部传入size不超过398字节
 *-----------------------------------------------------------*/
unsigned char is_new_CMD_rx = 0;    // 新命令接收标志（1=有新数据待处理）
unsigned char CMD_rx_buff[400];     // 命令接收缓冲区（前2字节保留）

void mqtt_DealCmdSet(unsigned char *data, int size)
{
    /* 当前实现：静态缓冲区存储 */
    memcpy(&CMD_rx_buff[2], data, size); // 数据存入缓冲区[2]之后（前2字节保留）
    is_new_CMD_rx = 1;                   // 触发新数据处理标志
    /* 潜在风险：当size>398时会溢出，需调用方控制数据长度 */
}


/*-----------------------------------------------------------
 * 函数名：mqtt_Dealsetdata_Qs0
 * 功能：解析QoS0级别的MQTT PUBLISH报文，提取有效载荷数据
 * 协议结构：
 *   | 固定头 | 剩余长度（变长编码） | 主题长度 | 主题内容 | 消息载荷 |
 * 实现流程：
 *   1. 解析剩余长度字段（MQTT变长编码规范）
 *   2. 计算主题长度（含2字节长度字段）
 *   3. 计算消息载荷在数据流中的位置
 *   4. 提取消息载荷并提交处理
 *-----------------------------------------------------------*/
void mqtt_Dealsetdata_Qs0(unsigned char *redata)
{
    int i, topic_len, cmd_len, cmd_local, temp, temp_len, multiplier;
    unsigned char tempbuff[400];  // 临时存储有效载荷数据
    unsigned char *data;          // 指向原始数据偏移2字节后的位置（跳过长度头）
    temp_len = 0;             // 剩余长度计算值
    i = multiplier = 1;          // 变长编码解析起始位置
    data = &redata[2];         // 跳过外部长度头（非MQTT标准头）

    /* 步骤1：解析MQTT剩余长度字段（变长编码） */
    do
    {
        temp = data[i];                     // 读取当前字节
        temp_len += (temp & 127) * multiplier; // 取低7位并累加
        multiplier *= 128;                   // 位权倍增（7位编码）
        i++;                                // 指针后移
    }
    while((temp & 128) != 0);               // 最高位为1时继续解析

    /* 步骤2：计算主题长度（大端格式） */
    topic_len = data[i] * 256 + data[i + 1] + 2; // 主题长度=主题内容长度+2字节长度字段
    /* 步骤3：计算消息载荷长度和起始位置 */
    cmd_len = temp_len - topic_len;          // 有效载荷=总长度-主题相关长度
    cmd_local = topic_len + i;               // 有效载荷起始位置=主题结尾+变长编码偏移
    /* 步骤4：提取有效载荷并提交处理 */
    memcpy(tempbuff, &data[cmd_local], cmd_len); // 拷贝有效载荷到临时缓冲区
    mqtt_DealCmdSet(tempbuff, cmd_len);      // 提交给命令处理模块
}



/*-----------------------------------------------------------
 * 函数名：mqtt_Ping
 * 功能：构造并发送MQTT PINGREQ心跳请求包
 * 协议规范：
 *   | 固定头(0xC0) | 剩余长度(0x00) |
 *   1. PINGREQ包固定2字节，无有效载荷
 *   2. 0xC0 = 11000000，高4位1100表示PINGREQ类型，低4位保留全0
 *   3. 0x00表示剩余长度为0（无后续内容）
 *-----------------------------------------------------------*/
void mqtt_Ping(void)
{
    mqtt_buff[0] = 0xC0;  // PINGREQ报文类型（二进制11000000）
    mqtt_buff[1] = 0x00;  // 剩余长度字段（值为0，无可变头和载荷）
    mqtt_DealTxData(mqtt_buff, 2); // 发送完整心跳包（2字节长度）
}

/*-----------------------------------------------------------
 * 函数名：Ali_MsessageInit
 * 功能：初始化阿里云MQTT连接参数（客户端ID/用户名/密码）
 * 注：
 *   1. 代码中 memset 参数顺序错误（正确应为 memset(目标, 值, 长度)）
 *   2. BAD 变量未定义（历史遗留问题，已注释）
 *-----------------------------------------------------------*/
void Ali_MsessageInit()
{
    /* 客户端ID初始化 */
    memset(clientid, 128, 0);  // 错误用法：意图清空128字节缓冲区，实际填充值错误（应为 memset(clientid, 0, 128)）
    sprintf(clientid, "%s", strMqtt_Inof.device_name); // 从结构体拷贝设备名（如"Device_123"）
    clientid_size = strlen(clientid);                  // 计算客户端ID长度

    /* 用户名初始化 */
    memset(username, 128, 0);  // 同上错误
    sprintf(username, "%s", strMqtt_Inof.key);         // 从结构体拷贝产品密钥（如"PK_ABCD"）
    username_size = strlen(username);                  // 计算用户名长度

    /* 密码初始化 */
    memset(passward, 128, 0);  // 同上错误
    sprintf(passward, "%s", strMqtt_Inof.secre);       // 从结构体拷贝设备密钥（如"DS_123456"）
    passward_size = strlen(passward);                  // 计算密码长度

}

/*-----------------------------------------------------------
 * 函数名：mid
 * 功能：从源字符串src的指定位置m开始截取n个字符到目标字符串dst
 * 参数：
 *   dst - 目标字符串缓冲区（需确保足够空间）
 *   src - 源字符串
 *   n   - 期望截取长度（实际可能被调整）
 *   m   - 起始位置（从0开始计数）
 * 返回值：
 *   成功返回dst指针，失败返回NULL（当m>src长度时）
 * 注意：
 *   1. 当n超过剩余长度时，自动截取到字符串末尾
 *   2. 起始位置m会被强制约束在[0, src长度]范围内
 *   3. 目标字符串始终以'\0'终止（可能造成缓冲区溢出风险）
 *-----------------------------------------------------------*/
char *mid(char *dst, char *src, int n, int m) /*n为长度，m为位置*/
{
	char *p = src;  // 源字符串指针
	char *q = dst;  // 目标字符串指针
	int len = strlen(src); // 获取源字符串长度
	
	/* 边界条件处理 */
	if(n > len)
	{
		n = len - m;    /*从第m个到最后（当n超过剩余长度时修正）*/
	}
	
	if(m < 0)
	{
		m = 0;    /*从第一个开始（负数位置强制归零）*/
	}
	
	if(m > len)
	{
		return NULL; // 起始位置超出字符串长度，返回空指针
	}
	
	p += m; // 移动源指针到起始位置
	
	/* 执行拷贝操作 */
	while(n--)
	{
		*(q++) = *(p++); // 逐字符复制
	}
	
	*(q++) = '\0'; /*有必要吗？很有必要 - 强制添加终止符（若dst空间不足可能溢出）*/
	return dst; // 返回目标字符串指针
}




/*-----------------------------------------------------------
 * 函数名：period_get_server
 * 功能：周期性地将串口接收数据存入环形缓冲区（20ms间隔）
 * 设计说明：
 *   1. 使用环形缓冲区解决数据持续接收与处理的异步问题
 *   2. 缓冲区结构：| 长度高位(1B) | 长度低位(1B) | 数据内容(NB) | ...
 *   3. 每个缓冲区单元固定400字节（长度头2B+数据最大398B）
 * 注意：
 *   - 缓冲区溢出风险：当rcv_cnt>398时会覆盖后续数据
 *-----------------------------------------------------------*/
void period_get_server(void)
{
	/* 触发条件：距上次接收时间≥20ms 且 存在待处理数据 */
	if(get_sys_tick() - strEsp8266_rcv.rcv_trick_ms >= 20 && strEsp8266_rcv.rcv_cnt)
	{
		/* 数据封装：前2字节写入数据长度（大端格式） */
		memcpy(&mqtt_RxInPtr[2], strEsp8266_rcv.buff, strEsp8266_rcv.rcv_cnt);  // 数据拷贝（偏移2字节）
		mqtt_RxInPtr[0] = strEsp8266_rcv.rcv_cnt / 256;  // 长度高位
		mqtt_RxInPtr[1] = strEsp8266_rcv.rcv_cnt % 256;  // 长度低位
	
		/* 环形缓冲区指针维护 */
		mqtt_RxInPtr += 400;  // 指向下一个缓冲区单元（400字节/单元）
	
		if(mqtt_RxInPtr >= mqtt_RxEndPtr)     // 到达环形缓冲区末端
		{
			mqtt_RxInPtr = mqtt_RxBuf[0];    // 指针回绕到起始地址
		}
	
		strEsp8266_rcv.rcv_cnt = 0;  // 清空接收计数器（隐含清空缓冲区动作）
	}
}

/*-----------------------------------------------------------
 * 函数名：mqtt_Content
 * 功能：MQTT协议核心处理引擎（接收数据解析/状态管理/命令处理）
 * 主要逻辑：
 *   1. 接收缓冲区处理：解析CONNACK/SUBACK/PINGRESP/PUBLISH等协议包
 *   2. 命令缓冲区处理：触发新命令处理流程
 * 设计特点：
 *   - 使用环形缓冲区实现异步通信
 *   - 支持MQTT协议多状态管理（连接/订阅/心跳/推送）
 *-----------------------------------------------------------*/
void mqtt_Content(void)
{
	/* 接收缓冲区处理（当读写指针不同时） */
	if(mqtt_RxOutPtr != mqtt_RxInPtr)
	{
		/* CONNACK报文处理（0x20=连接响应） */
		if(mqtt_RxOutPtr[2] == 0x20)
		{
			switch(mqtt_RxOutPtr[5])  // 读取连接返回码
			{
				case 0x00 :
					/* 连接成功 */
					ConnectPack_flag = 1;
					strEsp8266_info.MQTT_Connect_flag = 1;
					break;
	
				case 0x01 :
					/* 协议版本不支持 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
	
				case 0x02 :
					/* 客户端标识不合法 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
	
				case 0x03 :
					/* 服务端不可用 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
	
				case 0x04 :
					/* 用户名密码错误 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
	
				case 0x05 :
					/* 未授权 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
	
				default   :
					/* 其他错误 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
			}
		}
		/* SUBACK报文处理（0x90=订阅响应） */
		else if(mqtt_RxOutPtr[2] == 0x90)
		{
			switch(mqtt_RxOutPtr[6])  // 读取订阅状态码
			{
				case 0x01 :  /* QoS1订阅成功 */
					SubscribePack_flag = 1;
					Ping_flag = 0;
					mqtt_PublishQs0(strMqtt_Inof.pub, lex, strlen(lex)); // 触发首次数据发布
					break;
				default   :  /* 订阅失败 */
					strEsp8266_info.MQTT_Connect_flag = 0;
					break;
			}
		}
		/* PINGRESP报文处理（0xD0=心跳响应） */
		else if(mqtt_RxOutPtr[2] == 0xD0)
		{
			Ping_flag = 1;  /* 心跳响应正常标志 */
		}
		/* PUBLISH报文处理（0x30=服务器推送数据） */
		else if(mqtt_RxOutPtr[2] == 0x30)
		{
			mqtt_Dealsetdata_Qs0(mqtt_RxOutPtr);  /* 解析有效载荷数据 */
		}
	
		/* 环形缓冲区指针维护 */
		mqtt_RxOutPtr += 400;  /* 移动到下一个缓冲区单元（400字节/单元） */
	
		if(mqtt_RxOutPtr >= mqtt_RxEndPtr)
		{
			mqtt_RxOutPtr = mqtt_RxBuf[0];  /* 环形回绕到起始地址 */
		}
	}
	
	/* 命令缓冲区处理（新数据到达时） */
	if(is_new_CMD_rx == 1) //(mqtt_CMDOutPtr != mqtt_CMDInPtr)
	{
		is_new_CMD_rx = 0;  /* 清除标志位（实际命令处理逻辑需外部实现） */
		
		// 数据显示
		if(strstr((char *)CMD_rx_buff + 2, "Display")){
			oled_Clear();
			mode_selection = 0;
			displayFlag = 0;
			System.Switch3 = 0;
		}
		
		// 防盗模式
		if(strstr((char *)CMD_rx_buff + 2, "Anti_theft_mode")){
			oled_Clear();
			displayFlag = 1;
		}
	}
}



