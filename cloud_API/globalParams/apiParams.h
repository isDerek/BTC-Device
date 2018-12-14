
/* PACKET */
#define API_AUTH 1
#define API_SendData 2
#define API_Heartpack 3
#define API_PushDevice 4
#define API_OTA 5
#define API_AUTH_Sendpack "{\"apiId\":%d,\"versionSN\":\"%s\",\"mac\":\"%s\",\"reconnect\":%d}"					
#define API_SendData_Response "{\"apiId\":%d,\"respCode\":%d,\"msgId\":\"%s\"}"
#define API_SendData_Response_FloatData "{\"apiId\":%d,\"respCode\":%d,\"result\":[%4.2f],\"msgId\":\"%s\"}"
#define API_SendData_Response_IntData "{\"apiId\":%d,\"respCode\":%d,\"result\":[%d],\"msgId\":\"%s\"}"
#define API_SendData_Response_TwoData "{\"apiId\":%d,\"respCode\":%d,\"result\":[%d,%d],\"msgId\":\"%s\"}"
#define API_SendData_Response_ThreeData "{\"apiId\":%d,\"respCode\":%d,\"result\":[%3.2f,%3.2f,%3.2f],\"msgId\":\"%s\"}"
#define API_Heartpack_Response "{\"apiId\":%d}"
#define API_PushDevice_Response "{\"apiId\":%d,\"deviceStatus\":%d,\"portStatus\":[%d,%d]}"
#define API_OTA_Response "{\"apiId\":%d,\"versionSN\":%s,\"blockOffset\":%d,\"blockSize\":%d}"
#define API_AUTH_mac "be454f62e720"
#define API_AUTH_reconnect0 0
#define API_AUTH_reconnect1 1
//#define MODULE_PORTS 1
//#define MODULE_PWM 2
//#define MODULE_RGB 3
//#define MODULE_OLED 4
//#define MODULE_ACCEL 10
//#define MODULE_LIGHT 11
//#define MODULE_UV 12
//#define MODULE_TempHumi 13
//#define MODULE_PRESSURE 14
//#define MODULE_BUTTON 15
//#define MODULE_OTA 100

#define ERR_Code 1
#define ERR_Connect 2
#define ERR_Packet 3
#define ERR_Device 4
#define ERR_Server 5
#define ERR_Success 100

enum API_apiId
{
    API_apiId_AUTH = 1U, 
    API_apiId_SendData,      
    API_apiId_Heartpack,      
    API_apiId_PushDevice,      
    API_apiId_OTA,      
};
enum API_module
{
    API_module_port = 1U, 
    API_module_pwm = 2U,      
    API_module_rgb = 3U,      
    API_module_oled = 4U,      
    API_module_accel = 10U,
		API_module_light = 11U,
		API_module_uv = 12U,
		API_module_temphumi = 13U,
		API_module_pressure = 14U,
		API_module_button = 15U,
		API_module_ota = 100U,
};
