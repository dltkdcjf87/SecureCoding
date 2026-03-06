// MMCManager.cpp: implementation of the CMMCManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "MMCManager.h"

#include "ManagerLayer.h"
#include "DivisionLayer.h"

#include "SRManager.h"
#include "ServiceDaemon.h"
#include "QueueManager.h"

#include "VersionHistory.h"
#include "Global.h"
#include "CSIMDef.h"

#include "XBusDef3.h"
#include "Logger.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMMCManager& theMMCManager(void)
{
	static CMMCManager manager;

	return manager;
}

CMMCManager::CMMCManager()
{

}

CMMCManager::~CMMCManager()
{

}

bool CMMCManager::initialize(void)
{
	theMgrLayer().initialize();

	theMgrLayer().setMMCFuncRegister(MMC_DIS_VER_CSIM, CMMCManager::cmd_VersionInfo);		//## Version 정보 확인
	theMgrLayer().setMMCFuncRegister(MMC_DIS_STA_CSIM, CMMCManager::cmd_SessionStatusInfo);		//## Version 정보 확인

	theMgrLayer().setMMCFuncRegister(MMC_SET_LOGLV_CSIM, CMMCManager::cmd_SetLogLevel);		//## Version 정보 확인

	theMgrLayer().start();

	LOGGER(TRACE_LEVEL1, "mmc manager : initialized.");

	return true;
}

void CMMCManager::uninitialize(void)
{
	LOGGER(TRACE_LEVEL1, "CMMCManager::uninitialize : Start");

	theMgrLayer().uninitialize();

	LOGGER(TRACE_LEVEL1, "CMMCManager::uninitialize : End OK.");
}

bool CMMCManager::cmd_VersionInfo(int nFrom, MMC_HEAD* pHead, u_pchar pData)
{
	IMD_34001* pIMD = (IMD_34001*)pData;
	OMD_34001 omdData;

	if((*pIMD).cSide != theGlobal().getEMSIdx() && !((*pIMD).cSide == 2 && theSRManager().isActive())) return true;

	LOGGER(TRACE_INFO, "MMC : Version 정보 확인.(%d)", (*pIMD).cSide);

	memset(&omdData, 0x00, sizeof(omdData));

	//################################################//

	sprintf(omdData.szVersion, "%d.%d.%d", version_num[0], version_num[1], version_num[2]);
	strcpy(omdData.szLastUpdateDate, version_date);
	strcpy(omdData.szLastUpdateUser, version_user);
	strcpy(omdData.szDescription, version_dest);

	//################################################//

	theMgrLayer().sendToMGM(pHead, sizeof(OMD_34001), (u_char*)&omdData);

	return true;
}

bool CMMCManager::cmd_SessionStatusInfo(int nFrom, MMC_HEAD* pHead, u_pchar pData)
{
	IMD_34002* pIMD = (IMD_34002*)pData;
	OMD_34002* pOmdData;
	OMD_34002_ITEM* pItem;
	int nLength;

	if((*pIMD).cSide != theGlobal().getEMSIdx() && !((*pIMD).cSide == 2 && theSRManager().isActive())) return true;

	LOGGER(TRACE_INFO, "MMC : Session 정보 확인.");

	nLength = sizeof(OMD_34002) + sizeof(OMD_34002_ITEM) * 20;
	pOmdData = (OMD_34002*)malloc(nLength);

	memset(pOmdData, 0x00, nLength);

	//################################################//
	int i, idx;
	int nAS;
	unsigned char cMoID;
	time_t tvalue;

	idx = 0;
	i = 0;

	while(true)
	{
		pItem = &((*pOmdData).item[idx]);

		if(!theServiceDaemon().getSessionInfo(i, nAS, cMoID, (*pItem).szIP, tvalue, (*pItem).nReqCount, (*pItem).nAckCount, (*pItem).nErrCount)) break;
		(*pItem).cAS = nAS;
		(*pItem).cState = 1;
		(*pItem).tConnected = tvalue;
		(*pItem).cType = (cMoID == CGM_A)?0:(cMoID == CGM_B)?1:(cMoID == SGM_A)?2:(cMoID == SGM_B)?3:4;

		i++; idx++;

		// 20개 단위로 전송
		if(i % 20 == 0) {
			(*pOmdData).cCount = idx;
			nLength = sizeof(OMD_34002) + sizeof(OMD_34002_ITEM) * idx;
			theMgrLayer().sendToMGM(pHead, nLength, (u_char*)pOmdData, MMC_MSGTYPE_CONTINUE);
			idx = 0;
		}
	}

	// 마지막 전송
	(*pOmdData).cCount = idx;
	(*pOmdData).cHAStatus = theSRManager().getStatus();
	nLength = sizeof(OMD_34002) + sizeof(OMD_34002_ITEM) * idx;

	theMgrLayer().sendToMGM(pHead, nLength, (u_char*)pOmdData);

	free(pOmdData);
	return true;
}


