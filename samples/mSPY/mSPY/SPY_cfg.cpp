
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

#define _GET_CONFIG_INT(x)		(!strcmp(#x, var)) x = atoi(val);
#define _GET_CONFIG_STR(x)		(!strcmp(#x, var)) strcpy(x, val);
#define _GET_CONFIG_nSTR(x, n)	(!strcmp(#x, var)) strncpy(x, val, n);
#define _GET_CONFIG_BOOL(x)		(!strcmp(#x, var))          \
    {                                                       \
        if     (!strcasecmp("TRUE", val)) { x = true;  }    \
        else if(!strcasecmp("YES",  val)) { x = true;  }    \
        else                              { x = false; }    \
    }


#pragma mark -
#pragma mark -
#pragma mark spy.cfg file 처리 함수 들


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_PrintOutConfig
 * CLASS-NAME     : -
 * PARAMETER      : 
 * RET. VALUE     : -
 * DESCRIPTION    : 읽어들인 Config 값을 Log파일에 출력
 * REMARKS        :
 **end*******************************************************/
void SPY_PrintOutConfig(void)
{
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "CONFIG FILENAME = %s\n", g_strCfgFile);
    Log.printf(LOG_INF, "============================================\n");
    
    // [DEBUG]
    Log.printf(LOG_INF, "[DEBUG]\n");
    Log.printf(LOG_INF, "  LOG_LEVEL         =   %d\n", C_LOG_LEVEL);
    Log.printf(LOG_INF, "  LOG_MAX_SIZE      =   %d\n", C_LOG_MAX_SIZE);
    Log.printf(LOG_INF, "  LOG_OPTIONS       =   %s\n", (C_LOG_OPTIONS == true) ? "True" : "False");
    Log.printf(LOG_INF, "  LOG_REGISTER      =   %s\n", (C_LOG_REGISTER == true) ? "True" : "False");
    Log.printf(LOG_INF, "  OUTBOUND_PROXY    =   %s\n", (C_OUTBOUND_PROXY == true) ? "True" : "False");
    Log.printf(LOG_INF, "  OUTBOUND_IP       =   %s\n", C_OUTBOUND_IP);
    
    // [MODULE]
    Log.printf(LOG_INF, "[MODULE]\n");
    Log.printf(LOG_INF, "  AS_BLOCK          =   %s\n", (C_AS_BLOCK == true) ? "True" : "False");
    Log.printf(LOG_INF, "  NETWORK_QOS       =   %d\n", C_NETWORK_QOS);
    Log.printf(LOG_INF, "  ORIG_STACK_IP     =   %s\n", C_ORIG_STACK_IP);
    Log.printf(LOG_INF, "  ORIG_STACK_PORT   =   %d\n", C_ORIG_STACK_PORT);
    Log.printf(LOG_INF, "  TERM_STACK_IP     =   %s\n", C_TERM_STACK_IP);
    Log.printf(LOG_INF, "  TERM_STACK_PORT   =   %d\n", C_TERM_STACK_PORT);
    Log.printf(LOG_INF, "  HASH_FAIL_ROUTE   =   %s\n", (C_HASH_FAIL_ROUTE == true) ? "True" : "False");
    Log.printf(LOG_INF, "  BYE_NO_HASH_200   =   %s\n", (C_BYE_NO_HASH_200 == true) ? "True" : "False");
//    Log.printf(LOG_INF, "  USE_SCM_MAP       =   %s\n", (C_USE_SCM_MAP     == true) ? "True" : "False");
    Log.printf(LOG_INF, "  SCM_MAP_AUDIT_TIME =   %d\n", C_SCM_MAP_AUDIT_TIME);

    Log.printf(LOG_INF, "  REPORT_TIME       =   %d\n", C_REPORT_TIME);
    Log.printf(LOG_INF, "  ALARM_SEND_TIME   =   %d\n", C_ALARM_SEND_TIME);     // Add 20170306
    
