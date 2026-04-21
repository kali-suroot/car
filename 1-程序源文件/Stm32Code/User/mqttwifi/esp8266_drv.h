#ifndef _ESP8266_DRV_H_
#define _ESP8266_DRV_H_

#include "sys.h"

#define ID   "abcd" //热点账号                    
#define PASSWORD   "12345678" //热点密码

#define ESP8266_RESET_LCK        RCC_APB2Periph_GPIOA
#define ESP8266_RESET_PORT       GPIOA
#define ESP8266_RESET_PIN        GPIO_Pin_1

#define ESP8266_RESET_SET_H     GPIO_SetBits(ESP8266_RESET_PORT, ESP8266_RESET_PIN)  //HAL_GPIO_WritePin(ESP8266_RESET_PORT,ESP8266_RESET_PIN,x)
#define ESP8266_RESET_SET_L     GPIO_ResetBits(ESP8266_RESET_PORT, ESP8266_RESET_PIN)
typedef struct
{

    unsigned int rcv_trick_ms;
    unsigned char buff[256];
    unsigned int rcv_cnt;

} DATA_RCV_STR;

typedef struct
{

    unsigned char Net_stu;      //网络状态
    unsigned char Cmd_stu;      //命令状态
    unsigned int cmd_send_trick;    //指令发送时间
    unsigned char retry_cnt;
    unsigned char run_heart;            //运行次数
    unsigned short rcv_idle_cnt;        //单位s
    unsigned char offline_cnt;          //离线次数
    unsigned char MQTT_Connect_flag;
} ESP8266_STR;

typedef struct
{

    char cmd[80];
    char respond[20];
    unsigned short wait_time;
    unsigned short retry_cnt;

} CMD_TYPE_STRUCT;

typedef enum
{
    NET_NULL = 0,
    NET_RESET,
    NET_AT,
    NET_RST,
    NET_CWAUTOCONN,
    NET_CAJAP,
    NET_CIPMUX,
    NET_CIPMODE,
    NET_CIPSTART,
    NET_CIPSEND,


} ESP8266_NET_STU;

//MQTT连接参数结构体
typedef struct
{
    uint32_t device_id;
    unsigned int Round_num;//随机数
    char device_name[20];//DEVICENAME
    char key[20];			//PRODUCTKEY
    char secre[20];		//DEVICESECRE
    char sub[20];     //SUBSCRIBE_TOPIC
    char pub[20];     //P_TOPIC_NAME

} STR_MQTT_INFO;


extern STR_MQTT_INFO strMqtt_Inof;

unsigned int get_sys_tick(void);
void send_data_to_dev(char *data, unsigned short len);
void USART_rcv_ch(unsigned char ch, DATA_RCV_STR *str_rcv);
void ESP8266_run_handle(void);
void USARTx_RCVHandler(USART_TypeDef *USARTX);
extern ESP8266_STR strEsp8266_info;
extern DATA_RCV_STR strEsp8266_rcv;      //调试串口接收

unsigned int get_round_num(void);//生成随机数的函数
void Mqtt_Parameter_init(void);//MQTT连接参数
void WIFI_RESET_init(void);//WIFI复位引脚初始化
void MqttCon_Display(void);//显示连接参数
void Topic_Display(void);//显示订阅主题
void mqttPublic(void);//向mqtt服务器发送数据
void Scheduled (void);//定时1秒发送一次
#endif
