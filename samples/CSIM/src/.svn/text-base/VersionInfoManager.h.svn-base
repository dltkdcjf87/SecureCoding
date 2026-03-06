// VersionInfoManager.h: interface for the CVersionInfoManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VERSIONINFOMANAGER_H__6A7E603F_97A6_4E87_9D07_72AD9A69E5C0__INCLUDED_)
#define AFX_VERSIONINFOMANAGER_H__6A7E603F_97A6_4E87_9D07_72AD9A69E5C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_MODULE_COUNT 0xff
#define SHM_BASE_KEY 	(key_t)0xa0099900   /* 0xa0099900 ~ 0xa00999xx */

class CVersionInfoManager  
{
protected:
	typedef struct {
		unsigned int module_code;
		unsigned char cMajor;
		unsigned char cMinor;
		unsigned char cMicro;
		unsigned char cXBusMajor;
		unsigned char cXBusMinor;
		unsigned char cXBusMicro;
		char szUpdateDate[16]; //YYYYMMDD
		char szUpdateOwner[16]; //chiwoo
		char szDescription[128];
	} VER_INFO_SHM;

	VER_INFO_SHM* m_pSharedInfo;

public:
	CVersionInfoManager();
	virtual ~CVersionInfoManager();

	VER_INFO_SHM* SharedOpen(void);

	bool initialize(void);
	void setVersionInfo(unsigned char cModuleID, char cMajor, char cMinor, char cMicro, char cXBusMajor, char cXBusMinor, char cXBusMicro, char* szDate, char* szOwner, char* szDescription);


	friend CVersionInfoManager& theVersionMgr(void);
};

extern CVersionInfoManager& theVersionMgr(void);

#endif // !defined(AFX_VERSIONINFOMANAGER_H__6A7E603F_97A6_4E87_9D07_72AD9A69E5C0__INCLUDED_)