#ifdef IPV6_MODE
    Log.printf(LOG_INF, "  ORIG_STACK_IP_V6  =   %s\n", C_ORIG_STACK_IP_V6);
    Log.printf(LOG_INF, "  TERM_STACK_IP_V6  =   %s\n", C_TERM_STACK_IP_V6);
    Log.printf(LOG_INF, "  IP_MODE_V6        =   %s\n", (C_IP_MODE_V6 == true) ? "True" : "False");
#endif
    Log.printf(LOG_INF, "  SSW_PORT          =   %d\n", C_SSW_PORT);
    Log.printf(LOG_INF, "  SAM_PORT          =   %d\n", C_SAM_PORT);//MIN ADD 20210415
    Log.printf(LOG_INF, "  SYNTAX_CHECK      =   %s\n", (C_SYNTAX_CHECK == true) ? "True" : "False");
    Log.printf(LOG_INF, "  MAX_REG_Q_SIZE    =   %d\n", C_MAX_REG_Q_SIZE);
    Log.printf(LOG_INF, "  REG_Q_ALM_LIMIT   =   %d\n", C_REG_Q_ALM_LIMIT);       // add 20170303
    
#ifdef ECS_MODE
    Log.printf(LOG_INF, "  ECS_UPDATE        =   %s\n", (C_ECS_UPDATE == true) ? "True" : "False");
#endif


    // [THREAD]
    Log.printf(LOG_INF, "[THREAD]\n");
    Log.printf(LOG_INF, "  MAX_RSSW_ORIG_THREAD =   %d\n", C_MAX_RSSW_ORIG_THREAD);
    Log.printf(LOG_INF, "  MAX_RSSW_TERM_THREAD =   %d\n", C_MAX_RSSW_TERM_THREAD);
    Log.printf(LOG_INF, "  MAX_RSSW_REG_THREAD  =   %d\n", C_MAX_RSSW_REG_THREAD);
    Log.printf(LOG_INF, "  MAX_RSSW_OPT_THREAD  =   %d\n", C_MAX_RSSW_OPT_THREAD);
    Log.printf(LOG_INF, "  MAX_RSAM_SES_THREAD  =   %d\n", C_MAX_RSAM_SES_THREAD);
    
    // [SES]
    Log.printf(LOG_INF, "[SES]\n");
    Log.printf(LOG_INF, "  SES_BOARD            =   %d\n", C_SES_BOARD);
//// 20240123 bible : ONE Server - Multi SES
//#ifndef ONE_SERVER_MULTI_SES
    Log.printf(LOG_INF, "  SES_PORT             =   %d\n", C_SES_PORT);
//#endif
    for(int i = 0; i < MAX_SES_BOARD; i ++)
    {
        if(C_SES_IP[i][0] != '\0')
        {
            Log.printf(LOG_INF, "  SES_%02d            =   %s\n", i, C_SES_IP[i]);
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//			Log.printf(LOG_INF, "  SES_%02d_PORT       =   %d\n", i, C_SES_PORT[i]);
//#endif
        }
    }
    
    // [ETC]
    Log.printf(LOG_INF, "[ETC]\n");
    Log.printf(LOG_INF, "  DIAL_IN_PREFIX         =   %s[len=%d]\n", C_DIAL_IN_PREFIX, C_DIAL_IN_PREFIX_LEN);
    Log.printf(LOG_INF, "  TAS_BLOCK_480_FLAG     =   %s\n", (C_TAS_BLOCK_480_FLAG == true) ? "True" : "False");
    Log.printf(LOG_INF, "  DEL_18X_SUPPORTED_FLAG =   %s\n", (C_DEL_18X_SUPPORTED_FLAG == true) ? "True" : "False");
    Log.printf(LOG_INF, "  REFER_405              =   %s\n", (C_REFER_405 == true) ? "True" : "False");
    Log.printf(LOG_INF, "  BLACK_LIST_USE         =   %s\n", (C_BLACK_LIST_USE == true) ? "True" : "False");
    Log.printf(LOG_INF, "  BLACK_LIST_NAME        =   %s\n", C_BLACK_LIST_NAME);
