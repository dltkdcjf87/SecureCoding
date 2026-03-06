// RegularManager.h: interface for the CRegularManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGULARMANAGER_H__8354B441_2504_4864_A16F_6A3535815B9F__INCLUDED_)
#define AFX_REGULARMANAGER_H__8354B441_2504_4864_A16F_6A3535815B9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef ALTI_V6
#include "AltiCinf.h"
#endif

#ifdef OCCI
#include "OCCIExt.h"
#endif

#include "STSDef.h"

class CRegularManager  
{
private:
#ifdef OCCI
    COCCIExt    m_occi;
#endif

#ifdef ALTI_V6
    AltiCinf m_atci;
#endif

	int m_nIndex;

	THREAD_HANDLE m_hThread;

protected:
	static unsigned long executeThread(LPVOID param);
	void main(void);
	
	bool stsRegist(const char cASID, const char* pData, int nLength);

	bool insertSCMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SCM_STS_ORG* pData);
	bool insertSCM2Sts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SCM_BD_STS_ORG* pData, int nLength);
	// 20150826 bible - TAS Regi/De-regi 통계 처리를 위해 추가
	bool insertSPY_Regi_Sts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SPY_REGI_STS_ORG* pData);
	// 20151110 bible IPv6 SPY Proxy 통계 추가
	bool insertSPYSts_v6(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SPY_STS_ORG* pData);
	bool insertSPYSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SPY_STS_ORG* pData);
	bool insertSAMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SAM_STS_ORG* pData);
	bool insertSRMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SRM_STS_ORG* pData, int nLength);
#ifdef _BULK_STS
	bool insertSRMSts_V2(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SRM_STS_ORG_V2* pData, int nLength);
#endif
//	bool insertDBMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, DBM_STS_ORG* pData, int nLength);
//	bool insertMGMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, MGM_STS_ORG* pData);
//	bool insertCGMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, CGM_STS_ORG* pData);
	bool insertDGMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, DGM_STS_ORG* pData);
//	bool insertSGMSts(const char cASID, unsigned char cModuleID, const char* szStartDate, const char* szEndDate, SGM_STS_ORG* pData);

public:
	CRegularManager(int nIdx);
	virtual ~CRegularManager();

	void initialize(void);
	void start(void);
};

#endif // !defined(AFX_REGULARMANAGER_H__8354B441_2504_4864_A16F_6A3535815B9F__INCLUDED_)
