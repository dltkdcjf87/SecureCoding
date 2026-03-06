//
//  SPY_trace.h
//  smSPY
//
//  Created by SMCHO on 2014. 4. 8..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __SPY_TRACE_H__
#define __SPY_TRACE_H__

#include <iostream>
#include <map>
#include <string.h>
#include <sys/time.h>

#include "libnsmcom.h"

#ifndef OT_TYPE_ORIG
#   define OT_TYPE_ORIG        0
#   define OT_TYPE_TERM        1
#endif

#define MAX_TRACE_LIST         10

#define TRACE_OK                    0
#define TRACE_ERROR_DUPLICATED      1
#define TRACE_ERROR_TABLE_FULL      2

#define TRACE_FILE_FORMAT   "/home/mcpas/log/spy/trace_%04d%02d%02d.txt"

typedef struct
{
	char    strFrom[32];
	char    strTo[32];
	char    strCall_ID[256];
	char    strSvcKey[64];
	int     nMaxCount;
	int     nCount;
    int     nBoardNo;       // SES Board Number
    int     nChannelNo;     // SES Channel Number
} TRACE_LIST;

using namespace std;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : TRACE_TBL
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : Call Trace Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class TRACE_TBL
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for file access

    TRACE_LIST      m_TraceList[MAX_TRACE_LIST];
    int             m_nTraceListCount;
    int             m_ActiveCount;              // MaxCount 까지 Trace가 끝난 List를 제외한 실제 동작 중인 List Count
                                                // Max까지 처리하더라도 LIST를 삭제하지 않기 때문에(WEB에서 보여주기 위해)
                                                // m_nTraceListCount외에 m_ActiveCount를 별도로 사용 함
	
    void    get_trace_filename(char *fname);    // TRACE를 저장할 Filename
    void    erase_file();       // extlist.cfg file에서 delete
    void    recount_list();     // m_nTraceListCount 를 다시 계산
    bool    already_reg(TRACE_LIST *trace);    // 동일한 TRACE가 등록되어 있는지 확인하는 함수
    
public:
    TRACE_TBL(void);
	~TRACE_TBL(void);
    
	void    clear(void);                            // table을 초기화 하는 함수
	int     size(void);                             // table에 등록된 Trace 개수를 구하는 함수
    bool    insert(TRACE_LIST *trace, int *nIndex); // table 에 Trace 정보를 등록
    bool    erase(int nTrace);                      // table 에서 Trace 정보 삭제
    int     exist(const char *strFrom, const char *strTo, const char *strSvcKey, int nBoard, int nChannel);
            // 해당 Trace가 등록되어 있는지 여부를 확인하는 함수
    
    int     check(int nBoardNo, int nChannel);  // 해당 board/channel이 Trace 중인지 확인하는 함수
    bool    trace(int nTrace, int nBoardNo, int nChannelNo, int ot_type, const char *strToIP, int nToPort, const char *strSendBuf);
            // SIP 메세지를 Trace File에 저장하는 함수
    int     sprint_trace(char *strTrace);       // Trace List룰 strTrace애 츌력하는 함수
    bool    get_trace(int nTrace, char *strFrom, char *strTo, char *strSvcKey, int *nMaxCount, int *nCount);
};


#endif /* defined(__SPY_TRACE_H__) */
