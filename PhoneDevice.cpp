#include "PhoneDevice.h"
#include "PlatformDefine.h"
#include "Encode265.h"

#include "XDes.h"

#if TEST_SAVE_FILE
#include "XRecorder.h"
CXRecorder *m_recorder = NULL;
#endif
#if TEST_4_PLAT_LIVE
CPhoneDevice::CPhoneDevice(void) :m_plMsg(sizeof(MyStruct4PhoneDeviceSession))
#else
CPhoneDevice::CPhoneDevice(void)
#endif
{

	m_param = NULL;
	m_handle = NULL;
	m_pic_in = NULL;
	m_pic_in_buff = NULL;
	m_pic_out = NULL;
	m_pBuffer = new char[2 * 1024 * 1024];


#if TEST_4_PLAT_LIVE
	m_szBuffer = new unsigned char[MAX_MSG_4_PHONE_DEVICE_SESSION];
	m_hWorkerThread = CROSS_THREAD_NULL;
	m_bLogin = FALSE;
	m_pTcpStream = NULL;
#endif



#if TEST_SAVE_FILE
	m_recorder = NULL;
#endif


}


CPhoneDevice::~CPhoneDevice(void)
{

	Stop();
	delete[] m_pBuffer;

#if TEST_4_PLAT_LIVE
	delete[] m_szBuffer;
#endif

}
void CPhoneDevice::Start(int width, int height)
{

	CROSS_TRACE("CPhoneDevice -- Start --11111\n");


#if TEST_SAVE_FILE
	if (m_recorder == NULL)
	{
		//CROSS_TRACE("CPhoneDevice -- Start -- 111111--------1\n");
		m_recorder = new CXRecorder();
		m_recorder->Start("/mnt/sdcard/", "avvv", 120, 20);
		//m_recorder->Start("/storage/emulated/0/", "avvv", 120, 64);
	}
#endif




	m_width = width;
	m_height = height;

	int m_frameRate = 30;
	//int m_bitRate = 128 * 1024;


	m_param = x265_param_alloc();
	x265_param_default(m_param);
	m_param->bRepeatHeaders = 1;//write sps,pps before keyframe  
	m_param->internalCsp = X265_CSP_I420;
	m_param->sourceWidth = width;
	m_param->sourceHeight = height;
	m_param->fpsNum = m_frameRate;
	m_param->fpsDenom = 1;


	m_param->frameNumThreads = 4;
	m_param->logLevel = X265_LOG_NONE;
	m_param->bframes = 0;
	//Init  
	m_handle = x265_encoder_open(m_param);

	int y_size = m_param->sourceWidth * m_param->sourceHeight;
	m_pic_in = x265_picture_alloc();
	x265_picture_init(m_param, m_pic_in);

	m_pic_in_buff = new char[(y_size * 3 / 2)];

	m_pic_in->planes[0] = m_pic_in_buff;
	m_pic_in->planes[1] = m_pic_in_buff + y_size;
	m_pic_in->planes[2] = m_pic_in_buff + y_size * 5 / 4;
	m_pic_in->stride[0] = width;
	m_pic_in->stride[1] = width / 2;
	m_pic_in->stride[2] = width / 2;

	m_pic_out = x265_picture_alloc();
	x265_picture_init(m_param, m_pic_out);
	

#if TEST_4_PLAT_LIVE
	if (m_hWorkerThread == CROSS_THREAD_NULL)
	{
		m_bStreamIsError = FALSE;
		m_nSendStep = NET_SEND_STEP_SLEEP;
		m_nSession = CrossGetTickCount64();
		m_bLogin = FALSE;
		m_hEvtWorkerStop = FALSE;
		m_hWorkerThread = CrossCreateThread(WorkerThread, this);
	}
#endif

}

