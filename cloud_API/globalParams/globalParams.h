#include "ip_addr.h"
#include "cJSON.h"
#define SOCKET_OUT_BUFFER_SIZE 256
#define SOCKET_IN_BUFFER_SIZE (4096+256)


typedef struct _SocketInfo {
    char inBuffer[SOCKET_IN_BUFFER_SIZE];
    char outBuffer[SOCKET_OUT_BUFFER_SIZE];
} SocketInfo;

typedef struct _OTAInfo {
	  int blockOffset;      
		int blockSize;        
		int versionSize;      
		int checkSum ;
		char versionSN[33];   
} OTAInfo;


typedef struct _BTCInfo {
		int deviceRegister;
		int deviceID;
		int userID;
		int apiId;
		int deviceStatus;
		int module;
		int configBuffer[10];
		char msgId[10]; 	
		char placeholder [10]; // 占位符
		char oledOneLine[20];
		char oledSecondLine[20];
		char oledThirdLine[20];
	  char oledForthLine[20];
		char mac[12];	

} BTCInfo;

typedef struct _EventFlag {
    bool heatbeatFlag;
    bool getLatestFWFromServerFlag; 
	  bool firstConnectFlag ; 
		bool downLoadFinshFWFromServerFlag;

} SystemEventHandle;

extern BTCInfo btcInfo;
extern char versionSN [33];
extern int respCode;
extern OTAInfo otaInfo;
extern SystemEventHandle eventHandle;
extern char tempBuffer[256];
extern SocketInfo socketInfo;
extern struct netconn *tcpsocket;
extern ip_addr_t server_ipaddr;

extern bool server_connect_Flag;
extern volatile bool ConnectAuthorizationFlag;
/***************/
extern float x, y, z;//加速度
extern int UVA_data, UVB_data;
extern int lx;//光亮等级
extern uint32_t persure;
extern float temp, hum;
extern int light_res;
extern int red, green, blue;

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
