
#ifndef __SPY_EXTLIST_H__
#define __SPY_EXTLIST_H__

#include <iostream>
#include <map>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "btxbus3.h"
#include "libnsmcom.h"
#include "libnsmlog.h"

using namespace std;

#define MAX_EXTSVR_LIST         200
#define MAX_EXTSVR_IP_LEN       63

typedef struct
{
    char    sswid[4];       // SSW ID (3자리)
    char    ip[MAX_EXTSVR_IP_LEN+1];
#ifdef IPV6_MODE
    struct in6_addr v6_addr;
#endif
    bool    block;
    char    monitor;
    char    fault;
    long    lastTime;
    int     checkTime;
    char    tryCount;
    char    recvCount;
} EXTSVR_LIST;

#define MAX_ERRSVR_LIST     30


/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : EXTLIST_TBL
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : extlist.cfg 파일을 읽어서 처리하고 관리하는 Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class EXTLIST_TBL
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for extlist.cfg file access
    
    char            m_filename[256];
    EXTSVR_LIST     m_ExtSvrList[MAX_EXTSVR_LIST];  // External Server List
    int             m_nExtSvrListCount;

    bool    insert_file(EXTSVR_LIST *extSvr);   // extlist.cfg file에 insert
    bool    update_file(EXTSVR_LIST *extSvr);   // extlist.cfg file에 update
    bool    erase_file(EXTSVR_LIST *extSvr);    // extlist.cfg file에서 delete
    bool    backup_file(void);                  // 파일 작업전 backup하는 함수
    void    print_log(void);
    
public:
    EXTLIST_TBL(void);
	~EXTLIST_TBL(void);
    
	void    clear(void);                        // TABLE을 초기화 하는 함수
	int     size(void);                         // TABLE에 등록된 SSW 개수를 구하는 함수
	bool    read(const char *fname);            // extlist.cfg 파일을 읽어서 TABLE에 저장하는 함수
    bool    reread(void);                       // 현재 내용을 지우고 다시 읽어들이는 함수
	void	check_configure_file(char* _fileName, int _smallestSize,char* _returnResult); // 2019.01.21 dw.choi - check whether file size is too small
    bool    reread_from_WEB(const char *web_extlist_filename); // 현재 내용을 지우고 web에서 업로드한 파일로 다시 읽어들이는 함수

    bool    exist(EXTSVR_LIST *extSvr);         // 해당 IP의 External Server가 등록되어 있는지 여부를 확인하는 함수
    
    bool    insert(EXTSVR_LIST *extSvr, bool bInsertFile);  // TABLE에 External Server를 등록
    bool    select(const char *strIP, bool *bBlock);        // TABLE에 External Server BLOCK여부를 SELECT

    bool    update(EXTSVR_LIST *extSvr);        // TABLE에 External Server 정보를 UPDATE
    bool    erase(EXTSVR_LIST *extSvr);         // TABLE에서 해당 SSW 삭제
    bool    all_alarm_off(void);                // 모든 ALARM을 OFF
    bool    receive_options(const char *strIP, int ot_type); // OPTIONS 메세지 수신 처리를 하는 함수
    void    is_options_received(void);          // OPTIONS가 수신되었는지 여부를 확인하는 함수
    void    update_tbl_ssw_config(void);        // ORACLE 에 Ext.Srver 상태를 UPDATE하는 함수
    bool    save_file(void);                    // 메모리의 내용을 extlist.cfg file에서 write
    
    bool    add_unreg_count(const char *strIP); // 등록되지 않은 서버에서 메세지를 수신한 정보를 기록하는 함수
    bool    add_msgerr_count(const char *strIP); // 잘못된 SIP 메세지를 보낸 SSW(MS,...) 정보를 기록하는 함수
};

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : EXTLIST_TBL_V6
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : extlist_v6.cfg 파일을 읽어서 처리하고 관리하는 Class
 *                :
 * REMARKS        : IPv6용으로 EXTLIST_TBL과는 파일내에서 사용하는 구분자가 다르다
 *                : V4 = ';', V6 = ','
 **end*******************************************************/