void CPhoneDevice::Stop()
{
	CROSS_TRACE("CPhoneDevice -- Stop --\n");

#if TEST_SAVE_FILE
	if (m_recorder != NULL)
	{
		m_recorder->Stop();
		delete m_recorder;
		m_recorder = NULL;
	}
#endif

#if TEST_4_PLAT_LIVE
	if (m_hWorkerThread)
	{
		m_hEvtWorkerStop = TRUE;
		CrossWaitThread(m_hWorkerThread);
		CrossCloseThread(m_hWorkerThread);
		m_hWorkerThread = CROSS_THREAD_NULL;
		m_bLogin = FALSE;
	}
	LogoutPlat();
#endif

	//----------------------------------------------------------
	if (m_handle)
	{
		x265_encoder_close(m_handle);
		m_handle = NULL;
	}
	if (m_pic_in)
	{
		x265_picture_free(m_pic_in);
		m_pic_in = NULL;
	}

	if (m_pic_out)
	{
		x265_picture_free(m_pic_out);
		m_pic_out = NULL;
	}
	if (m_param)
	{
		x265_param_free(m_param);
		m_param = NULL;
	}

	if (m_pic_in_buff)
	{
		delete[] m_pic_in_buff;
		m_pic_in_buff = NULL;
	}

}
void CPhoneDevice::InputData(int enc265type, char * buffer, int len)
{

#if TEST_4_PLAT_LIVE
	if (!m_bLogin)
	{
		CROSS_TRACE("CPhoneDevice -- InputData -- !m_bLogin   return\n");
		return;
	}
	if (m_hEvtWorkerStop)
	{
		return;
	}
#endif
	if (!m_handle)
	{
		CROSS_TRACE("CPhoneDevice -- InputData -- m_handle = 0   return\n");
		return;
	}

	//CROSS_TRACE("CPhoneDevice -- InputData --111 len = %d\n", len);

#if TEST_4_PLAT_LIVE
	m_csMsgYUV.Lock();
	MyStruct4PhoneDeviceSession *p = (MyStruct4PhoneDeviceSession *)m_plMsg.malloc();
	memcpy(p->buffer, buffer, len);
	p->nSendLen = len;
	p->nType = enc265type;
	m_msgListYUV.push_back(p);
	m_csMsgYUV.Unlock();
	//CROSS_TRACE("CPhoneDevice -- InputData --222 len = %d\n", len);
	return;
#endif



#if TEST_SAVE_FILE
	if (enc265type == ENCODE_TYPE_VIDEO_265)
	{

		CROSS_TRACE("CPhoneDevice -- InputData --  1 \n");

		int i = 0;

		CROSS_DWORD64 dwa = CrossGetTickCount64();

		int nPicSize = m_param->sourceWidth*m_param->sourceHeight;


		char * y = (char*)m_pic_in->planes[0];
		char * v = (char*)m_pic_in->planes[1];
		char * u = (char*)m_pic_in->planes[2];

		CROSS_TRACE("CPhoneDevice -- InputData --  2 \n");

		memcpy(y, buffer, nPicSize);
		for (i = 0; i < nPicSize / 4; i++)
		{
			*(u + i) = *(buffer + nPicSize + i * 2);
			*(v + i) = *(buffer + nPicSize + i * 2 + 1);
		}
		CROSS_TRACE("CPhoneDevice -- InputData --  3 \n");

		x265_nal *pNals = NULL;
		uint32_t iNal = 0;
		m_len = 0;


		//请注意这样写的话，一会会挂掉

		CROSS_TRACE("CPhoneDevice -- InputData --  4 \n");
		int ret = x265_encoder_encode(m_handle, &pNals, &iNal, m_pic_in, m_pic_out);
		if (ret < 0)
		{
			CROSS_TRACE("CPhoneDevice -- InputData --  iNal 11111===== %d ret=%d\n", iNal, ret);
			return;
		}
		CROSS_TRACE("CPhoneDevice -- InputData --  iNal ===== %d ret=%d\n", iNal, ret);
		for (int i = 0; i < iNal; i++)
		{
			//fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
			CROSS_TRACE("CPhoneDevice -- InputData --  %d ===== %d \n", pNals[i].sizeBytes, m_len);
			memcpy(m_pBuffer + m_len, pNals[i].payload, pNals[i].sizeBytes);
			m_len += pNals[i].sizeBytes;
		}

		CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv 2 -- time = %d  %d ==>> %d \n", CrossGetTickCount64() - dwa, len, m_len);
		//CROSS_TRACE("CPhoneDevice -- InputData -- file -- 22222 \n");
		if (m_len > 0)
		{
			m_recorder->InputData(m_pBuffer, m_len, FREAM_TYPE_H265_IFRAME, 0, 15);
		}
		//
	}
#endif
}



