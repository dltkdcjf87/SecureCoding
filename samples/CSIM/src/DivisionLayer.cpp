// SessionLayer.cpp: implementation of the CDivisionLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "VersionHistory.h"
#include "DivisionLayer.h"

#include "Global.h"

#include "Utility.h"
#include "Logger.h"
#include "XBusDef3.h"

//##########################################################//
//##########################################################//
//##                                                      ##//
//##  CServiceLayer CMMCLayer CStatisticLayer   표현층    ##//
//##         ^                                            ##//
//##         |                                            ##//
//##         |                                            ##//
//##  CDivisionLayer                            세션층    ##//
//##  ~~~~~~~~~~~~~~                                      ##//
//##         ^                                            ##//
//##         |                                            ##//
//##  XBus --+                                  전송층    ##//
//##                                                      ##//
//##########################################################//
//##########################################################//

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDivisionLayer& theDivisionLayer(void)
{
	static CDivisionLayer divisionLayer;

	return divisionLayer;
}


CDivisionLayer::CDivisionLayer() : m_bInit(false)
{
}

CDivisionLayer::~CDivisionLayer()
{
}

bool CDivisionLayer::initialize(void)
{
	int nResult;

	if(m_bInit == true) {
		LOGGER(TRACE_INFO, "devision layer is already initialized.");
		return true;
	}

	int nIdx = GetPrivateProfileInt("SERVER", "EMS", 0, GLOBAL_CONFIG_FILE);

	//## XBUS 초기화 및 콜백함수 등록 ##//
	if(nIdx == 0) {
		nResult = init_btxbus(CSIM_A, CDivisionLayer::eventCallBack_XBus, CDivisionLayer::eventCallBack_MBus);
	} else if(nIdx == 1) {
		nResult = init_btxbus(CSIM_B, CDivisionLayer::eventCallBack_XBus, CDivisionLayer::eventCallBack_MBus);
	} else {
		LOGGER(TRACE_ERROR, "regist fail the event receive callback function : SYSTEM ID is not 0 or 1.");
		return false;
	}

	set_module_version(version_num[0], version_num[1], version_num[2], version_date, version_user, version_dest);

	if(nResult != 0) {
		LOGGER(TRACE_ERROR, "regist fail the event receive callback function. : %d", nResult);
		return false;
	}

	m_bInit = true;


	return true;
}

void CDivisionLayer::uninitialize(void)
{
	m_bInit = false;
}


bool CDivisionLayer::findCallBack(unsigned short int nID, DivisionCallBack& pFunc)
{
	CALLBACK_MAP::iterator lt;

	lt = m_mapCallBack.find(nID);
	if(lt == m_mapCallBack.end()) return false;

	pFunc = (*lt).second;

	return true;
}

void CDivisionLayer::setCallBack(unsigned short int nID, DivisionCallBack pFunc)
{
	m_mapCallBack[nID] = pFunc;
}


//##########################################################
//##########################################################
//##*           EVENT CALL BACK FUNCTION DEFINE          ##*
//##########################################################
//##########################################################

//## 일반 통신용 BUS ##//
int CDivisionLayer::eventCallBack_MBus(int nLength, u_pchar pData)
{
	DivisionCallBack pCallBack;
	unsigned short int nID;
	u_char nFrom;

	XBUS_MSG_HDR* pHead = (XBUS_MSG_HDR*)pData;

	pData += sizeof(XBUS_MSG_HDR);
	nLength -= sizeof(XBUS_MSG_HDR);

	nID = (*pHead).MsgId;
	nFrom = (*pHead).From;

	CALLBACK_MAP::iterator lt;

	if(theDivisionLayer().findCallBack(nID, pCallBack) == false) {
		LOGGER(TRACE_WARNNING, "division layer : unknow id(%d)", nID);
		return false;
	}
	if(!pCallBack) {
		LOGGER(TRACE_WARNNING, "division layer : function is null", nID);
		return 0;
	}

	LOGGER(TRACE_LEVEL1, "division layer : receive mbus id(0x%x)", nID);

	return pCallBack(nLength, nID, nFrom, pData);
}

//## 이중화 BUS ##//
int CDivisionLayer::eventCallBack_XBus(int nLength, u_pchar pData)
{
	DivisionCallBack pCallBack;
	unsigned short int nID;
	u_char nFrom;

	XBUS_MSG_HDR* pHead = (XBUS_MSG_HDR*)pData;

	pData += sizeof(XBUS_MSG_HDR);
	nLength -= sizeof(XBUS_MSG_HDR);

	nID = (*pHead).MsgId;
	nFrom = (*pHead).From;

	LOGGER(TRACE_LEVEL1, "division layer : receive xbus id(0x%x)", nID);
	return 0;
}