#ifdef IPV6_MODE
    Log.printf(LOG_INF, "  BLACK_LIST_NAME_V6     =   %s\n", C_BLACK_LIST_NAME_V6);
#endif
    Log.printf(LOG_INF, "  WEB_EXTLIST_NAME       =   %s\n", C_WEB_EXTLIST_NAME);
#ifdef IPV6_MODE
    Log.printf(LOG_INF, "  WEB_EXTLIST_NAME_V6    =   %s\n", C_WEB_EXTLIST_NAME_V6);
#endif
    Log.printf(LOG_INF, "  ADD_UPDATE_TIMER       =   %s\n", (C_ADD_UPDATE_TIMER == true) ? "True" : "False");
    Log.printf(LOG_INF, "  ADD_200_UPDATE_TIMER   =   %s\n", (C_ADD_200_UPDATE_TIMER == true) ? "True" : "False");
    Log.printf(LOG_INF, "  ADD_200_INVITE_TIMER   =   %s\n", (C_ADD_200_INVITE_TIMER == true) ? "True" : "False");
    Log.printf(LOG_INF, "  TAS_REGISTER_SKEY      =   %s\n", C_TAS_REGISTER_SKEY);
    Log.printf(LOG_INF, "  OVERLOAD_RESP_CODE     =   %d\n", C_OVERLOAD_RESP_CODE);
	Log.printf(LOG_INF, "  USE MESSAGE            =   %s\n", (C_USE_MESSAGE == true) ? "True" : "False");	// 20170725
	Log.printf(LOG_INF, "  REGI_API_4             =   %s\n", (C_USE_REGI_API4 == true) ? "True" : "False");	//[JIRA AS-211]
    
    Log.printf(LOG_INF, "============================================\n");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_SetConfigDefaultValue
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Config 변수 Default 값 설정
 * REMARKS        : 0이 아닌 값이 default인 경우
 **end*******************************************************/
void SPY_SetConfigDefaultValue(void)
{
    C_ORIG_STACK_PORT     = 5060;
    C_TERM_STACK_PORT     = 5070;
    
    C_SSW_PORT            = 5060;
    C_SAM_PORT            = 5090; //MIN ADD 20210415
    C_TAS_BLOCK_480_FLAG  = true;
    
    C_SES_BOARD            = MAX_SES_BOARD;
    C_MAX_RSSW_ORIG_THREAD = 2;
    C_MAX_RSSW_TERM_THREAD = 2;
    C_MAX_RSSW_REG_THREAD  = 0;
    C_MAX_RSSW_OPT_THREAD  = 1;
    C_MAX_RSAM_SES_THREAD  = 2;
    
    C_OVERLOAD_RESP_CODE   = 503;
}

#define MAX_SSW_THREAD      128
//#define MAX_SES_THREAD      64 // 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_AdjustConfigValue
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Config 파일을 읽은 후 범위가 맞지 않는 Config 값 조정
 * REMARKS        : include Extension Config
 **end*******************************************************/