#if TEST_4_PLAT_LIVE
int CPhoneDevice::WorkerThread(void* param)
{
	CPhoneDevice *pService = (CPhoneDevice*)param;
	pService->Worker();
	return 0;
}
//---------------------------------------------------------------------------------------
void CPhoneDevice::Worker()
{
	//CROSS_TRACE("CPhoneDevice -- Worker 1\n");
	m_bLogin = FALSE;
	while (!m_hEvtWorkerStop)// (::WaitForSingleObject(m_hEvtWorkerStop, 3 * 1000) != WAIT_OBJECT_0)
	{

		//CROSS_TRACE("CPhoneDevice -- Worker 2\n");
		//////////////////////////////////////////////////////////////////////////
		if (!m_bLogin)
		{
			//CROSS_TRACE("CPhoneDevice -- Worker 3\n");
			if (0 != LoginPlat())
			{
				//CROSS_TRACE("CPhoneDevice -- Worker 4\n");
				m_bLogin = FALSE;
				if (!m_hEvtWorkerStop)
				{
					//TRACE("Login error!\n");
					for (int i = 0; i < 30; i++)
					{
						CrossSleep(100);
						if (m_hEvtWorkerStop)
							break;
					}
					continue;
				}

			}
			else
			{
				m_bLogin = TRUE;
			}
			//CROSS_TRACE("CPhoneDevice -- Worker 5\n");
		}
		//CROSS_TRACE("CPhoneDevice -- Worker 6\n");

		//////////////////////////////////////////////////////////////////////////
		if ((CrossGetTickCount64() - m_dTickHearbeat) > 20 * 1000)
		{
			m_bLogin = FALSE;
			LogoutPlat();
		}




		//////////////////////////////////////////////////////////////////////////
		{
			//CROSS_TRACE("CPhoneDevice -- Worker -- 1111\n");


			MyStruct4PhoneDeviceSession * pYUV = NULL;
			//CROSS_TRACE("CPhoneDevice -- Worker -- 222\n");
			m_csMsgYUV.Lock();
			if (m_msgListYUV.size() <= 0)
			{
				//CROSS_TRACE("CPhoneDevice -- Worker -- 333\n");
				m_csMsgYUV.Unlock();
				CrossSleep(1);
				//CROSS_TRACE("CPhoneDevice -- Worker -- 444\n");
				continue;
			}
			//CROSS_TRACE("CPhoneDevice -- Worker -- 555\n");
			pYUV = m_msgListYUV.front();
			//CROSS_TRACE("CPhoneDevice -- Worker -- 666\n");
			m_msgListYUV.pop_front();
			//CROSS_TRACE("CPhoneDevice -- Worker -- 777\n");
			//m_plMsg.free(pYUV);
			CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv -- m_msgListYUV.size()=%d\n", m_msgListYUV.size());
			m_csMsgYUV.Unlock();

			//continue;




			if (pYUV->nType == ENCODE_TYPE_VIDEO_265)
			{
				//CROSS_TRACE("CPhoneDevice -- InputData --  1 \n");
				CROSS_DWORD64 dwa = CrossGetTickCount64();
				int i = 0;
				int nPicSize = m_param->sourceWidth*m_param->sourceHeight;
				char * y = (char*)m_pic_in->planes[0];
				char * v = (char*)m_pic_in->planes[1];
				char * u = (char*)m_pic_in->planes[2];
				//CROSS_TRACE("CPhoneDevice -- InputData --  2 \n");
				memcpy(y, pYUV->buffer, nPicSize);
				for (i = 0; i < nPicSize / 4; i++)
				{
					*(u + i) = *(pYUV->buffer + nPicSize + i * 2);
					*(v + i) = *(pYUV->buffer + nPicSize + i * 2 + 1);
				}
				//CROSS_TRACE("CPhoneDevice -- InputData --  3 \n");
				//CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv -- time = %d\n", CrossGetTickCount64() - dwa);
				x265_nal *pNals = NULL;
				uint32_t iNal = 0;

				//CROSS_TRACE("CPhoneDevice -- InputData --  4 \n");
				int ret = x265_encoder_encode(m_handle, &pNals, &iNal, m_pic_in, m_pic_out);
				if (ret < 0)
				{
					CROSS_TRACE("CPhoneDevice -- InputData --  ############################error\n");
					return;
				}

				if (iNal == 0)
				{
					continue;
				}


// 				m_len = 0;
// 				//CROSS_TRACE("CPhoneDevice -- InputData --  iNal ===== %d ret=%d\n", iNal, ret);
// 				for (int i = 0; i < iNal; i++)
// 				{
// 					//fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
// 					//CROSS_TRACE("CPhoneDevice -- InputData --  %d ===== %d \n", pNals[i].sizeBytes, m_len);
// 					memcpy(m_pBuffer + m_len, pNals[i].payload, pNals[i].sizeBytes);
// 					m_len += pNals[i].sizeBytes;
// 				}
// 				CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv 2 -- time = %d  iNal %d  == m_len %d \n", (int)(CrossGetTickCount64() - dwa), iNal, m_len);
// 				if (m_bLogin)
// 				{
// 					MyStruct4PhoneDeviceSession *p = (MyStruct4PhoneDeviceSession *)m_plMsg.malloc();
// 					p->head.magicnum = MAGIC_NUM;
// 					p->head.cmd = _X_CMD_STREAM;
// 					p->head.result = _X_CMD_RESULT_OK;
// 					p->head.session = m_nSession;
// 					p->head.datalen = m_len + sizeof(_stream_t);
// 					int k = sizeof(rqst_head) + m_len + sizeof(_stream_t);
// 					p->nSendLen = sizeof(rqst_head) + m_len + sizeof(_stream_t);
// 					_stream_t m_stream;
// 					m_stream.avType = FREAM_TYPE_H265_IFRAME;
// 					m_stream.datalen = m_len;
// 					m_stream.tick = CrossGetTickCount64();
// 					memcpy(p->buffer, &m_stream, sizeof(_stream_t));
// 					memcpy(p->buffer + sizeof(_stream_t), m_pBuffer, m_len);
// 
// 					//CROSS_TRACE("CPhoneDevice -- InputData -- -- m_len=%d  p->nSendLen=%d \n", m_len, p->nSendLen);
// 					m_csMsg.Lock();
// 
// 					// 					if (m_msgList.size()>25)
// 					// 					{
// 					// 						while (m_msgList.size() > 0)
// 					// 						{
// 					// 							MyStruct4PhoneDeviceSession * p = m_msgList.front();
// 					// 							m_msgList.pop_front();
// 					// 							m_plMsg.free(p);
// 					// 						}
// 					// 					}
// 
// 					m_msgList.push_back(p);
// 
// 					CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv -- m_msgList.size()=%d\n", m_msgList.size());
// 					m_csMsg.Unlock();
// 				}

				do 
				{
					m_len = 0;
					//CROSS_TRACE("CPhoneDevice -- InputData --  iNal ===== %d ret=%d\n", iNal, ret);
					for (int i = 0; i < iNal; i++)
					{
						//fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
						//CROSS_TRACE("CPhoneDevice -- InputData --  %d ===== %d \n", pNals[i].sizeBytes, m_len);
						memcpy(m_pBuffer + m_len, pNals[i].payload, pNals[i].sizeBytes);
						m_len += pNals[i].sizeBytes;
					}
					CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv 2 -- time = %d   ==>> %d \n", (int)(CrossGetTickCount64() - dwa), m_len);

					if (m_bLogin)
					{
						MyStruct4PhoneDeviceSession *p = (MyStruct4PhoneDeviceSession *)m_plMsg.malloc();
						p->head.magicnum = MAGIC_NUM;
						p->head.cmd = _X_CMD_STREAM;
						p->head.result = _X_CMD_RESULT_OK;
						p->head.session = m_nSession;
						p->head.datalen = m_len + sizeof(_stream_t);
						int k = sizeof(rqst_head) + m_len + sizeof(_stream_t);
						p->nSendLen = sizeof(rqst_head) + m_len + sizeof(_stream_t);
						_stream_t m_stream;
						m_stream.avType = FREAM_TYPE_H265_IFRAME;
						m_stream.datalen = m_len;
						m_stream.tick = CrossGetTickCount64();
						memcpy(p->buffer, &m_stream, sizeof(_stream_t));
						memcpy(p->buffer + sizeof(_stream_t), m_pBuffer, m_len);

						//CROSS_TRACE("CPhoneDevice -- InputData -- -- m_len=%d  p->nSendLen=%d \n", m_len, p->nSendLen);
						m_csMsg.Lock();

						// 					if (m_msgList.size()>25)
						// 					{
						// 						while (m_msgList.size() > 0)
						// 						{
						// 							MyStruct4PhoneDeviceSession * p = m_msgList.front();
						// 							m_msgList.pop_front();
						// 							m_plMsg.free(p);
						// 						}
						// 					}

						m_msgList.push_back(p);

						CROSS_TRACE("CPhoneDevice -- InputData -- yuvyuv -- m_msgList.size()=%d\n", m_msgList.size());
						m_csMsg.Unlock();
					}



					int ret = x265_encoder_encode(m_handle, &pNals, &iNal, NULL, NULL);
					if (ret < 0)
					{
						CROSS_TRACE("CPhoneDevice -- InputData --  ############################error\n");
						return;
					}

					if (ret == 0)
					{
						break;
					}

				} while (ret>0);


				
			}

			m_plMsg.free(pYUV);
		}

		//////////////////////////////////////////////////////////////////////////
		//CrossSleep(10);

	}
}





















