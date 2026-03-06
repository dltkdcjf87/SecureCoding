// ManagerLayer.h: interface for the CManagementLayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MANAGERLAYER_H__B24C56CD_F3A3_4791_AD98_2A82E2E01D4A__INCLUDED_)
#define AFX_MANAGERLAYER_H__B24C56CD_F3A3_4791_AD98_2A82E2E01D4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MessageQueue.h"

#include "MMCDef.h"
#include "SGMDef.h"

class CManagerLayer  
{
protected:
	bool m_bInit;
	THREAD_HANDLE m_hThread;
	CMsgQueue m_msgQueue;

	typedef bool (*TypeMMCCmdFunc)(int nFrom, MMC_HEAD* pHead, u_pchar pData);
	typedef std::map<long, TypeMMCCmdFunc> MAP_MMCCMD2FUNC;
	MAP_MMCCMD2FUNC m_mapMMCCMD2FuncList;

	typedef bool (*TypeSTSCmdFunc)(int nFrom, STS_HEAD* pHead, u_pchar pData);
	typedef std::map<long, TypeSTSCmdFunc> MAP_STSCMD2FUNC;
	MAP_STSCMD2FUNC m_mapSTSCMD2FuncList;

	static int callBack_MGMEvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData);
	static int callBack_STSEvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData);

	static u_long WINAPI eventProcessThread(LPVOID param);
	bool mainEventLoop(void);

public:
	int sendToMGM(MMC_HEAD* pHead, unsigned short length, unsigned char* pMessage, int opt = MMC_MSGTYPE_END);
	int sendToMGMAlarm(int nID, unsigned short length, unsigned char* pMessage, int nFrom);
	int sendToMGMState(int nID, unsigned short length, unsigned char* pMessage, int nFrom);
	int sendToSGM(STS_HEAD* pHead, unsigned short length, unsigned char* pMessage, int nFlag);

	void setMMCFuncRegister(int nCmdID, TypeMMCCmdFunc pFunc) { m_mapMMCCMD2FuncList[nCmdID] = pFunc; };
	void setSTSFuncRegister(int nCmdID, TypeSTSCmdFunc pFunc) { m_mapSTSCMD2FuncList[nCmdID] = pFunc; };

public:
	CManagerLayer();
	virtual ~CManagerLayer();

	bool initialize(void);
	void uninitialize(void);
	bool start(void);
	bool stop(void);

	friend CManagerLayer& theMgrLayer(void);
};

extern CManagerLayer& theMgrLayer(void);

#endif // !defined(AFX_MANAGERLAYER_H__B24C56CD_F3A3_4791_AD98_2A82E2E01D4A__INCLUDED_)
