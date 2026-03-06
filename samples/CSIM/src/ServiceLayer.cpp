// ServiceLayer.cpp: implementation of the CServiceLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "DivisionLayer.h"
#include "ServiceLayer.h"
#include "SRManager.h"

#include "QueueManager.h"
#include "VersionInfoManager.h"

#include "Global.h"

#include "XBusDef3.h"
#include "Logger.h"
#include "Utility.h"

#include "SGMDef.h"
#include "CSIMDef.h"

//##########################################################//
//##########################################################//
//##                                                      ##//
//##                                                      ##//
//##                                                      ##//
//##   SChannelMgr                               ÀÀ¿ëÃþ   ##//
//##         ^                                            ##//
//##         |                                            ##//
//##  CServiceLayer  CMMLayer  CStatisticLayer  Ç¥ÇöÃþ   ##//
//##  ~~~~~~~~~~~~~                                       ##//
//##         ^                                            ##//
//##         |                                            ##//
//##         |                                            ##//
//##  CDivisionLayer                             ¼¼¼ÇÃþ   ##//
//##         ^                                            ##//
//##         |                                            ##//
//##  XBus --+                                   Àü¼ÛÃþ   ##//
//##                                                      ##//
//##########################################################//
//##########################################################//

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServiceLayer& theServiceLayer(void)
{
	static CServiceLayer serviceLayer;

	return serviceLayer;
}


CServiceLayer::CServiceLayer() : m_bInit(false)
{
}

CServiceLayer::~CServiceLayer()
{
}

bool CServiceLayer::initialize(void)
{
	if(m_bInit == true) {
		LOGGER(TRACE_INFO, "service layer is already initialized.");
		return true;
	}

	theDivisionLayer().setCallBack(CSIM_MSG_ID, CServiceLayer::callBack_CSIMEvent);
	theDivisionLayer().setCallBack(VER_MSG_ID, CServiceLayer::callBack_VEREvent);

	m_bInit = true;

	theDivisionLayer().initialize();

	LOGGER(TRACE_LEVEL1, "service layer : initialized.");

	return true;
}

void CServiceLayer::uninitialize(void)
{
	theDivisionLayer().uninitialize();

	m_bInit = false;
}


bool CServiceLayer::start(void)
{
	return true;
}

bool CServiceLayer::stop(void)
{
	LOGGER(TRACE_LEVEL1, "service layer : destory thread OK.");

	return true;
}

// SGM_HEAD + query string
int CServiceLayer::callBack_CSIMEvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData)
{
	if(!theSRManager().isActive()) return 0;

	if(nID == CSIM_MSG_ID) {
		char* pMsg = (char*) theUtility().clon(pData, nLength);
		theQueueMgr().putMsg(1 /* tcp:0, xbus:1 */, (char*)pMsg, nLength, nFrom /* tcp:sock_handle, xbus:module_id*/);
	}

	return 0;
}


int CServiceLayer::callBack_VEREvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData)
{

//	if(nFrom == SGM_A || nFrom == SGM_B) return 0;  

	if(nLength < sizeof(SGM_MODULE_VERSION_INFO)) {
		LOGGER(TRACE_WARNNING, "service layer : version info received. size unknow. (md:0x%02x,len:%d)", nFrom, nLength);
		return 0;
	}

	SGM_MODULE_VERSION_INFO* pVersion = (SGM_MODULE_VERSION_INFO*) pData;

	theVersionMgr().setVersionInfo((*pVersion).module_code
		, (*pVersion).cMajor
		, (*pVersion).cMinor
		, (*pVersion).cMicro
		, (*pVersion).cXBusMajor
		, (*pVersion).cXBusMinor
		, (*pVersion).cXBusMicro
		, (*pVersion).szUpdateDate
		, (*pVersion).szUpdateOwner
		,(*pVersion).szDescription);

	LOGGER(TRACE_LEVEL3, "service layer : version info received. (md:0x%02x,ver:%d.%d.%d)", (*pVersion).module_code, (*pVersion).cMajor, (*pVersion).cMinor, (*pVersion).cMicro);

	if(!theSRManager().isActive()) return 0;

	CSIM_DB_HEAD csimDBHead;
	char* pBufData;
	char* pQuery;

	pBufData = (char*) malloc(1024);

	pQuery = pBufData + CSIM_DB_HEAD_SIZE;

	memset(&csimDBHead, 0x00, CSIM_DB_HEAD_SIZE); 
	memcpy(pBufData, &csimDBHead, CSIM_DB_HEAD_SIZE);
	sprintf(pQuery, "begin sp_modVersionUpdate(%d, %d, '%d.%d.%d', '%s', '%s', '%s'); end;"
		, theGlobal().getASIdx()
		, (*pVersion).module_code
		, (*pVersion).cMajor, (*pVersion).cMinor, (*pVersion).cMicro
		, (*pVersion).szUpdateDate
		, (*pVersion).szUpdateOwner
		, (*pVersion).szDescription);

	theQueueMgr().putMsg(1 /* tcp:0, xbus:1 */, pBufData, strlen(pQuery) + CSIM_DB_HEAD_SIZE, 0 /* tcp:sock_handle, xbus:module_id*/);

	return 0;
}


bool CServiceLayer::sendToANY(unsigned char cTo, unsigned short int nCmdID, int length, char* pMessage)
{
	int nRet;

	nRet = CDivisionLayer::send(length, cTo, nCmdID, (u_char*)pMessage);

	return true;
}
