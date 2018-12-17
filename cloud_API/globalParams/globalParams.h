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
		int apiId;
		char msgId[10]; 
		char mac[12];
		int configBuffer[10];
		char oledOneLine[16];
		char oledSecondLine[16];
		char oledThirdLine[16];
	  char oledForthLine[16];
		int module;
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
extern SocketInfo socketInfo;
extern struct netconn *tcpsocket;
extern ip_addr_t server_ipaddr;

extern bool server_connect_Flag;
extern volatile bool ConnectAuthorizationFlag;
/***************/
extern float x, y, z;//加速度
extern uint16_t UVA_data, UVB_data;
extern float lx;//光亮等级
extern uint32_t persure;
extern float temp, hum;

extern int red, green, blue;