class EXTLIST_TBL_V6
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for extlist.cfg file access
    
    char            m_filename[256];
    EXTSVR_LIST     m_ExtSvrList[MAX_EXTSVR_LIST];  // External Server List
    int             m_nExtSvrListCount;
    
    bool    insert_file(EXTSVR_LIST *extSvr);   // extlist.cfg file에 insert
    bool    update_file(EXTSVR_LIST *extSvr);   // extlist.cfg file에 update
    bool    erase_file(EXTSVR_LIST *extSvr);    // extlist.cfg file에서 delete
    bool    backup_file(void);                  // 파일 작업전 backup하는 함수
    void    print_log(void);
    
public:
    EXTLIST_TBL_V6(void);
    ~EXTLIST_TBL_V6(void);
    
    void    clear(void);                        // TABLE을 초기화 하는 함수
    int     size(void);                         // TABLE에 등록된 SSW 개수를 구하는 함수
    bool    read(const char *fname);            // extlist.cfg 파일을 읽어서 TABLE에 저장하는 함수
    bool    reread(void);                       // 현재 내용을 지우고 다시 읽어들이는 함수
    bool    reread_from_WEB(const char *web_extlist_filename); // 현재 내용을 지우고 web에서 업로드한 파일로 다시 읽어들이는 함수
    bool    exist(EXTSVR_LIST *extSvr);         // 해당 IP의 External Server가 등록되어 있는지 여부를 확인하는 함수
    
    bool    insert(EXTSVR_LIST *extSvr, bool bInsertFile);  // TABLE에 External Server를 등록
    bool    select(struct in6_addr *v6_addr, bool *bBlock);        // TABLE에 External Server BLOCK여부를 SELECT

    bool    update(EXTSVR_LIST *extSvr);        // TABLE에 External Server 정보를 UPDATE
    bool    erase(EXTSVR_LIST *extSvr);         // TABLE에서 해당 SSW 삭제
    bool    all_alarm_off(void);                // 모든 ALARM을 OFF
    bool    receive_options(struct in6_addr *v6_addr, int ot_type); // OPTIONS 메세지 수신 처리를 하는 함수
    void    is_options_received(void);          // OPTIONS가 수신되었는지 여부를 확인하는 함수
    void    update_tbl_ssw_config(void);        // ORACLE 에 Ext.Srver 상태를 UPDATE하는 함수
    bool    save_file(void);                    // 메모리의 내용을 extlist.cfg file에서 write
    
    bool    add_unreg_count(const char *strIP); // 등록되지 않은 서버에서 메세지를 수신한 정보를 기록하는 함수
    bool    add_msgerr_count(const char *strIP); // 잘못된 SIP 메세지를 보낸 SSW(MS,...) 정보를 기록하는 함수
};


typedef struct
{
    char    strIP[64];          // SIP 메세지를 보내온 등록되지 않은 외부 서버 또는 잘못된 메세지를 보낸 외부서버 IP (IPv6를 고려 64byte로)
    int     nUnRegCount;        // 등록되지 않은 외부 서버에서  SIP 메세지를 수신한 횟수
    int     nMsgErrCount;       // 등록되어 있지만 잘못된 메세지를 보낸 횟수
} ERRSVR_LIST;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : ERRSVR_TBL
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : 문제가 있는 외부 연동 서버를 관리하는 Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class ERRSVR_TBL
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    ERRSVR_LIST     m_ErrSvrList[MAX_ERRSVR_LIST];  // ERROR Service List

public:
    ERRSVR_TBL(void);
	~ERRSVR_TBL(void);
    
	void    clear(void);                        // TABLE을 초기화 하는 함수
    bool    add_unreg(const char *strIP);       // 등록되지 않은 서버에서 메세지를 수신한 정보를 기록하는 함수
    bool    add_msgerr(const char *strIP);      // 잘못된 SIP 메세지를 보낸 SSW(MS,...) 정보를 기록하는 함수
    void    report_to_ems(void);
};

#endif /* defined(__SPY_EXTLIST_H__) */