void CPhoneDevice::DeleteStreamData()
{
	m_pTcpStream = NULL;

	//////////////////////////////////////////////////////////////////////////
	m_csMsg.Lock();
	while (m_msgList.size() > 0)
	{
		MyStruct4PhoneDeviceSession * p = m_msgList.front();
		m_msgList.pop_front();
		m_plMsg.free(p);
	}
	m_csMsg.Unlock();

	//////////////////////////////////////////////////////////////////////////
	m_csMsgYUV.Lock();
	while (m_msgList.size() > 0)
	{
		MyStruct4PhoneDeviceSession * p = m_msgListYUV.front();
		m_msgListYUV.pop_front();
		m_plMsg.free(p);
	}
	m_csMsgYUV.Unlock();

}
void CPhoneDevice::OnPacketCompleteNetStreamData(int32_t dwCompleteBytes, int32_t bRecieve)
{
	if (NULL == m_pTcpStream)
	{
		return;
	}
	if (m_bStreamIsError)
	{
		return;
	}
	////网络出错，用户断开连接
	if (dwCompleteBytes < 0)
	{
		m_bStreamIsError = TRUE;
		return;
	}

	if (bRecieve)
	{
		//CROSS_TRACE("CPhoneDevice -- OnPacketCompleteNetStreamData -- recv -- \n");
		if (0 == dwCompleteBytes)
		{
			m_nRcvStep = NET_RECIEVE_STEP_HEAD;
			m_pTcpStream->AsyncRead(&m_head, sizeof(m_head));
			return;
		}
		else
		{
			if (m_nRcvStep == NET_RECIEVE_STEP_HEAD)
			{
				m_dataLen = m_head.datalen;
				if (m_dataLen > MAX_MSG_4_PHONE_DEVICE_SESSION)
				{
					m_bStreamIsError = TRUE;
					return;
				}
				if (m_dataLen == 0)
				{
					DoMsg();
					m_nRcvStep = NET_RECIEVE_STEP_HEAD;
					m_pTcpStream->AsyncRead(&m_head, sizeof(m_head));
				}
				else
				{
					m_nRcvStep = NET_RECIEVE_STEP_BODY;
					m_pTcpStream->AsyncRead(m_szBuffer, m_dataLen);
				}
				return;
			}
			if (m_nRcvStep == NET_RECIEVE_STEP_BODY)
			{
				DoMsg();
				m_nRcvStep = NET_RECIEVE_STEP_HEAD;
				m_pTcpStream->AsyncRead(&m_head, sizeof(m_head));
				return;
			}
		}
	}
	else
	{
		//CROSS_TRACE("CPhoneDevice -- OnPacketCompleteNetStreamData -- send -- \n");
		////////////////////////////////////////////////////////////////////////
		if (NET_SEND_STEP_SLEEP == m_nSendStep)
		{
			if (m_msgList.size() > 0)
			{
				m_nSendStep = NET_SEND_STEP_SEND;
				m_pTcpStream->AsyncWrite(m_msgList.front(), ((MyStruct4PhoneDeviceSession *)m_msgList.front())->nSendLen);
				return;
			}
			else
			{
				m_pTcpStream->PostDelayWriteStatus();//强制调用一次发送回调
				return;
			}
		}
		//////////////////////////////////////////////////////////////////////////
		if (NET_SEND_STEP_SEND == m_nSendStep)
		{
			m_nSendStep = NET_SEND_STEP_SLEEP;
			m_csMsg.Lock();
			MyStruct4PhoneDeviceSession * p = m_msgList.front();
			m_msgList.pop_front();
			m_plMsg.free(p);
			m_csMsg.Unlock();

			CROSS_TRACE("CPhoneDevice -- send Data -- -- \n");

			if (m_msgList.size() > 0)
			{
				m_nSendStep = NET_SEND_STEP_SEND;
				m_pTcpStream->AsyncWrite(m_msgList.front(), ((MyStruct4PhoneDeviceSession *)m_msgList.front())->nSendLen);
				return;
			}
			else
			{
				m_pTcpStream->PostDelayWriteStatus();//强制调用一次发送回调
				return;
			}
		}
	}
}