bool CMMCManager::cmd_SetLogLevel(int nFrom, MMC_HEAD* pHead, u_pchar pData)
{
	IMD_34201* pIMD = (IMD_34201*)pData;
	OMD_34201 omdData;

	LOGGER(TRACE_INFO, "MMC : LOG LEVEL 설정.");

	memset(&omdData, 0x00, sizeof(omdData));

	//################################################//
	//################################################//

	omdData.cBeforeLogLevel = theLogger().getLevel();

	theLogger().setLevel((*pIMD).ucTraceLevel);
	theGlobal().setLogLevel((*pIMD).ucTraceLevel);

	omdData.cChangedLogLevel = theLogger().getLevel();

	loglevelUpdate( omdData.cChangedLogLevel);

	//################################################//

	if((*pIMD).cSide != theGlobal().getEMSIdx() && !((*pIMD).cSide == 2 && theSRManager().isActive())) return true;

	theMgrLayer().sendToMGM(pHead, sizeof(omdData), (u_char*)&omdData);

	return true;
}

int loglevelUpdate(int lv)
{
	char *pBuff;
	int nLength;

	pBuff = (char*)malloc(512);
	nLength = sprintf(pBuff + sizeof(CSIM_DB_HEAD), "begin sp_setLogLevel(%d, %d, %d); end;", theGlobal().getASIdx(), (theGlobal().getEMSIdx() == 0)?CSIM_A:CSIM_B, lv);

	theQueueMgr().putMsg(1 /* tcp:0, xbus:1*/, pBuff, nLength + sizeof(CSIM_DB_HEAD), 0 /* tcp:sock_handle, xbus:module_id*/);

	return 0;
}

int CMMCManager::MMCFaultSend(int nFaultID, int level, int nOnOff, char* pComment)
{
        char szBuff[1024];
        int nTotalLen;
        MMC_HEAD* pHd = (MMC_HEAD*)szBuff;
        ALM_MSG* pAlarmHd = (ALM_MSG*) (szBuff + sizeof(MMC_HEAD));
        int nCommentLen;

        //if(!theSRManager().isActive()) return 0;

        memset(szBuff, 0x00, sizeof(szBuff));

        nTotalLen = sizeof(MMC_HEAD) + sizeof(ALM_MSG);

        (*pHd).usMsgID = MMC_FAULT_ID;
        (*pHd).usCmdNo = nFaultID;
        (*pHd).ucFrom  = (theGlobal().getEMSIdx() == 0)? CSIM_A : CSIM_B;
        (*pHd).ucTo    = MGM;
        (*pHd).ulTime  = (unsigned long) time(NULL);
        (*pHd).ucType  = MMC_MSGTYPE_END;

        (*pAlarmHd).CaseNo = 0;
        (*pAlarmHd).MoId = (theGlobal().getEMSIdx() == 0)? CSIM_A : CSIM_B;
        (*pAlarmHd).alarmFlag = nOnOff;
        (*pAlarmHd).reserv = 0;
        (*pAlarmHd).alarmId = nFaultID;
        (*pAlarmHd).Extend[0] = (nOnOff == ALARM_ON)?level:0;

        nCommentLen = strlen(pComment);
        if(nCommentLen > 0) {
                memcpy((*pAlarmHd).comment, pComment, nCommentLen);
                nTotalLen += nCommentLen;
        }

        (*pHd).usiLen  = nTotalLen;


        return CDivisionLayer::send( nTotalLen, OAMS, MMC_FAULT_ID, (u_char*)szBuff );
        //return CommunicationFramework::instance().sendMessage(nTotalLen, MGM, MMC_ALARM_ID, (u_char*)szBuff);
}


