#include "ip_addr.h"
#include "cJSON.h"
#define SOCKET_OUT_BUFFER_SIZE 256
#define SOCKET_IN_BUFFER_SIZE (4096+256)
typedef struct _ExpEvent {
	bool connectErrorFlag;
} ExpEvent;

typedef struct _SocketInfo {
    char inBuffer[SOCKET_IN_BUFFER_SIZE];   // Socket 通讯接收 Buffer
    char outBuffer[SOCKET_OUT_BUFFER_SIZE]; // Socket 通讯发送 Buffer
} SocketInfo;

typedef struct _OTAInfo {
	  int blockOffset;  // 请求的分包后的单个包偏移量
		int blockSize;   // 请求的分包后的单个包文件大小     
		int versionSize; // 请求的版本文件大小     
		int checkSum ; // OTA CRC16 校验值
		char versionSN[33];   // OTA MD5 校验值
} OTAInfo;


typedef struct _BTCInfo {
		int deviceRegister; // 设备是否注册，1 注册，0 为注册
		int deviceID; // 设备 ID
		int userID; // 用户 ID
		int apiId; // 服务器定义的 API ID
		int deviceStatus; // 设备状态，若为 10 表示 OTA 完成升级，为 11 OTA 升级失败
		int module; // 服务器定义的模块编号
		int configBuffer[10]; // 服务器定义的 API Config 参数
		char msgId[10]; 	// 服务器的消息标志位
		char placeholder [10]; // 占位符
		char oledOneLine[20]; // oled 屏第一行的数据
		char oledSecondLine[20]; // oled 屏第二行的数据
		char oledThirdLine[20]; // oled 屏第三行的数据
	  char oledForthLine[20]; // oled 屏第四行的数据
		char mac[12];	// 设备 MAC 地址存放 Buffer

} BTCInfo;

typedef struct _EventFlag {
    bool getLatestFWFromServerFlag; // 从服务器开始获取指定的设备固件标志位

} SystemEventHandle;

extern BTCInfo btcInfo; // 大树云设备标准信息
extern char versionSN [33]; // md5 校验的值
extern int respCode; // 服务器的响应参数
extern OTAInfo otaInfo; //OTA 结构体
extern SystemEventHandle eventHandle; // 个别事件处理结构体
extern char tempBuffer[256]; // ota 升级版本信息使用的 Buffer
extern SocketInfo socketInfo; // Socket 结构体
extern struct netconn *tcpsocket; // Socket 套接字
extern ip_addr_t server_ipaddr; //  服务器的 IP 地址
extern bool server_connect_Flag; // 网络是否已连接
extern volatile bool ConnectAuthorizationFlag; // 用户是否获得服务器的验证通过
extern float x, y, z;//加速度传感器采集的值
extern int UVA_data, UVB_data; // 紫外线传感器采集的值
extern int lx;// 光强传感器采集的值
extern uint32_t persure; // 气压计采集的值
extern float temp, hum; //  温湿度传感器采集的值
extern int light_res; // 光敏电阻 ADC 采集的值
extern int red, green, blue; // rgb 灯参数
extern bool oledUserFlag;   // oled 用户使用自定义
extern bool oledSwitchFlag; // oled 用户自定义界面与初始界面切换
extern int oledUserTimer; // oled 用户使用了自定义界面开始计时
extern bool sensorTxDataFlag; // Sensor 数据推送标志位
extern bool sw1PressBtn; // SW1 按下标志位
extern bool sw2PressBtn; // SW2 按下标志位
extern int sw1Status; // SW1 状态
extern int sw2Status; // SW2 状态
extern int swTimer; // 按键界面展示时间
extern bool startSWTmr; // 开始按键展示时间计时标志位 
extern ExpEvent expEvent; // 异常处理结构体声明
extern int connectTmr; // 连接服务器计时，有响应清空，无响应 +1
