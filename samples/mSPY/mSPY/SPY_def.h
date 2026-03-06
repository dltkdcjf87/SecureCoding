
#ifndef __SPY_DEF_H__
#define __SPY_DEF_H__

#if (TAS_MODE && MPBX_MODE)
#   error "can't set TAS_MODE & MPBX_MODE simulaneously"
#endif

#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// 20241226 kdh add for test
#include <atomic>

#include "typedef.h"
#include "btxbus3.h"
#include "MMC_head.h"


#define SPY_SERVER_THREAD_PRIORITY  14

// SSW I/F  관련 ORIG/TERM Server ID
#define OT_TYPE_ORIG        0
#define OT_TYPE_TERM        1


//#define SAM_UDP_PORT	 	5060
#define ASCII_SPACE         0x20
#define ASCII_CRLF          "\r\n"

#define msleep(n)           usleep(n*1000)


/* MAX Definitions */
#define MAX_SES_BOARD       10              // 160831 by SMCHO (Max SES 6 -> 10)
#define MAX_SES_THREAD      64
#define MAX_SES_CH          100000          // 20240909 sangchul - chg :: 30000 -> 100000 
#define MAX_DN_LEN          31
#define MAX_FNAME_LEN       127
#ifdef IPV6_MODE
#   define MAX_IPADDR_LEN      47
#else
#   define MAX_IPADDR_LEN      31
#endif
//#define MAX_BUF_SIZE		4096

#define MAX_LINE_LEN        998		// 1라인 최대 길이 - EXCEPT_CRLF RFC 2822

#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
#else
	#define MSG_ID_SES_HA_START  0x0025
#endif
#endif



#define TIME_CHECK_SSW_EXT_STATUS_CHECK     15      // 15 SEC 마다 SSW/EXT 상태 변화를 Check

//#define SSW_UDPSERVER_PORT 	5060
#define SAM_UDP_SERVER_PORT     5090


#include "libnet.h"


#include "libnsmcom.h"
#include "libnsmlog.h"
#include "libnsmcfg.h"
#include "libnsmptrq.h"

//#include "SPY_cr.h
#include "SPY_hash.h"
#include "SPY_mmc.h"
#include "SPY_mmc2.h"
#include "SPY_cfg.h"        // Config 관련 헤더파일
#include "SPY_stat.h"       // 통계 관련 헤더파일
#include "SPY_alm.h"        // ALARM/FAULT 관련 헤더파일
#include "SPY_xbus.h"       // XBUS 관련 헤더파일
//#include "SPY_sswlist.h"    // sswlist.cfg 처리 class관련 헤더파일
#include "SPY_extlist.h"    // extlist.cfg 처리 class관련 헤더파일
#include "SPY_svckey.h"     // service_key.cfg 처리 class관련 헤더파일
#include "SPY_trace.h"      // Call Trace 처리 class관련 헤더파일
#include "SPY_blist.h"      // Black List 처리 class관련 헤더파일
#include "SPY_chmap.h"
#ifdef IPV6_MODE
#   include "SPY_libv6.h"
#   include "SPY_blist_v6.h"      // Black List 처리 class관련 헤더파일
#endif
#ifdef INCLUDE_REGI
#   include "SPY_regi.h"
#endif

#ifdef FAULT_LOG_MODE //v4.4.2-1.1.0 MYUNG 20230906
#include "flcm_manager.h"
#endif


#ifdef	SOIP_REGI
#define MAX_MSG_SIZE 8192

typedef struct _SRM_Head
{
    char        nCode;
    char        nVersion;
    uint16_t  nBoardNo;
    uint16_t  nChannelNo;
    uint16_t  nMessageId;
    size_t    nLowCallId;
    size_t    nHighCallId;
} SRM_Head;

typedef struct
{
    SRM_Head    head;
    char        body[MAX_MSG_SIZE];
} sSAMtoSRM;
#endif

#endif  //  __SPY_DEF_H__

