// DBManager.cpp: implementation of the CDBManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "ServiceLayer.h"
#include "DBManager.h"
#include "QueueManager.h"
#include "ServiceDaemon.h"

#include "CSIMDef.h"
#include "Global.h"

#include "Logger.h"
#include "XBusDef3.h"
#include "Utility.h"

#include "SessionUtility.h"
using namespace chiwoo;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDBManager::CDBManager(int nIdx)
{
	m_hThread = 0;
	m_nIndex = nIdx;
}

CDBManager::~CDBManager()
{

}

void CDBManager::initialize(void)
{
	char szUserID[32];
	char szPasswd[32];
	char szConnStr[32];

	GetPrivateProfileStringv2("CSIM", "ORACLE_USERID", "sysdba", szUserID, sizeof(szUserID), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("CSIM", "ORACLE_PASSWD", "oracle", szPasswd, sizeof(szPasswd), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("CSIM", "ORACLE_CONNSTR", "", szConnStr, sizeof(szConnStr), theGlobal().getCSIMConfigFile()); 

#ifdef OCCI
	m_occi.initialize(szUserID, szPasswd, szConnStr);
#endif
#ifdef ALTI_V6
	char	ip[128];
	GetPrivateProfileStringv2("CSIM", "LOCAL_IP", "", ip, sizeof(ip), theGlobal().getCSIMConfigFile());
	m_atci.initialize(szUserID, szPasswd, ip);
#endif

}

void CDBManager::start(void)
{
	u_long ulThreadID = 0;

	if(m_hThread != 0) return;

	_CreateThread(m_hThread, NULL, 0, (LPTHREAD_START_ROUTINE) CDBManager::executeThread, (LPVOID)this, 0, &ulThreadID);

	if(m_hThread == null) {
		LOGGER(TRACE_ERROR, "db manager : create thread 1 fail. [%ld]", GetLastError());
	
		return;
	}
}


unsigned long CDBManager::executeThread(LPVOID param)
{
	CDBManager* pThis = (CDBManager*) param;

	(*pThis).main();

	return 0;
}

void CDBManager::main(void)
{
	IFString streamBuff('\1', '\2', 6, 2048);
	SessionStream stmClient(false);

	CMsgQueue::_MSG_ msg;
	std::string sResultQuery;
	char* pQuery;
	bool bOk;
	char tmp_Query[8];

	SESSION_INFO session;

	while(true)
	{
		if(theQueueMgr().getMsg(msg) == false) continue;

		if(msg.id == 0) { /* tcp */

			pQuery = msg.pData + CSIM_BOOMERANG_LEN;

			CSIM_TCP_HEAD csimHead;

			if(!theServiceDaemon().findSessionInfo(msg.nParam, &session))
			{
				msg.clear();
				continue;
			}

			stmClient.set(session.handle, sessionTypeTCP);

			csimHead.nAsCode = session.nAS;
			csimHead.nModule = 0;
			csimHead.nMsgId = msg.id;

#ifdef OCCI
			if(m_occi.isConnect()) {
				
				if(*pQuery == 's' || *pQuery == 'S') {
					bOk = m_occi.sqlSelect(pQuery, sResultQuery);
				} else {
/*
#ifdef ALTI_V6
					if(*pQuery == 'e' || *pQuery =='E')
					{
						for(int i=0; i<6; i++)
						{
							if((pQuery[i] >= 'A') && (pQuery[i] <= 'Z'))
								tmp_Query[i] = pQuery[i] -'A' + 'a';
						}
						
						if(strstr(tmp_Query, "execute") != NULL)
							*pQuery = pQuery[8]; //execute sp_~
						else
							*pQuery = pQuery[5]; //exec sp_~
					}
#endif
*/
					bOk = m_occi.sqlExecute(pQuery, sResultQuery);
				}

				if(bOk) {
					m_occi.commit();
					csimHead.result = successCode;
					csimHead.reason = reasonOk;
				} else {
					if(m_occi.isConnect()) csimHead.result = csimDataWrong;
					else csimHead.result = dbmsDisconnect;
					csimHead.reason = reasonOk;
				}
			} else {
				csimHead.result = failCode;
				csimHead.reason = dbmsDisconnect;
			}
#endif
#ifdef ALTI_V6
            if(m_atci.isConnect()) {

                if(*pQuery == 's' || *pQuery == 'S') {
                    bOk = m_atci.sqlSelect(pQuery, sResultQuery);
                } else {
                    bOk = m_atci.sqlExecute(pQuery, sResultQuery);
                }

                if(bOk) {
                    m_atci.commit();
                    csimHead.result = successCode;
                    csimHead.reason = reasonOk;
                } else {
                    if(m_atci.isConnect()) {csimHead.result = csimDataWrong; }
                    else csimHead.result = dbmsDisconnect;
                    csimHead.reason = reasonOk;
                }
            } else {
                csimHead.result = failCode;
                csimHead.reason = dbmsDisconnect;
            }
#endif

			streamBuff.clear();
			streamBuff.append(sizeof(CSIM_TCP_HEAD), (char*)&csimHead);
			streamBuff.append(CSIM_BOOMERANG_LEN, msg.pData);
			streamBuff.append(sResultQuery.size(), sResultQuery.c_str());

			try {
				stmClient.send(&streamBuff);
			} catch(const char* errMsg) {
				stmClient.disconnect();
			}

		} else if(msg.id == 1) { /* xbus */
			pQuery = msg.pData + sizeof(CSIM_DB_HEAD);



			CSIM_DB_HEAD csimDBHead;

			memcpy(&csimDBHead, msg.pData, sizeof(CSIM_DB_HEAD));
#ifdef OCCI
			if(m_occi.isConnect()) {

				pQuery = msg.pData + CSIM_BOOMERANG_LEN;
				
				if(*pQuery == 's' || *pQuery == 'S') {
					m_occi.sqlSelect(pQuery, sResultQuery);
				} else {
					m_occi.sqlExecute(pQuery, sResultQuery);
				}

			} else {
				sResultQuery = "0005:SERVER DISCONNECTED";
			}
#endif
#ifdef ALTI_V6
            if(m_atci.isConnect()) {

                pQuery = msg.pData + CSIM_BOOMERANG_LEN;

                if(*pQuery == 's' || *pQuery == 'S') {
                    m_atci.sqlSelect(pQuery, sResultQuery);
                } else {
                    m_atci.sqlExecute(pQuery, sResultQuery);
                }

            } else {
                sResultQuery = "0005:SERVER DISCONNECTED";
            }

#endif

			if(msg.nParam != 0)
			{
				streamBuff.clear();
				streamBuff.append(sizeof(CSIM_DB_HEAD), (char*)&csimDBHead);
				streamBuff.append(sResultQuery.size(), sResultQuery.c_str());

				theServiceLayer().sendToANY(msg.nParam, CSIM_MSG_ID, streamBuff.length(), streamBuff.getBuff());

			} 
		}

		msg.clear();
	}
}


