/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_main.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           :
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    :
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"
#include "VersionHistory.h"
#define MY_PS_NAME       "spy"


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : shut_down
 * CLASS-NAME     :
 * PARAMETER    IN: reason - show down reason
 * RET. VALUE     : -
 * DESCRIPTION    : shot down process
 * REMARKS        :
 **end*******************************************************/
void shut_down(int reason)
{
	LIB_unset_run_pid(MY_PS_NAME);
    
	Log.printf(LOG_INF, "\n------------ SPY Termainate reason = %d !! -----------\n\n", reason);
//    Log.close();
	exit(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SPY_PrintVersion
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : print out version information
 * REMARKS        :
 **end*******************************************************/
void SPY_PrintVersion(void)
{
    printf("................... SPY Module Infomation ..................\n");
    printf("Version = %d.%d.%d\n", MVER1,MVER2, MVER3);
    printf("Date    = %s\n", BUILD_DATE);
    printf("Owner   = %s\n", AUTHOR);
    printf("Desc    = %s\n", PKGNAME);
    printf("Build   = %s %s\n", __DATE__, __TIME__);
    printf("........................... SPY ............................\n");
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SPY_LoggingVersion
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : print out version information to log file
 * REMARKS        :
 **end*******************************************************/
void SPY_LoggingVersion(void)
{
    Log.printf(LOG_INF, "................... SPY Module Infomation ..................\n");
//	Log.printf(LOG_INF, "Version = %c.%c.%c\n", MVER1,MVER2, MVER3);
//	Log.printf(LOG_INF, "Date    = %s\n",       BUILD_DATE);
//	Log.printf(LOG_INF, "Owner   = %s\n",       AUTHOR);
//	Log.printf(LOG_INF, "Desc    = %s\n",       PKGNAME);
	Log.printf(LOG_INF, "Package-name = %s\n", 		PKGNAME);
	Log.printf(LOG_INF, "Date         = %s %s\n",	__DATE__, __TIME__);
#if 0 // BMT 날짜 확인 방지 - FIXIT
    Log.printf(LOG_INF, "Build   = %s %s\n",    __DATE__, __TIME__);
#endif
    Log.printf(LOG_INF, "............................................................\n");
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_init_data
 * CLASS-NAME     : 
 * PARAMETER    IN: argc   - arg. counter
 *              IN: argv[] - arg. value
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SPY 초기화
 * REMARKS        : 
 **end*******************************************************/
bool SPY_init_data(int argc, char* argv[])
{
    int     pid;
	versionDisplay( argc, argv);
/*
    if((argc >= 2) && ((strcmp(argv[1], "-version") == 0) || (strcmp(argv[1], "-v") == 0)))
	{
		SPY_PrintVersion();
        exit(0);
	}
*/  
    mkdir("/home/mcpas/log/spy", 0777);     // make log directory
    Log.init("/home/mcpas/log/spy/spy_");   // set log file path
    
    // Register Shutdown task..
	signal(SIGINT,  shut_down);
	signal(SIGTERM, shut_down);
	signal(SIGKILL, shut_down);
	signal(SIGQUIT, shut_down);
//    signal(SIGSEGV, shut_down);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
    
    if(LIB_is_running_process(MY_PS_NAME, &pid) == true)    // 이미 spy가 실행 중인지 확인 (중복 실행 방지)
	{
		Log.printf(LOG_ERR, "Already running process %s, check !!!\n", MY_PS_NAME);
		return(false);
	}

    if(LIB_set_run_pid(MY_PS_NAME) == false)                // 프로세스 정보 등록 - 중복 실행 방지를 위해
	{
		Log.printf(LOG_ERR, "set /var/run/mcpas/%s.pid error\n", MY_PS_NAME);
		return(false);
	}
    
    if(LIB_get_as_id(&MY_AS_ID) == false) { MY_AS_ID = 0; }   // MY AS ID(INDEX) 정보

    if(LIB_get_server_side_id("CMS", (uint8_t *)&MY_SIDE) == false)     // CMS Server SIDE 정보 A(0)/B(1)
	{
		Log.printf(LOG_ERR, "Can't get CMS SIDE_ID in role.def\n");
		return(false);
	}
    
    if(MY_SIDE == 0) { MY_BLK_ID = SPY_A; PEER_BLK_ID = SPY_B; }
    else             { MY_BLK_ID = SPY_B; PEER_BLK_ID = SPY_A; }


    g_nHA_State                          = HA_UnAssign;
    g_nSamServerResetCount               = 0;
    g_nSswServerResetCount[OT_TYPE_ORIG] = 0;
    g_nSswServerResetCount[OT_TYPE_TERM] = 0;
    
    bzero(&statistic,      sizeof(statistic));          // init statistic data
#ifdef IPV6_MODE
    bzero(&statistic_v6,   sizeof(statistic_v6));       // init statistic data
#endif
    
#ifdef INCLUDE_REGI
    bzero(&g_RegiSts, sizeof(g_RegiSts));               // (REGI 통계)
#endif
    
    snprintf(g_strCfgFile, sizeof(g_strCfgFile), "/home/mcpas/cfg/CMS%d/mspy.cfg", MY_SIDE); // FIXIT

    if(argc > 1) { strcpy(g_strCfgFile, argv[1]); }     // 실행시 arg로 config file이름을 줄 수 있다(디버깅 용)
    
    SPY_LoggingVersion();
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_read_config
 * CLASS-NAME     :
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SPY Config file들을 메모리로 읽어 들임
 * REMARKS        :
 **end*******************************************************/
bool SPY_read_config(void)
{
    char    filename[MAX_FNAME_LEN];
    
    if(SPY_ReadConfigFile() == false)       // Read spy.cfg (Configuration Info)
    {
        Log.printf(LOG_ERR, "Config file[%s] Read fail !!!!\n", g_strCfgFile);
		return(false);
    }
    
    snprintf(filename, sizeof(filename), "/home/mcpas/cfg/CMS%d/extlist.cfg", MY_SIDE);
    if(g_ExtList.read(filename) == false)    // Read extlist.cfg (EXT List File)
    {
        Log.printf(LOG_INF, "EXT List file[%s] Read fail !!!!\n", filename);
		// continue - not mandatory
    }
    g_ExtList.all_alarm_off();              // add 20161002
    
#ifdef IPV6_MODE
    snprintf(filename, sizeof(filename), "/home/mcpas/cfg/CMS%d/extlist_v6.cfg", MY_SIDE);
    if(g_ExtList_V6.read(filename) == false)    // Read extlist.cfg (EXT List File)
    {
        Log.printf(LOG_INF, "IPv6 EXT List file[%s] Read fail !!!!\n", filename);
        // continue - not mandatory
    }
    g_ExtList_V6.all_alarm_off();           // add 20161002
#endif
    
    snprintf(filename, sizeof(filename), "/home/mcpas/cfg/CMS%d/service_key.cfg", MY_SIDE);
    if(g_SvcKey.read(filename) == false)    // Read service_key.cfg(Service Key List)
    {
        Log.printf(LOG_INF, "service_key config file[%s] Read fail !!!!\n", filename);
        // continue - not mandatory
    }

    return(true);
}

#ifdef MULTI_Q_MODE // 20241205 add kdh m4.4.1-v2.1.2
long getSize_SamQ(int size_type)
{
    long size;
    NSM_PTR_Q* ptr;

    size = 0;
    ptr = 0;

    for(int bd = 0; bd < C_SES_BOARD; bd++)
    {
        for(int th = 0; th < C_MAX_RSAM_SES_THREAD; th++)
        {
            ptr = &Q_Sam[bd][th];
            if(ptr != 0)
            {
                switch(size_type)
                {
                    case 1:
                        size += ptr->size();
                        break;
                    case 2:
                        size += ptr->malloc_size();
                        break;
                    case 3:
                        size += ptr->free_size();
                        break;
                }
            }
        }
    }

    return size;
}
#endif // MULTI_Q_MODE


#if 1
extern int g_nRegiCount;
#endif
void print_out_spy_status(void)
{
    Log.printf(LOG_INF, "------------------------------------- SPY Stat. Report -----------------------------------\n");
	Log.printf(LOG_INF, "global_test_spy_send_to_sam_bye_count(%d)\n", global_test_spy_send_to_sam_bye_count.load());

    Log.printf(LOG_INF, " Hash_O(call, ch) = (%d, %d), Hash_T = (%d, %d) SCM MAP (%d, %d) CR=%d, Call_ID=%d\n",
               g_Hash_O.size(), g_Hash_O.ch_size(), g_Hash_T.size(), g_Hash_T.ch_size(), g_ScmMap.n_inseert, g_ScmMap.n_erase, g_ScmMap.m_map_pCR.size(), g_ScmMap.m_map_CallID.size());
    Log.printf(LOG_INF, " Q_REG:  size = %d, malloc = %ld, free = %ld\n", Q_Reg.size(),   Q_Reg.malloc_size(),   Q_Reg.free_size());
    Log.printf(LOG_INF, " g_CR:              alloc = %ld, free = %ld\n", g_CR.sum_malloc_cnt,   g_CR.sum_free_cnt);
#if INCLUDE_REG
    Log.printf(LOG_INF, " REGI:    IN: %d(%d) SUCC: %d FAIL: %d,  CPS = %d\n", g_RegiSts.in_regi,   g_RegiSts.in_regi_succ  +g_RegiSts.in_regi_fail,   g_RegiSts.in_regi_succ,   g_RegiSts.in_regi_fail, g_nRegiCount);
    Log.printf(LOG_INF, " DeREGI:  IN: %d(%d) SUCC: %d FAIL: %d\n",            g_RegiSts.in_deregi, g_RegiSts.in_deregi_succ+g_RegiSts.in_deregi_fail, g_RegiSts.in_deregi_succ, g_RegiSts.in_deregi_fail);
#endif
//    Log.printf(LOG_INF, " Q_OPT:      size = %d, malloc = %ld, free = %ld\n",   Q_Opt.size(),   Q_Opt.malloc_size(),   Q_Opt.free_size());
    Log.printf(LOG_INF, " Q_ORIG: size = %d, malloc = %ld, free = %ld\n", Q_Ssw_ORIG.size(), Q_Ssw_ORIG.malloc_size(), Q_Ssw_ORIG.free_size());
    Log.printf(LOG_INF, " Q_TERM: size = %d, malloc = %ld, free = %ld\n", Q_Ssw_TERM.size(), Q_Ssw_TERM.malloc_size(), Q_Ssw_TERM.free_size());
    
#ifdef MULTI_Q_MODE // 20241205 kdh add m4.4.1-v2.1.2
    Log.printf(LOG_INF, " Q_SAM:  size = %d, malloc = %ld, free = %ld\n",
                    getSize_SamQ(1), getSize_SamQ(2), getSize_SamQ(3));
#else
    Log.printf(LOG_INF, " Q_SAM:  size = %d, malloc = %ld, free = %ld\n",
               Q_Sam[0].size()+Q_Sam[1].size()+Q_Sam[2].size()+Q_Sam[3].size()+Q_Sam[4].size()+Q_Sam[5].size(),
               Q_Sam[0].malloc_size()+Q_Sam[1].malloc_size()+Q_Sam[2].malloc_size()+Q_Sam[3].malloc_size()+Q_Sam[4].malloc_size()+Q_Sam[5].malloc_size(),
               Q_Sam[0].free_size()+Q_Sam[1].free_size()+Q_Sam[2].free_size()+Q_Sam[3].free_size()+Q_Sam[4].free_size()+Q_Sam[5].free_size());
#endif // MULTI_Q_MODE
    Log.printf(LOG_INF, "------------------------------------------------------------------------------------------\n");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_init_black_list
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Black List 관련 함수 초기화
 * REMARKS        :
 **end*******************************************************/
void SPY_init_black_list(void)
{
    if(C_BLACK_LIST_USE == true)
    {
        if(C_BLACK_LIST_NAME[0]    != '\0') { g_BlackList.read(C_BLACK_LIST_NAME);       }
#ifdef IPV6_MODE
        if(C_BLACK_LIST_NAME_V6[0] != '\0') { g_BlackList_V6.read(C_BLACK_LIST_NAME_V6); }
        
        if((g_BlackList.size() + g_BlackList_V6.size()) == 0)     // File을 읽었는데 등록된 IP가 없는 경우 not use로 변경
#else
        if(g_BlackList.size() == 0)
#endif
        {
            C_BLACK_LIST_USE = false;
            Log.printf(LOG_INF, "BLACK_LIST_USE = true, but no IP registered, so set BLACK_LIST_USE = false !!\n");
        }
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : main
 * CLASS-NAME     :
 * PARAMETER    IN: argc   - arg. counter
 *              IN: argv[] - arg. value
 * RET. VALUE     : -
 * DESCRIPTION    : SPY 메인 함수
 * REMARKS        :
 **end*******************************************************/
int main(int argc, char* argv[])
{
    int     count = 0;
    
    if(SPY_init_data(argc, argv) == false) { shut_down(-1); }
    if(SPY_read_config()         == false) { shut_down(-2); }
    if(SPY_init_xbus()           == false) { shut_down(-3); }

    SPY_init_black_list();
    
    msleep(500);   // 500 MSEC
 

//v4.4.2-1.1.0 MYUNG 20230906   
#ifdef FAULT_LOG_MODE
	flcm_manager.Fault_Init(MY_BLK_ID, ASSIGN_MODE, "INVALID MSG", "CSCF INVALID SIP MSG RECV COUNT", 5 * 60);
#endif

    if(RSAM_main() == false) { shut_down(-21); }
    if(RSSW_main() == false) { shut_down(-22); }
    
    SEND_LogLevelToEMS();                           // Log Level을 Oracle로 전송 (WEB에서 Log Level을 관리하기 위한 가좌 요청사항)

    g_DBMAlive[0] = get_module_state(DBM_A);        // DBM Process의 상태를 XBUS에서 가져온다.
    g_DBMAlive[1] = get_module_state(DBM_B);

    CreateAlarmSendTherad();                          // add 20170306 - Alatm을 주기적으로 보내는  Thread를 생성한다.
    
    while(1)
    {
        sleep(TIME_CHECK_SSW_EXT_STATUS_CHECK);
        
        if(g_nHA_State != HA_Active)    // Active가 아니면 wait
        {
            if((++count % g_ReportTime) == 0) { print_out_spy_status(); }
            continue;
        }

        g_ExtList.is_options_received();            // 외부 연동서버에서 OPTIONS 메세지를 수신했는지 안했는지 여부를 조사
#ifdef IPV6_MODE
        g_ExtList_V6.is_options_received();
#endif
        g_ErrSvrList.report_to_ems();               // 외부 연동서버 에러 정보를 Oracle로 전송, FIXIT - 넘 자주 보내는거 아닌가? 15초???

        if((++count % g_ReportTime) == 0) { print_out_spy_status(); }    // 60 sec
        
        /* 20170303 CreateAlarmSendTherad() 함수에서 처리하게 변경
        // 20140808 AS Block 시 주기적으로 Alarm 전송하게 변경 (OAMS에서 유실되는 경우가 있음)
        if(C_AS_BLOCK)
        {
            SEND_AsBlockStatusToEMS();
            SEND_AsBlockAlarm();
        }
         */
    }
    
//	shut_down(999);
}


