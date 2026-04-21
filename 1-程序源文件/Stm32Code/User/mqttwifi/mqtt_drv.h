#ifndef __MQTT_H__
#define __MQTT_H__








//此处已注释不做使用
//#define  DEVICESECRE_LEN      strlen(DEVICESECRE)

//#define  SUBSCRIBE_TOPIC         "/SmartWaterGlasses0329/Sub"
//#define  P_TOPIC_NAME            "/SmartWaterGlasses0329/Pub"    //需要发布的主题
//阿里云
//#define  DEVICENAME      "k1ctrLnsSlm.MYLED|securemode=2,signmethod=hmacsha256,timestamp=1716881898347|"
//#define  PRODUCTKEY      "MYLED&k1ctrLnsSlm"
//#define  DEVICESECRE     "7289b93a675f4a9587bdc39838ce942ae2cdd8c06c9398e7a849520dbbf1c6e0"
//#define  BAD     		 "iot-06z00g5z48h0n39.mqtt.iothub.aliyuncs.com"

/*-----------------------------------------------------------
 * MQTT协议栈核心头文件（esp8266_drv.h扩展）
 * 功能：定义协议参数、缓冲区和关键函数接口
 * 注：保留历史实现方案注释以体现架构演进
 *-----------------------------------------------------------*/

/* 设备密钥长度宏（动态计算） */
#define  DEVICESECRE_LEN      strlen(DEVICESECRE)

/* 历史发送接口定义（已注释弃用） */
//#define  mqtt_TxData(x)       u2_TxData(x)

/* 服务器连接参数 */
extern char ServerIP[128];   // MQTT Broker IP地址（例："iot-as-mqtt.cn-shanghai.aliyuncs.com"）
extern int  ServerPort;      // MQTT Broker端口（默认1883）

/* 历史发送缓冲区配置（已注释弃用） */
//extern unsigned char  mqtt_TxBuf[7][400];
//extern unsigned char *mqtt_TxInPtr;
//extern unsigned char *mqtt_TxOutPtr;
//extern unsigned char *mqtt_TxEndPtr;

/* 接收缓冲区配置（环形队列实现） */
extern unsigned char  mqtt_RxBuf[7][250];    // 接收缓冲区矩阵（7通道x250字节）
extern unsigned char *mqtt_RxInPtr;          // 接收缓冲区写入指针
extern unsigned char *mqtt_RxOutPtr;         // 接收缓冲区读取指针
extern unsigned char *mqtt_RxEndPtr;         // 接收缓冲区结束地址指针

/* 历史指令缓冲区配置（已注释弃用） */
//extern unsigned char  mqtt_CMDBuf[7][400];
//extern unsigned char *mqtt_CMDInPtr;
//extern unsigned char *mqtt_CMDOutPtr;
//extern unsigned char *mqtt_CMDEndPtr;

/* MQTT协议状态标志 */
extern unsigned char ConnectPack_flag;    // 连接成功标志（1=已连接）
extern unsigned char SubscribePack_flag;  // 订阅成功标志（1=已订阅）
extern unsigned char Ping_flag;           // 心跳响应标志（1=心跳正常）

/* 核心功能函数声明 */
void Mqtt_ConnectMessege(void);         // 构造MQTT CONNECT协议包
void Ali_MsessageInit(void);            // 初始化连接参数（ClientID/Username/Password）
void MQTT_Buff_Init(void);              // 初始化协议缓冲区并启动连接流程
void mqtt_Ping(void);                   // 发送MQTT心跳包（PINGREQ）
void mqtt_PublishQs0(char *topic, char *data, int data_len); // QoS0级别数据发布
void mqtt_Dealsetdata_Qs0(unsigned char *redata); // QoS0级别数据解析
void mqtt_Content(void);                // MQTT协议核心状态机（处理接收数据）
void period_get_server(void);           // 周期性处理服务器数据（20ms间隔）

/* 辅助功能函数 */
void Mqtt_Subscribe(char *topic_name, int QoS);       // 构造订阅请求包
void mqtt_DealTxData(unsigned char *data, int size);  // 协议数据发送封装

#endif

