
/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_blist.h
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V3.X
 DATE           : 2014/09/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : Black List 파일을 읽어서 처리하고 관리하는 Class 함수
 REMARKS        : Contact에 있는 IP 주소를 기준으로 Black List 처리
 *end*************************************************************/


#ifndef __SPY_BLIST_V6_H__
#define __SPY_BLIST_V6_H__

#include <iostream>
#include <map>

#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libnsmcom.h"
#include "libnsmlog.h"

#define MAX_BLACK_LIST      128      // MAX Black List Table Size

#define BLIST_OK            0
#define BLIST_FILEOPEN_ERR  1
#define BLIST_FILENAME_ERR  2
#define BLIST_IP_NOT_EXIST  3
#define BLIST_UNDEFINED_ERR 4

using namespace std;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : BLIST_TBL_V6
 * HISTORY        : 2016/06 - 최초 작성
 * DESCRIPTION    : Hosts Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class BLIST_TBL_V6
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for file access
    
    char            m_filename[256];                // Black List Filename
    struct in6_addr m_BlackListTable[MAX_BLACK_LIST];    // Black List List Memory
    int             m_nBlockListCount;              // m_BlackList에 등록된 IP 수
    void            print_log();                    // Black List File에서 읽어들인 Table을 Log파일로 출력

    
public:
    BLIST_TBL_V6(void);
    ~BLIST_TBL_V6(void);
    
    void    clear(void);                // table을 초기화 하는 함수
    int     size(void);                 // TABLE에 등록된 Black List 수를 구하는 함수
    
    int     read(void);                 // Black List 파일을 읽어서 TABLE에 저장하는 함수
    int     read(const char *fname);    // Black List 파일을 읽어서 TABLE에 저장하는 함수
    int     read_again(void);           // MMC/Web의 요청에 의해 Black List를 다시 읽어 들이는 함수
    int     read_again(const char *fname);
    bool    is_black_list(const char *strIP);   // 입력된 IP가 Black List인지 확인하는 함수
    bool    is_black_list(struct in6_addr ip6addr);        // 입력된 IP가 Black List인지 확인하는 함수

    bool    insert();          // table 에 Trace 정보를 등록
    bool    erase(int nTrace);                  // table 에서 Trace 정보 삭제
    
};


#endif /* __SPY_BLIST_V6_H__ */
