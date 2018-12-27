// 设备第一次请求连接，判断设备是否注册，且是否通过服务区校验
#define API_AUTH_Sendpack "{\"apiId\":%d,\"versionSN\":\"%s\",\"mac\":\"%02x%02x%02x%02x%02x%02x\",\"userId\":%d,\"deviceId\":%d}"		//cloud api
// 设备向服务器发送心跳包
#define API_Heartpack_Sendpack "{\"apiId\":%d}"
// 设备发送传感器数据
#define API_SenSorData_Sendpack "{\"apiId\":%d,\"userId\":%d,\"deviceId\":%d,\"sensorData\":{\"temp\":%d,\"humidity\":%d,\"lightRes\":%d,\"uva\":%d,\"uvb\":%d,\"luxInt\":%d}}"
// 设备向服务器响应是否成功接收到设备信息的消息
#define API_SendData_Response "{\"apiId\":%d,\"respCode\":%d,\"msgId\":\"%s\"}"
// 设备发送按键状态
#define API_SendKeyData_Sendpack "{\"apiId\":%d,\"userId\":%d,\"deviceId\":%d,\"key\":{\"first\":%d,\"second\":%d}}"
// 设备向服务器请求分包后的的那个包的消息
#define NOTIFY_REQ_updateVersion     "{\"apiId\":%d,\"versionSN\":\"%s\",\"blockOffset\":%d,\"blockSize\":%d}"
// 设备向服务器响应是否接收到了服务器的版本请求消息
#define CMD_RESP_otaUpdate           "{\"msgId\":\"%s\",\"apiId\":%d,\"respCode\":%d}"
// 设备向服务器发送 OTA 升级是否完成的消息， deviceStatus : 10 完成升级，11 升级失败
#define	NOTIFY_REQ_otaDeviceStatus	 "{\"apiId\":%d,\"deviceStatus\":%d}"	


#define ERR_Code 1	// 代码有未知错误
#define ERR_Connect 2	// 连接出错
#define ERR_Packet 3	// 包出错
#define ERR_Device 4	// 设备出错
#define ERR_Server 5	// 服务器出错
#define ERR_Success 100  // 成功标志位

//  Device 推送数据的 API ID
enum API_SEND
{
	API_SEND_AUTH = 1U,
	API_SEND_Heartpack,
	API_SEND_DATA,
	API_SEND_OTA,
};
// Device 发送数据的响应 API ID
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

    API_module_pwm = 2U,     // 服务器定义的 PWM Moudle
    API_module_rgb = 3U,     // 服务器定义的 RGB Moudle
    API_module_oled = 4U,    // 服务器定义的 OLED Moudle
		API_module_ota = 100U,   // 服务器定义的 OTA Moudle

};


