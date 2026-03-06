// VersionInfoManager.cpp: implementation of the CVersionInfoManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "VersionInfoManager.h"
#include "Logger.h"

#if !defined(WIN32)
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVersionInfoManager& theVersionMgr(void)
{
	static CVersionInfoManager _this_;

	return _this_;
}


CVersionInfoManager::CVersionInfoManager()
{
	m_pSharedInfo = null;
}

CVersionInfoManager::~CVersionInfoManager()
{

}

bool CVersionInfoManager::initialize(void)
{
	if((m_pSharedInfo = SharedOpen()) == null) {
		LOGGER(TRACE_WARNNING, "HA SHARED MEM CREATE FAIL!!");
		return false;
	}
	
	return true;
}

CVersionInfoManager::VER_INFO_SHM* CVersionInfoManager::SharedOpen(void)
{
	char *p;

	p = null;

#if !defined(WIN32)
	int id;

	if((id = shmget(SHM_BASE_KEY, sizeof(VER_INFO_SHM)*MAX_MODULE_COUNT, 0666 | IPC_CREAT)) < 0){
		LOGGER(TRACE_INFO, "HA SHARED MEM CREATE FAIL : %s", strerror(errno));
		return (VER_INFO_SHM *)-1;
	}

	if((p = (char *)shmat(id, 0, 0)) == (char *)-1){
		LOGGER(TRACE_INFO, "HA SHARED MEM OPEN FAIL : %s", strerror(errno));
		return (VER_INFO_SHM *)-1;
	}
#endif

	return (VER_INFO_SHM *)p;
}

void CVersionInfoManager::setVersionInfo(unsigned char cModuleID, char cMajor, char cMinor, char cMicro, char cXBusMajor, char cXBusMinor, char cXBusMicro, char* szDate, char* szOwner, char* szDescription)
{
	if(m_pSharedInfo == null) return;

	m_pSharedInfo[cModuleID].module_code = cModuleID;
	m_pSharedInfo[cModuleID].cMajor = cMajor;
	m_pSharedInfo[cModuleID].cMinor = cMinor;
	m_pSharedInfo[cModuleID].cMicro = cMicro;
	m_pSharedInfo[cModuleID].cXBusMajor = cXBusMajor;
	m_pSharedInfo[cModuleID].cXBusMinor = cXBusMinor;
	m_pSharedInfo[cModuleID].cXBusMicro = cXBusMicro;
	strcpy(m_pSharedInfo[cModuleID].szUpdateDate, szDate);
	strcpy(m_pSharedInfo[cModuleID].szUpdateOwner, szOwner);
	strcpy(m_pSharedInfo[cModuleID].szDescription, szDescription);

	return;
}
