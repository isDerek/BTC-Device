
#define API_AUTH_Sendpack "{\"apiId\":%d,\"versionSN\":\"%s\",\"mac\":\"%02x%02x%02x%02x%02x%02x\",\"userId\":%d,\"deviceId\":%d}"		//cloud api
#define API_Heartpack_Sendpack "{\"apiId\":%d}"
#define API_SenSorData_Sendpack "{\"apiId\":%d,\"userId\":%d,\"deviceId\":%d,\"sensorData\":{\"temp\":%d,\"humidity\":%d,\"lightRes\":%d,\"uva\":%d,\"uvb\":%d,\"luxInt\":%d}}"
#define API_SendData_Response "{\"apiId\":%d,\"respCode\":%d,\"msgId\":\"%s\"}"
#define API_SendKeyData_Sendpack "{\"apiId\":%d,\"userId\":%d,\"deviceId\":%d,\"key\":{\"first\":%d,\"second\":%d}}"

#define NOTIFY_REQ_updateVersion     "{\"apiId\":%d,\"versionSN\":\"%s\",\"blockOffset\":%d,\"blockSize\":%d}"
#define CMD_RESP_otaUpdate           "{\"msgId\":\"%s\",\"apiId\":%d,\"respCode\":%d}"
#define	NOTIFY_REQ_otaDeviceStatus	 "{\"apiId\":%d,\"deviceStatus\":%d}"	

//#define API_AUTH_mac "be454f62e720"  //test
#define API_AUTH_mac "002e20020019"  //test
#define API_AUTH_reconnect0 0
#define API_AUTH_reconnect1 1


#define ERR_Code 1
#define ERR_Connect 2
#define ERR_Packet 3
#define ERR_Device 4
#define ERR_Server 5
#define ERR_Success 100

//  Device 推送数据的 API
enum API_SEND
{
	API_SEND_AUTH = 1U,
	API_SEND_Heartpack,
	API_SEND_DATA,
	API_SEND_OTA,
};
// Device 发送数据的响应 API
enum API_RES
{
    API_RES_AUTH = 1U, 
		API_RES_Heartpack,  
		API_RES_SendData,	              	 
	  API_RES_OTA,
		API_RES_SERVER_SEND,
		
};
enum API_module
{

    API_module_pwm = 2U,      
    API_module_rgb = 3U,      
    API_module_oled = 4U,      

};