void SPY_AdjustConfigValue(void)
{
    if((C_LOG_LEVEL <= 0) || (C_LOG_LEVEL >= LOG_ERR)) { C_LOG_LEVEL = LOG_ERR; }
    
    snprintf(C_ORIG_STACK_IP_PORT, sizeof(C_ORIG_STACK_IP_PORT), "%s:%d", C_ORIG_STACK_IP, C_ORIG_STACK_PORT);
    snprintf(C_TERM_STACK_IP_PORT, sizeof(C_TERM_STACK_IP_PORT), "%s:%d", C_TERM_STACK_IP, C_TERM_STACK_PORT);
    
#ifdef IPV6_MODE
    snprintf(C_ORIG_STACK_IP_PORT_V6, sizeof(C_ORIG_STACK_IP_PORT_V6), "%s:%d", C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT);
    snprintf(C_TERM_STACK_IP_PORT_V6, sizeof(C_TERM_STACK_IP_PORT_V6), "%s:%d", C_TERM_STACK_IP_V6, C_TERM_STACK_PORT);
    inet_pton(AF_INET6, C_ORIG_STACK_IP_V6, &C_IN6_SPY);
#endif
    
    if(C_SES_BOARD > MAX_SES_BOARD) { C_SES_BOARD = MAX_SES_BOARD; }

    if(C_MAX_RSSW_ORIG_THREAD < 0) { C_MAX_RSSW_ORIG_THREAD = 1; }
    if(C_MAX_RSSW_ORIG_THREAD > MAX_SSW_THREAD) { C_MAX_RSSW_ORIG_THREAD = MAX_SSW_THREAD; }      // MAX = 8
    
    if(C_MAX_RSSW_TERM_THREAD < 0) { C_MAX_RSSW_TERM_THREAD = 1; }
    if(C_MAX_RSSW_TERM_THREAD > MAX_SSW_THREAD) { C_MAX_RSSW_TERM_THREAD = MAX_SSW_THREAD; }      // MAX = 8
    
    if(C_MAX_RSSW_REG_THREAD < 0)  { C_MAX_RSSW_REG_THREAD = 1; }
    if(C_MAX_RSSW_REG_THREAD > 16) { C_MAX_RSSW_REG_THREAD = 16; }      // MAX = 16
    
    if(C_MAX_RSSW_OPT_THREAD < 0) { C_MAX_RSSW_OPT_THREAD = 1; }
    if(C_MAX_RSSW_OPT_THREAD > 2) { C_MAX_RSSW_OPT_THREAD = 2; }        // MAX = 2
    
    if(C_MAX_RSAM_SES_THREAD < 0) { C_MAX_RSAM_SES_THREAD = 1; }
    if(C_MAX_RSAM_SES_THREAD > MAX_SES_THREAD) { C_MAX_RSAM_SES_THREAD = MAX_SES_THREAD; }      // MAX = 8

    if(C_REPORT_TIME < TIME_CHECK_SSW_EXT_STATUS_CHECK) { C_REPORT_TIME = TIME_CHECK_SSW_EXT_STATUS_CHECK; }
    if(C_ALARM_SEND_TIME < 5) { C_ALARM_SEND_TIME = 5; }        // MIN = 5 SEC, add 20170306
    
    if((C_SCM_MAP_AUDIT_TIME < 0) || (C_SCM_MAP_AUDIT_TIME > 5)) { C_SCM_MAP_AUDIT_TIME = 3; }    // 0 ~ 5 (0:unuse)
    
    g_ReportTime = (int)(C_REPORT_TIME / TIME_CHECK_SSW_EXT_STATUS_CHECK);
    if(g_ReportTime <= 0) { g_ReportTime = 20; }
    
//    if((C_OVERLOAD_RESP_CODE >= 700) || (C_OVERLOAD_RESP_CODE <=299)) { C_OVERLOAD_RESP_CODE = 503; }
    // SES IP를 uint로 변환한다.
    for(int i = 0; i < MAX_SES_BOARD; i ++)
    {
        if(C_SES_IP[i][0] != '\0') { C_SES_INET_ADDR[i] = (uint32_t)inet_addr(C_SES_IP[i]); }
    }

    C_DIAL_IN_PREFIX_LEN = (int)strlen(C_DIAL_IN_PREFIX);
    
    // START Add 170303 by SMCHO
    if(C_MAX_REG_Q_SIZE < 10000) { C_MAX_REG_Q_SIZE = 100000; }
    if((C_REG_Q_ALM_LIMIT == 0) || ( C_REG_Q_ALM_LIMIT > C_MAX_REG_Q_SIZE)) {  C_REG_Q_ALM_LIMIT = C_MAX_REG_Q_SIZE * 0.95; }
    
    
    
#if 0   // TB 테스트용으로 LV1을 풀었음
    if(C_LOG_LEVEL == LOG_LV1) { C_LOG_LEVEL = LOG_LV2; }   // 160823 - Log Level 1으로 시작할 수 없게 변경 (운용자 실수 방지)
    Log.set_level(C_LOG_LEVEL);
#else
    if(C_LOG_LEVEL == LOG_LV1) { Log.set_debug(); }
    else                        { Log.set_level(C_LOG_LEVEL); }
#endif
    Log.set_max_limit(C_LOG_MAX_SIZE*1000*1000);  // LOG_MAX_SIZE (MByte)
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_ReadConfigFile
 * CLASS-NAME     : -
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : spy.cfg 파일을 변수(구조체)로 읽어들임
 * REMARKS        :
 **end*******************************************************/
bool SPY_ReadConfigFile(void)
{
	bzero(&spy_cfg, sizeof(SPY_CFG));
    
    cfg.LoadFile(g_strCfgFile);     // Load Config File to Memory

    // [DEBUG] Section
    if(cfg.GetInt ("DEBUG", "LOG_MAX_SIZE",   (int *)&C_LOG_MAX_SIZE) == false) { C_LOG_MAX_SIZE   = 1024; }
    if(cfg.GetInt ("DEBUG", "LOG_LEVEL",      (int *)&C_LOG_LEVEL)    == false) { C_LOG_LEVEL      = 4; }
    if(cfg.GetBool("DEBUG", "LOG_OPTIONS",    &C_LOG_OPTIONS)         == false) { C_LOG_OPTIONS    = false; }
    if(cfg.GetBool("DEBUG", "LOG_REGISTER",   &C_LOG_REGISTER)        == false) { C_LOG_REGISTER   = false; }
    if(cfg.GetBool("DEBUG", "OUTBOUND_PROXY", &C_OUTBOUND_PROXY)      == false) { C_OUTBOUND_PROXY = false; }
    if(cfg.GetStr ("DEBUG", "OUTBOUND_IP", C_OUTBOUND_IP, sizeof(C_OUTBOUND_IP)) == false) { C_OUTBOUND_IP[0] = '\0'; }
    
    // [MODULE] Section
    if(cfg.GetStr ("MODULE", "ORIG_STACK_IP",   C_ORIG_STACK_IP, sizeof(C_ORIG_STACK_IP)) == false) { C_ORIG_STACK_IP[0] = '\0'; }
    if(cfg.GetStr ("MODULE", "TERM_STACK_IP",   C_TERM_STACK_IP, sizeof(C_TERM_STACK_IP)) == false) { C_TERM_STACK_IP[0] = '\0'; }
    if(cfg.GetBool("MODULE", "HASH_FAIL_ROUTE",&C_HASH_FAIL_ROUTE) == false) { C_HASH_FAIL_ROUTE = false; }
    if(cfg.GetBool("MODULE", "BYE_NO_HASH_200",&C_BYE_NO_HASH_200) == false) { C_BYE_NO_HASH_200 = false; }
//    if(cfg.GetBool("MODULE", "USE_SCM_MAP",    &C_USE_SCM_MAP)     == false) { C_USE_SCM_MAP     = true;  }
    if(cfg.GetInt ("MODULE", "REPORT_TIME",     (int *)&C_REPORT_TIME)     == false) { C_REPORT_TIME     = 300; }
    if(cfg.GetInt ("MODULE", "ALARM_SEND_TIME", (int *)&C_ALARM_SEND_TIME) == false) { C_ALARM_SEND_TIME = 30; }    // Add 20170306
    
    if(cfg.GetInt ("MODULE", "SCM_MAP_AUDIT_TIME", (int *)&C_SCM_MAP_AUDIT_TIME) == false) { C_SCM_MAP_AUDIT_TIME = 3; }
    
#ifdef IPV6_MODE
    if(cfg.GetStr ("MODULE", "ORIG_STACK_IP_V6", C_ORIG_STACK_IP_V6, sizeof(C_ORIG_STACK_IP_V6)) == false) { C_ORIG_STACK_IP_V6[0] = '\0'; }
    if(cfg.GetStr ("MODULE", "TERM_STACK_IP_V6", C_TERM_STACK_IP_V6, sizeof(C_TERM_STACK_IP_V6)) == false) { C_TERM_STACK_IP_V6[0] = '\0'; }
    if(cfg.GetBool("MODULE", "IP_MODE_V6",      &C_IP_MODE_V6)      == false) { C_IP_MODE_V6      = false; }
#endif
    if(cfg.GetInt ("MODULE", "ORIG_STACK_PORT", &C_ORIG_STACK_PORT) == false) { C_ORIG_STACK_PORT = 5060; }
    if(cfg.GetInt ("MODULE", "TERM_STACK_PORT", &C_TERM_STACK_PORT) == false) { C_TERM_STACK_PORT = 5070; }
    if(cfg.GetInt ("MODULE", "SSW_PORT",        &C_SSW_PORT)        == false) { C_SSW_PORT        = 5060; }
    if(cfg.GetInt ("MODULE", "SAM_PORT",        &C_SAM_PORT)        == false) { C_SAM_PORT        = 5090; }//MIN ADD 20210415
    if(cfg.GetInt ("MODULE", "NETWORK_QOS",     &C_NETWORK_QOS)     == false) { C_NETWORK_QOS     = 160; }
    if(cfg.GetInt ("MODULE", "MAX_REG_Q_SIZE",  &C_MAX_REG_Q_SIZE)  == false) { C_MAX_REG_Q_SIZE  = 100000; }       // add 20170214
    if(cfg.GetInt ("MODULE", "REG_Q_ALM_LIMIT", &C_REG_Q_ALM_LIMIT) == false) { C_REG_Q_ALM_LIMIT = 0; }            // add 20170303
    
    if(cfg.GetBool("MODULE", "SYNTAX_CHECK",    &C_SYNTAX_CHECK)    == false) { C_SYNTAX_CHECK    = false; }
#ifdef ECS_MODE
    if(cfg.GetBool("MODULE", "ECS_UPDATE",      &C_ECS_UPDATE)      == false) { C_ECS_UPDATE      = false; }
#endif
    if(cfg.GetBool("MODULE", "AS_BLOCK",        &C_AS_BLOCK)        == false) { C_AS_BLOCK        = false; }
    
    // [THREAD] Section
    
    if(cfg.GetInt ("THREAD", "MAX_RSSW_ORIG_THREAD", &C_MAX_RSSW_ORIG_THREAD) == false) { C_MAX_RSSW_ORIG_THREAD = 2; }
    if(cfg.GetInt ("THREAD", "MAX_RSSW_TERM_THREAD", &C_MAX_RSSW_TERM_THREAD) == false) { C_MAX_RSSW_TERM_THREAD = 2; }
    if(cfg.GetInt ("THREAD", "MAX_RSSW_REG_THREAD",  &C_MAX_RSSW_REG_THREAD)  == false) { C_MAX_RSSW_REG_THREAD  = 1; }
    if(cfg.GetInt ("THREAD", "MAX_RSSW_OPT_THREAD",  &C_MAX_RSSW_OPT_THREAD)  == false) { C_MAX_RSSW_OPT_THREAD  = 1; }
    if(cfg.GetInt ("THREAD", "MAX_RSAM_SES_THREAD",  &C_MAX_RSAM_SES_THREAD)  == false) { C_MAX_RSAM_SES_THREAD  = 2; }

    // [SES] Section
    if(cfg.GetInt ("SES", "SES_BOARD",          &C_SES_BOARD)    == false) { C_SES_BOARD    = MAX_SES_BOARD; }
//// 20240123 bible : ONE Server - Multi SES
//#ifndef ONE_SERVER_MULTI_SES
    if(cfg.GetInt ("SES", "SES_PORT",           &C_SES_PORT)    == false)  { C_SES_PORT     = 5060; }
//#endif
    if(cfg.GetStr ("SES", "SES_00", C_SES_IP[0], MAX_IPADDR_LEN) == false) { C_SES_IP[0][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//	if(cfg.GetInt ("SES", "SES_00_PORT", 		&C_SES_PORT[0])	== false) { C_SES_PORT[0] = 6060; }
//#endif
    if(cfg.GetStr ("SES", "SES_01", C_SES_IP[1], MAX_IPADDR_LEN) == false) { C_SES_IP[1][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//	if(cfg.GetInt ("SES", "SES_01_PORT",        &C_SES_PORT[1]) == false) { C_SES_PORT[1] = 6061; }
//#endif
    if(cfg.GetStr ("SES", "SES_02", C_SES_IP[2], MAX_IPADDR_LEN) == false) { C_SES_IP[2][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_02_PORT",        &C_SES_PORT[2]) == false) { C_SES_PORT[2] = 6062; }
//#endif
    if(cfg.GetStr ("SES", "SES_03", C_SES_IP[3], MAX_IPADDR_LEN) == false) { C_SES_IP[3][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_03_PORT",        &C_SES_PORT[3]) == false) { C_SES_PORT[3] = 6063; }
//#endif
    if(cfg.GetStr ("SES", "SES_04", C_SES_IP[4], MAX_IPADDR_LEN) == false) { C_SES_IP[4][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_04_PORT",        &C_SES_PORT[4]) == false) { C_SES_PORT[4] = 6064; }
//#endif
    if(cfg.GetStr ("SES", "SES_05", C_SES_IP[5], MAX_IPADDR_LEN) == false) { C_SES_IP[5][0] = '\0'; }
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_05_PORT",        &C_SES_PORT[5]) == false) { C_SES_PORT[5] = 6065; }
//#endif
    if(cfg.GetStr ("SES", "SES_06", C_SES_IP[6], MAX_IPADDR_LEN) == false) { C_SES_IP[6][0] = '\0'; }       // 160831 by SMCHO (Max SES 6 -> 10)
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_06_PORT",        &C_SES_PORT[6]) == false) { C_SES_PORT[6] = 6066; }
//#endif
    if(cfg.GetStr ("SES", "SES_07", C_SES_IP[7], MAX_IPADDR_LEN) == false) { C_SES_IP[7][0] = '\0'; }       // 160831 by SMCHO (Max SES 6 -> 10)
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_07_PORT",        &C_SES_PORT[7]) == false) { C_SES_PORT[7] = 6067; }
//#endif
    if(cfg.GetStr ("SES", "SES_08", C_SES_IP[8], MAX_IPADDR_LEN) == false) { C_SES_IP[8][0] = '\0'; }       // 160831 by SMCHO (Max SES 6 -> 10)
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_08_PORT",        &C_SES_PORT[8]) == false) { C_SES_PORT[8] = 6068; }
//#endif
    if(cfg.GetStr ("SES", "SES_09", C_SES_IP[9], MAX_IPADDR_LEN) == false) { C_SES_IP[9][0] = '\0'; }       // 160831 by SMCHO (Max SES 6 -> 10)
//// 20240123 bible : ONE Server - Multi SES
//#ifdef ONE_SERVER_MULTI_SES
//    if(cfg.GetInt ("SES", "SES_09_PORT",        &C_SES_PORT[9]) == false) { C_SES_PORT[9] = 6069; }
//#endif

    // [ETC] Section
    if(cfg.GetStr ("ETC", "DIAL_IN_PREFIX",         C_DIAL_IN_PREFIX, sizeof(C_DIAL_IN_PREFIX))    == false) { C_DIAL_IN_PREFIX[0] = '\0'; }
    if(cfg.GetBool("ETC", "TAS_BLOCK_480_FLAG",     &C_TAS_BLOCK_480_FLAG)     == false) { C_TAS_BLOCK_480_FLAG     = false; }
    if(cfg.GetBool("ETC", "DEL_18X_SUPPORTED_FLAG", &C_DEL_18X_SUPPORTED_FLAG) == false) { C_DEL_18X_SUPPORTED_FLAG = false; }
    if(cfg.GetBool("ETC", "REFER_405",              &C_REFER_405)              == false) { C_REFER_405              = false; }
    if(cfg.GetBool("ETC", "BLACK_LIST_USE",         &C_BLACK_LIST_USE)         == false) { C_BLACK_LIST_USE         = false; }
    if(cfg.GetStr ("ETC", "BLACK_LIST_NAME",    C_BLACK_LIST_NAME,    sizeof(C_BLACK_LIST_NAME))    == false) { C_BLACK_LIST_NAME[0]   = '\0'; }
#ifdef IPV6_MODE
    if(cfg.GetStr ("ETC", "BLACK_LIST_NAME_V6", C_BLACK_LIST_NAME_V6, sizeof(C_BLACK_LIST_NAME_V6)) == false) { C_BLACK_LIST_NAME_V6[0] = '\0'; }
#endif
    if(cfg.GetStr ("ETC", "WEB_EXTLIST_NAME",    C_WEB_EXTLIST_NAME,    sizeof(C_WEB_EXTLIST_NAME))    == false) { strcpy(C_WEB_EXTLIST_NAME, "/home/mcpas/cfg/extlist.cfg"); }
#ifdef IPV6_MODE
    if(cfg.GetStr ("ETC", "WEB_EXTLIST_NAME_V6", C_WEB_EXTLIST_NAME_V6, sizeof(C_WEB_EXTLIST_NAME_V6)) == false) { strcpy(C_WEB_EXTLIST_NAME_V6, "/home/mcpas/cfg/extlist_v6.cfg"); }
#endif
    
    
    if(cfg.GetBool("ETC", "ADD_UPDATE_TIMER",       &C_ADD_UPDATE_TIMER)       == false) { C_ADD_UPDATE_TIMER       = false; }
    if(cfg.GetBool("ETC", "ADD_200_UPDATE_TIMER",   &C_ADD_200_UPDATE_TIMER)   == false) { C_ADD_200_UPDATE_TIMER   = false; }
    if(cfg.GetBool("ETC", "ADD_200_INVITE_TIMER",   &C_ADD_200_INVITE_TIMER)   == false) { C_ADD_200_INVITE_TIMER   = false; }
    if(cfg.GetStr ("ETC", "TAS_REGISTER_SKEY", C_TAS_REGISTER_SKEY, sizeof(C_TAS_REGISTER_SKEY)) == false) { strcpy(C_TAS_REGISTER_SKEY, "mmtel,V_UMC"); }
    if(cfg.GetInt ("ETC", "OVERLOAD_RESP_CODE",     &C_OVERLOAD_RESP_CODE)     == false) { C_OVERLOAD_RESP_CODE     = 503; }
	if(cfg.GetBool("ETC", "USE_MESSAGE",			&C_USE_MESSAGE)			   == false) { C_USE_MESSAGE = true; }	// 20170725
	if(cfg.GetBool("ETC", "REGI_API_4",				&C_USE_REGI_API4)		   == false) { C_USE_MESSAGE = false; }	//[JIRA AS-211]
    
    /*
     * Post Config 처리
     */
    SPY_AdjustConfigValue();
    SPY_PrintOutConfig();
    
    return(true);
}

