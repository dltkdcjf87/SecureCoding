// ServiceLayer.h: interface for the CServiceLayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICELAYER_H__D0C84FAB_B360_4A31_B870_6176768D1C76__INCLUDED_)
#define AFX_SERVICELAYER_H__D0C84FAB_B360_4A31_B870_6176768D1C76__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CServiceLayer  
{
protected:
	bool m_bInit;

public:
	static int callBack_CSIMEvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData);
	static int callBack_VEREvent(int nLength, unsigned short int nID /*xbus id*/, unsigned char nFrom, unsigned char* pData);

	bool sendToANY(unsigned char cTo, unsigned short int nCmdID, int length, char* pMessage);

public:
	CServiceLayer();
	virtual ~CServiceLayer();

	bool initialize(void);
	void uninitialize(void);
	bool start(void);
	bool stop(void);

	friend CServiceLayer& theServiceLayer(void);
};

CServiceLayer& theServiceLayer(void);

#endif // !defined(AFX_SERVICELAYER_H__D0C84FAB_B360_4A31_B870_6176768D1C76__INCLUDED_)