unsigned char CDivisionLayer::isActiveModule(unsigned short proc_id, unsigned char self_id)
{
//	return get_module_state(proc_id, self_id);
	return true;
}

int CDivisionLayer::send(unsigned short length, unsigned char to_id, unsigned short message_id, unsigned char* pMessage)
{
	int nRet = 0;

	if(isActiveModule(to_id, 0) == 0x00) {
		LOGGER(TRACE_WARNNING, "send fail : UNACCESSIBLE : %d", to_id);
		return 1;
	}

	nRet = msendsig(length, to_id, message_id, pMessage);

	if(nRet != 0) {
		LOGGER(TRACE_ERROR, "msendsig() result : %d.", nRet);
	}

	return nRet;
}


int CDivisionLayer::ssend(unsigned short length, unsigned short message_id, unsigned char* pMessage)
{
	int nRet = 0;

	nRet = ssendsig(length, message_id, pMessage);

	if(nRet != 0) {
		LOGGER(TRACE_ERROR, "ssendsig() result : %d.", nRet);
	}

	return nRet;
}
#if !defined(XBUS)

#include "Common/SessionUtility.h"
using namespace chiwoo;

CALLBACK_XBUS g_xBusFunc;
CALLBACK_XBUS g_mBusFunc;

u_long WINAPI xbusEventThread(LPVOID param);

int init_btxbus(unsigned char moId, CALLBACK_XBUS sbus, CALLBACK_XBUS mbus)
{
	THREAD_HANDLE _hThread;
	u_long ulThreadID = 0;

	g_xBusFunc = sbus;
	g_mBusFunc = mbus;

	_CreateThread(_hThread, NULL, 0, (LPTHREAD_START_ROUTINE) xbusEventThread, (LPVOID)null, 0, &ulThreadID);

	return 0;
}

int  set_module_version(char a, char b, char c, char *Date, char *Owner, char *Description)
{
	return 0;
}


int reg_callback(unsigned char cb_id, xbHaCB_t function)
{
	if(cb_id == HA_Active) function(0);
	return 0;
}
// Module State Chanage Call Back
int reg_mod_state_change(xbMoSt_t function)
{
	return 0;
}

unsigned char get_module_state(unsigned short proc_id, unsigned char self_id)
{
	return 1;
}

int ssendsig(unsigned short length, unsigned short msg_id, unsigned char *msg_ptr)
{
	return 0;
}

int msendsig(unsigned short length, unsigned char to_id, unsigned short msg_id, unsigned char *msg_ptr)
{
	unsigned char szTemp[1024];
	XBUS_MSG_HDR* pHead;
	unsigned char* pData;

	if(to_id != SRM_00) return 0;

	pHead = (XBUS_MSG_HDR*)szTemp;
	pData = szTemp + sizeof(XBUS_MSG_HDR);

	memcpy((*pHead).version, "XBusV2", 6);
	(*pHead).From = SRM_00;
	(*pHead).To = to_id;
	(*pHead).Self = 0;
	(*pHead).Length = length + sizeof(XBUS_MSG_HDR);
	(*pHead).MsgId = msg_id;

	memcpy(pData, msg_ptr, length);

	SessionStream stream;

	stream.set("127.0.0.1", 5100, sessionTypeUDP);
	stream.connect();

	IFString ifString('\1', '\2', 6, 1024);

	ifString.append((*pHead).Length, (char*)szTemp);

	stream.send(&ifString);

	stream.disconnect();

	return 0;
//	return CDivisionLayer::eventCallBack_MBus();
}

u_long WINAPI xbusEventThread(LPVOID param)
{
/*
	SessionServer server(sessionTypeUDP);	
	char  szRcvIP[20];
	char* pRcvData;
	int iRcvLen;

	pRcvData = new char[1024];

	server.Using(null, 5100);

	while(true)
	{
		iRcvLen = server.waitReceiver(pRcvData, 1024, szRcvIP);
		if(iRcvLen > 0) {
			pRcvData[iRcvLen] = null;
			g_mBusFunc(iRcvLen, (unsigned char*)pRcvData);
		} else break;

	}

	delete [] pRcvData;
*/
	return 0;
}

#endif

