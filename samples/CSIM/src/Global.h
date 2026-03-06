// Global.h: interface for the CGlobal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOBAL_H__58E69337_C88F_4312_959B_46747DD02D4D__INCLUDED_)
#define AFX_GLOBAL_H__58E69337_C88F_4312_959B_46747DD02D4D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined(_WIN32) || defined(WIN32)
	#define CSIM_CONFIG_FILE "./cfg/EMS%d/csim.cfg"
	#define GLOBAL_CONFIG_FILE	"./cfg/role.def"
#else
	#define CSIM_CONFIG_FILE "/home/mcpas/cfg/EMS%d/csim.cfg"
	#define GLOBAL_CONFIG_FILE	"/home/mcpas/cfg/role.def"
#endif

#ifdef _BULK_STS
#define	MAX_AS_COUNT	12
#endif

class CGlobal  
{
private:
	u_char m_ucEMSIdx;
	u_char m_ucASIdx;

	char szCSIMConfigFile[256];

#ifdef _BULK_STS
	bool	bBulkMode[MAX_AS_COUNT];
#endif

public:
	CGlobal();

	char* getCSIMConfigFile(void) { return szCSIMConfigFile; };
	void setLogLevel(int nLevel);
	int getLogLevel();

	void processCheck(char* pName);

	int getASIdx(void) { return m_ucASIdx; };
	int getEMSIdx(void) { return m_ucEMSIdx; };

#ifdef _BULK_STS
	void readBulkMode(void);
	bool isBulkMode(int nASID);
#endif

	friend CGlobal& theGlobal(void);
};

CGlobal& theGlobal(void);

#endif // !defined(AFX_GLOBAL_H__58E69337_C88F_4312_959B_46747DD02D4D__INCLUDED_)
