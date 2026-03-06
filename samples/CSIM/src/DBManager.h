// DBManager.h: interface for the CDBManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBMANAGER_H__A6949EC5_B179_4FC6_9AFD_F84E05E9B6F4__INCLUDED_)
#define AFX_DBMANAGER_H__A6949EC5_B179_4FC6_9AFD_F84E05E9B6F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef ALTI_V6
#include "AltiCinf.h"
#endif

#ifdef OCCI
#include "OCCIExt.h"
#endif


class CDBManager  
{
private:

#ifdef OCCI
    COCCIExt	m_occi;
#endif

#ifdef ALTI_V6
    AltiCinf	m_atci;
#endif

	int m_nIndex;

	THREAD_HANDLE m_hThread;

public:
	CDBManager(int nIdx);
	virtual ~CDBManager();

	void initialize(void);
	void start(void);

protected:
	static unsigned long executeThread(LPVOID param);
	void main(void);

};

#endif // !defined(AFX_DBMANAGER_H__A6949EC5_B179_4FC6_9AFD_F84E05E9B6F4__INCLUDED_)
