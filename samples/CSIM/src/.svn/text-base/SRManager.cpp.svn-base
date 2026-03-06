// SRManager.cpp: implementation of the CSRManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "SRManager.h"
#include "DivisionLayer.h"
#include "XBusDef3.h"

#include "Logger.h"

#if !defined(WIN32)
	#include <sys/time.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSRManager& theSRManager(void)
{
	static CSRManager the;

	return the;
}


CSRManager::CSRManager()
{
#if !defined(SINGLE_MODE)
	setStatus(HA_UNDEFINED);
#else
	setStatus(HA_ACTIVE);
#endif
}

CSRManager::~CSRManager()
{

}

bool CSRManager::initialize(void)
{
	// 이중화 상태변경 콜백 등록
#if !defined(SINGLE_MODE)
	reg_callback(HA_UnAssign,	ha_CallBackUnAssign);
	reg_callback(HA_Active,		ha_CallBackActive);
	reg_callback(HA_Standby,	ha_CallBackStandby);
	reg_callback(HA_AddStandby, ha_CallBackAddStandby);
	reg_callback(HA_RemoveStandby, ha_CallBackRemoveStandby);

	reg_mod_state_change(ha_CallBackModuleStateChange);

	LOGGER(TRACE_INFO, "sr manager : DUAL MODE");
#else
	LOGGER(TRACE_INFO, "sr manager : SINGLE MODE");
#endif

	if(theSRManager().isActive()) LOGGER(TRACE_INFO, "sr manager : ACTIVE MODE");
	else LOGGER(TRACE_INFO, "sr manager : STANDBY MODE");

	return true;
}

void CSRManager::uninitialize(void)
{
}

void CSRManager::changeStatus(int nStatus)
{
	switch(nStatus) 
	{
	case HA_UnAssign :    /* Unassign */
		LOGGER(TRACE_INFO, "HA CHANGE TO UNASSIGN");
		setStatus(HA_UNDEFINED);
		break;
    case HA_Active :          /* Active */
		LOGGER(TRACE_INFO, "HA CHANGE TO ACTIVE");
		setStatus(HA_ACTIVE);
		break;
    case HA_Standby :         /* Standby */
		LOGGER(TRACE_INFO, "HA CHANGE TO STANDBY");
		setStatus(HA_STANDBY);
		break;
    case HA_AddStandby :           /* Add_Standby */
		LOGGER(TRACE_INFO, "HA CHANGE ADD STANDBY");
		break;
    case HA_RemoveStandby :            /* Remove_Standby */
		LOGGER(TRACE_INFO, "HA CHANGE REMOVE STANDBY");
		break;
	}
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackUnAssign                           ##//
//## [ATTRIBUTE]                                        ##//
//##      int nParam : 미정의                           ##//
//## [DESCRIPTION]                                      ##//
//##      XBUS 이중화기능중 Active/Standby 미정의 상태  ##//
//##      를 알리는 콜백 함수                           ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackUnAssign(int nParam)
{
	theSRManager().changeStatus(HA_UnAssign);
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackActive                             ##//
//## [ATTRIBUTE]                                        ##//
//##      int nParam : 미정의                           ##//
//## [DESCRIPTION]                                      ##//
//##      XBUS 이중화기능중 Active 상태를 알리는 콜백   ##//
//##      함수                                          ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackActive(int nParam)
{
	theSRManager().changeStatus(HA_Active);
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackActive                             ##//
//## [ATTRIBUTE]                                        ##//
//##      int nParam : 미정의                           ##//
//## [DESCRIPTION]                                      ##//
//##      XBUS 이중화기능중 Standby 상태를 알리는 콜백  ##//
//##      함수                                          ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackStandby(int nParam)
{
	theSRManager().changeStatus(HA_Standby);
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackActive                             ##//
//## [ATTRIBUTE]                                        ##//
//##      int nParam : 미정의                           ##//
//## [DESCRIPTION]                                      ##//
//##      XBUS 이중화기능중 Standby가 실행되었음을      ##//
//##      알리는 콜백 함수                              ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackAddStandby(int nParam)
{
	theSRManager().changeStatus(HA_AddStandby);
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackActive                             ##//
//## [ATTRIBUTE]                                        ##//
//##      int nParam : 미정의                           ##//
//## [DESCRIPTION]                                      ##//
//##      XBUS 이중화기능중 Standby가 제거되었음을      ##//
//##      알리는 콜백 함수                              ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackRemoveStandby(int nParam)
{
	theSRManager().changeStatus(HA_RemoveStandby);
}

//########################################################//
//## [FUNCTION]                                         ##//
//##      ha_CallBackModuleStateChange                  ##//
//## [ATTRIBUTE]                                        ##//
//##      unsigned char mod_code : 모듈코드             ##//
//##      int nAlive : 모듈상태 (0:Kill, 1:Alive        ##//
//## [DESCRIPTION]                                      ##//
//##      타모듈의 상태가 변경되는 경우 알려주는 콜백   ##//
//##                                                    ##//
//## [HISTORY]                                          ##//
//##   DATE        EDITOR    DESC                       ##//
//##   2005.03.02  CHIWOO    CREATE                     ##//
//##                                                    ##//
//########################################################//
void CSRManager::ha_CallBackModuleStateChange(unsigned char mod_code, int nAlive)
{
	if(nAlive == 0x00)
		LOGGER(TRACE_INFO, "UNACCESSIBLE : %s(0x%02x)", MODULE_NAME(mod_code), mod_code);
	else
		LOGGER(TRACE_INFO, "ACCESSIBLE : %s(0x%02x)", MODULE_NAME(mod_code), mod_code);

	return;
}

