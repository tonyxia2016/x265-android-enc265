#ifndef PHONE_DEVICE_X_456W3432424324
#define PHONE_DEVICE_X_456W3432424324
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <x265.h>



#define TEST_4_PLAT_LIVE 1

#if TEST_4_PLAT_LIVE
#include "PlatformDefine.h"
#include "XCross.h"




#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/pool/pool.hpp>





#pragma pack(1)

typedef struct _stream_t
{
	unsigned int avType;//emFREAM_TYPE_DEFINE
	unsigned int datalen;
	long long seq;
	long long tick;//设备需要使用硬件时间保证该数值一直是递增的
	unsigned int reserve[4];
}streaminfo_t;

#define MAGIC_NUM 0XAA55

enum _X_CMD_DEFINE
{
	_X_CMD_LOGIN = 0,					//登陆
	_X_CMD_LOGIN_KEY,					//key s -> c
	_X_CMD_HEARBEAT,					//心跳
	_X_CMD_LOGOUT,						//注销
	_X_CMD_OPEN_MAIN_STREAM,	//打开实时流
	_X_CMD_CLOSE_MAIN_STREAM,	//关闭实时流
	_X_CMD_OPEN_SUB_STREAM,		//打开实时流(子码流)
	_X_CMD_CLOSE_SUB_STREAM,	//关闭实时流(子码流)
	_X_CMD_STREAM,//表示这是流数据
	_X_CMD_ALARM_UPLOAD = 12,
	_X_CMD_SET_PTZCONTROL = 43,
	_X_CMD_TRANS = 48,//透明传输


};

enum _X_COMPANY_DEFINE
{
	_X_COMPANY_MYCOM = 0,
	_X_COMPANY_HIK,
	_X_COMPANY_DH,
	_X_COMPANY_ZYX,
	_X_COMPANY_TST,
	_X_COMPANY_JYLP2P,
};

enum _X_CMD_RESULT
{
	_X_CMD_RESULT_OK = 0,
	_X_CMD_RESULT_ERROR
};

typedef struct _head_t
{
	unsigned int magicnum;
	unsigned int cmd;		//_X_CMD_DEFINE
	unsigned int session;
	unsigned int seq;
	unsigned int result;	//_X_CMD_RESULT_OK 服务器响应结果，客户不需要填充
	unsigned int datalen;
	unsigned int dataextern; //数量，当cmd为_X_CMD_GET_AREA_LIST_RESPONSE，_X_CMD_GET_DEVICE_LIST_RESPONSE，_X_CMD_GET_STORE_STRATEGY_RESPONSE
	unsigned int reserve[9];
}rqst_head;//request head

struct _login_t
{
	char user[64];
	char pwd[128]; // 使用des加密
};

//-------------------------------------------------
enum _PTZ_CMD_DEFINE
{
	PTZ_CMD_START = 0,
	PTZ_CMD_UP,
	PTZ_CMD_DOWN,
	PTZ_CMD_LEFT,
	PTZ_CMD_RIGHT,
	PTZ_CMD_LEFTUP,
	PTZ_CMD_LEFTDOWN,
	PTZ_CMD_RIGHTUP,
	PTZ_CMD_RIGHTDOWN,
	PTZ_CMD_STOP,
	PTZ_CMD_SETPRESET,
	PTZ_CMD_CLEARPRESET,
	PTZ_CMD_CALLPRESET,
	PTZ_CMD_PRESETSCAN,
	PTZ_CMD_LEVELAUTOSCAN,
	PTZ_CMD_VERTICALAUTOSCAN,
	PTZ_CMD_END,
};

typedef struct _ptz_control_t
{
	unsigned int cmd;
	unsigned int data;
	unsigned int reserve[8];
}ptzcontrol_t;

typedef enum {
	ALARM_MOTION_DETECT, ///< Motion detection alarm.
	ALARM_ALARMIN_TRIG, ///< External trigger alarm.
	ALARM_MAX ///<  Alarm manager command number.
} _X_ALARM_TYPE;
typedef struct _alarm_upload_t
{
	unsigned int alarmtype;
	unsigned int reserve;
}alarmupload_t;
//-------------------------------------------------

#pragma pack()




#define MAX_MSG_4_PHONE_DEVICE_SESSION (2*1024*1024)
struct MyStruct4PhoneDeviceSession
{
	rqst_head head;
	char buffer[MAX_MSG_4_PHONE_DEVICE_SESSION - sizeof(rqst_head) - sizeof(int) - sizeof(int)];
	int nSendLen;
	int nType;//emEnc264Type
};
enum emPhoneDeviceNetSendStep
{
	NET_SEND_STEP_SLEEP = 0,
	NET_SEND_STEP_SEND
};

#endif

#if TEST_4_PLAT_LIVE
class CPhoneDevice :public CXNetStreamData
#else
class CPhoneDevice
#endif
{
public:
	CPhoneDevice(void);
	~CPhoneDevice(void);


	//////////////////////////////////////////////////////////////////////////
public:
	void Start(int width ,int height);
	void Stop();
	void InputData(int enc265type, char * buffer, int len);


private:
	x265_param * m_param;
	x265_encoder* m_handle;
	x265_picture * m_pic_in;
	char *m_pic_in_buff;

	x265_picture * m_pic_out;
// 	x265_nal_t  *m_nal;
// 
// 	x265_picture_t *m_pic_out;



	long long i_pts;


	int m_width, m_height;



	char *m_pBuffer;
	int m_len;




#if TEST_4_PLAT_LIVE
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void DeleteStreamData();
	virtual void OnPacketCompleteNetStreamData(int32_t dwCompleteBytes, int32_t bRecieve);
private:
	int LoginPlat();
	int LogoutPlat();
	void DoMsg();
	//
	//long long m_videotick;
	BOOL m_bLogin;
	BOOL m_bStreamIsError;
	int m_nSession;
	//
	boost::pool<> m_plMsg;//内存池，消息链表
	//
	emNetRecieveStep m_nRcvStep;
	rqst_head m_head;
	int m_dataLen;
	unsigned char * m_szBuffer;
	//
	CXNetStream* m_pTcpStream;
	CrossCriticalSection m_csTcp;
	//list<boost::shared_ptr<CStreameFrame>> m_frameList;
	list<MyStruct4PhoneDeviceSession *> m_msgList;
	CrossCriticalSection m_csMsg;
	//
	list<MyStruct4PhoneDeviceSession *> m_msgListYUV;
	CrossCriticalSection m_csMsgYUV;
	emPhoneDeviceNetSendStep m_nSendStep;
	static int WorkerThread(void*);
	void Worker();
	CROSS_THREAD_HANDLE	m_hWorkerThread;
	BOOL		m_hEvtWorkerStop;

	CROSS_DWORD64 m_dTickHearbeat;

private:



	

#endif




};

#endif //PHONE_DEVICE_X_456W3432424324