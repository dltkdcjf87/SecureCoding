// make_win32.cpp : Defines the entry point for the console application.
//
#include "Environment.h"
#include "DivisionLayer.h"
#include "ManagerLayer.h"
#include "ServiceLayer.h"

#include "SRManager.h"
#include "MMCManager.h"
#include "VersionInfoManager.h"
#include "RegularManager.h"
#include "QueueManager.h"
#include "DBManager.h"
#include "VersionHistory.h"
#include "ServiceDaemon.h"

#include "Global.h"

#include "Utility.h"
#include "Logger.h"
#include "UserStat.h"
#include "SessionUtility.h"
using namespace chiwoo;

void sig_init(void);
int checkDupRun(char *name, int *ps_id);

/*
            connect
   CSIM <-------------- CGM
            cdr_data
        <............. 
		    cdr_resp
		.............>


   CSIM <-------------- SGM
            sts_data
        <............. 
		    sts_resp
		.............>

*/

int main(int argc, char* argv[])
{
  int nPID;

  if(checkDupRun("csim", &nPID)) {
    printf(">> csim Is Already Running Process. pid(%d) <<\n", nPID);
    return -1;
  }

	theGlobal().processCheck("csim");

	LOGGER_START("csim");

	theLogger().setLevel(theGlobal().getLogLevel());

	LOGGER(TRACE_INFO, "csim version : %d.%d.%d", version_num[0], version_num[1], version_num[2]);

#ifdef _BULK_STS
	theGlobal().readBulkMode();
#endif
	
	sig_init();

	SessionStream::socketStartup();

	theDivisionLayer().initialize();
	theMgrLayer().initialize();
	theServiceLayer().initialize();

	theSRManager().initialize();
	theMMCManager().initialize();
	theVersionMgr().initialize();
	theQueueMgr().initialize();

	char ip[64];

	int nCSIMPort = GetPrivateProfileInt("CSIM", "LISTEN_PORT", 9999, theGlobal().getCSIMConfigFile()); 
	int nCDRDBSessionCount = GetPrivateProfileInt("CSIM", "CDRDB_SESSION_COUNT", 3, theGlobal().getCSIMConfigFile()); 
	int nDNDBSessionCount = GetPrivateProfileInt("CSIM", "DYNAMIC_SESSION_COUNT", 3, theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("CSIM", "LOCAL_IP", "", ip, sizeof(ip), theGlobal().getCSIMConfigFile()); 
	
	theServiceDaemon().init();
	theServiceDaemon().daemonStart(ip, nCSIMPort);
	
	CRegularManager* regularMgr;
	int i;
	
	for(i = 0; i < nCDRDBSessionCount; i++)
	{
		regularMgr = new CRegularManager(i);
		(*regularMgr).initialize();
		(*regularMgr).start();
		Sleep(2000);
	}

	CDBManager* dbManager;

	for(i = 0; i < nDNDBSessionCount; i++)
	{
		dbManager = new CDBManager(i);
		(*dbManager).initialize();
		(*dbManager).start();
		Sleep(2000);
	}

	loglevelUpdate(theGlobal().getLogLevel());

	userStatInit();

	while(true) {
		Sleep(1000);
	}

	return 0;
}

void sig_init(void)
{
#if !defined(_WIN32) && !defined(WIN32)
	(void)signal(SIGINT,SIG_IGN);
	(void)signal(SIGQUIT,SIG_IGN);
	(void)signal(SIGTERM,SIG_IGN);
	(void)signal(SIGHUP,SIG_IGN);
	(void)signal(SIGCLD,SIG_IGN);
	(void)signal(SIGPIPE,SIG_IGN);
//	(void)signal(SIGSEGV,SIG_IGN);
	
//	if(fork() != 0) exit(0);

#endif
}


int checkDupRun(char *name, int *ps_id)
{
  char  fname[64];
  int  pid;
  FILE  *fp;

  snprintf(fname, 63, "/var/run/%s.pid", name);
  if((fp = fopen(fname, "r")) == 0) { return(0); }
  fscanf(fp, "%d", &pid);
  fclose(fp);

  *ps_id = pid;

  snprintf(fname, 63, "/proc/%d/status", pid);
  if((fp = fopen(fname, "r")) == 0) { return(0); }
  fclose(fp);

  return 1;
}