void CPhoneDevice::DoMsg()
{
	m_dTickHearbeat = CrossGetTickCount64();
}

int CPhoneDevice::LoginPlat()
{

	CROSS_TRACE("CPhoneDevice -- Login \n");
	MyStruct4PhoneDeviceSession * pSendmsg = new MyStruct4PhoneDeviceSession;

	MyStruct4PhoneDeviceSession *pRecvmsg = new MyStruct4PhoneDeviceSession;
	do
	{
		CROSS_TRACE("CPhoneDevice -- Login 1\n");
		m_pTcpStream = XNetCreateStream4Connect("192.168.2.120", 8002, 10);//23.99.10
		if (0 != XNetConnectStream(m_pTcpStream))
		{
			CROSS_TRACE("CPhoneDevice -- connect error\n");
			break;
		}
		CROSS_TRACE("CPhoneDevice -- Login 2\n");
		//
		pSendmsg->head.magicnum = MAGIC_NUM;
		pSendmsg->head.result = _X_CMD_RESULT_OK;
		pSendmsg->head.cmd = _X_CMD_LOGIN;
		pSendmsg->head.datalen = sizeof(_login_t);

		//
		_login_t login;
		memset(&login, 0, sizeof(login));

		char dest[256] = { 0 };
		XDESEncode("19544", 19544, dest);
		memcpy(login.pwd, dest, strlen(dest));
		//
		memcpy(pSendmsg->buffer, &login, sizeof(_login_t));
		//
		pSendmsg->nSendLen = sizeof(_head_t) + sizeof(_login_t);

		//AAAAAAAA:发送Serial给平台，平台将返回请求注册信息，要求KEY//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		if (0 != m_pTcpStream->SyncWriteAndRead(pSendmsg, pSendmsg->nSendLen, pRecvmsg, sizeof(_head_t)))
		{
			CROSS_TRACE("CPhoneDevice -- connect error 1\n");
			break;
		}
		if (_X_CMD_LOGIN != pRecvmsg->head.cmd)
		{
			CROSS_TRACE("CPhoneDevice -- connect error 2\n");
			break;
		}
		CROSS_TRACE("CPhoneDevice -- Login 3\n");
		//BBBBBBBB：发给平台Session KEY，并接收验证平台的注册//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		pSendmsg->head.magicnum = MAGIC_NUM;
		pSendmsg->head.result = _X_CMD_RESULT_OK;
		pSendmsg->head.cmd = _X_CMD_LOGIN_KEY;
		pSendmsg->head.session = m_nSession;
		pSendmsg->head.datalen = 0;
		if (0 != m_pTcpStream->SyncWriteAndRead(pSendmsg, sizeof(_head_t), pRecvmsg, sizeof(_head_t) + sizeof(_login_t)))
		{
			CROSS_TRACE("CPhoneDevice -- connect error 3\n");
			break;
		}
		if (_X_CMD_LOGIN != pRecvmsg->head.cmd)
		{
			CROSS_TRACE("CPhoneDevice -- connect error 4\n");
			break;
		}
		CROSS_TRACE("CPhoneDevice -- Login 4\n");
		//
		//验证KEY，暂时略过
		//
		//CCCCCCCC：验证平台的注册,返回成功//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		pSendmsg->head.magicnum = MAGIC_NUM;
		pSendmsg->head.result = _X_CMD_RESULT_OK;
		pSendmsg->head.cmd = _X_CMD_LOGIN;
		pSendmsg->head.session = m_nSession;
		pSendmsg->head.datalen = 0;
		if (0 != m_pTcpStream->SyncWriteAndRead(pSendmsg, sizeof(_head_t), 0, 0))
		{
			CROSS_TRACE("CPhoneDevice -- connect error 5\n");
			break;
		}

		CROSS_TRACE("CPhoneDevice -- connect okokokokokok\n");
		//////////////////////////////////////////////////////////////////////////
		m_dTickHearbeat = CrossGetTickCount64();
		m_pTcpStream->SetStreamData(this);
		delete pSendmsg;
		delete pRecvmsg;
		return 0;
		//////////////////////////////////////////////////////////////////////////


	} while (0);

	if (m_pTcpStream)
	{
		m_pTcpStream->Release();
		m_pTcpStream = NULL;//这里没有SetStreamData，所以不能do...while
	}

	delete pSendmsg;
	delete pRecvmsg;
	return -1;
}
int CPhoneDevice::LogoutPlat()
{
	if (m_pTcpStream)
	{
		m_pTcpStream->Release();
		do
		{
			CrossSleep(1);
		} while (m_pTcpStream);
	}
}

#endif