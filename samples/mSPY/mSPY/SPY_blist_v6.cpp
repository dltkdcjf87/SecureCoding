/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_blist_v6.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V5.X
 DATE           : 2016/03/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : IPv6 Black List 파일을 읽어서 처리하고 관리하는 Class 함수
 REMARKS        : Contact에 있는 IP 주소를 기준으로 Black List 처리
 *end*************************************************************/

#include "SPY_blist_v6.h"
#include "SPY_libv6.h"

extern NSM_LOG   Log;
extern void LIB_delete_special_character(char *str, int value);

#pragma mark -
#pragma mark 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : BLIST_TBL_V6
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : BLIST_TBL_V6 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
BLIST_TBL_V6::BLIST_TBL_V6(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    bzero(m_filename, sizeof(m_filename));

    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~BLIST_TBL_V6
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : BLIST_TBL_V6 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
BLIST_TBL_V6::~BLIST_TBL_V6(void)
{
    clear();
    
    pthread_mutex_destroy(&lock_mutex);
    pthread_mutex_destroy(&file_mutex);
}


#pragma mark -
#pragma mark MAP 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 List 를 삭제
 * REMARKS        :
 **end*******************************************************/
void BLIST_TBL_V6::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_BlackListTable, sizeof(m_BlackListTable));
        m_nBlockListCount = 0;
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      :
 * RET. VALUE     : N
 * DESCRIPTION    : TABLE에 등록된 Black List 수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL_V6::size(void)
{
    return(m_nBlockListCount);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : print_log
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 메모리로 읽어들인 Black List 정보를 로그파일로 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
void BLIST_TBL_V6::print_log(void)
{
    Log.printf(LOG_INF, "==============================================\n");
    Log.printf(LOG_INF, " V6 BLACK LIST FILENAME = %s\n", m_filename);
    Log.printf(LOG_INF, "    SIZEOF BLACK LIST   = %d\n", m_nBlockListCount);
    Log.printf(LOG_INF, "==============================================\n");

    for(int i = 0 ; i < m_nBlockListCount; i++ )
    {
        char    buf[128];
        
        inet_ntop(AF_INET6, (void *)&m_BlackListTable[i], buf, 64);
        Log.printf(LOG_INF, "  [%03d] [%s]\n", i, buf);
    }
    
    Log.printf(LOG_INF, "==============================================\n");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read_again
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MMC/WEB 요청에 의해 Black List를 다시 읽어 들이는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL_V6::read_again(void)
{
    clear();            // 기존  Table 삭제
    return(read());     // Black List File Read
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read_again
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER    IN: fname - Black List 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MMC/WEB 요청에 의해 Black List를 다시 읽어 들이는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL_V6::read_again(const char *fname)
{
    clear();            // 기존  Table 삭제
    return(read(fname));     // Black List File Read
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER    IN: fname - Black List 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Black List 파일을 읽어서 TABLE에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL_V6::read(const char *fname)
{
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[BLIST_TBL_V6] filename length is too big >= %d [%s]\n", sizeof(m_filename), fname);
        return(BLIST_FILENAME_ERR);
    }
    
    strcpy(m_filename, fname);        // store filename(/.../extlist.cfg)
    return(read());
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : int
 * DESCRIPTION    : Black List 파일을 읽어서 TABLE에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL_V6::read(void)
{
    FILE        *fp;
    char        buf[256];
    int         nIndex_V6 = 0;

    if(strlen(m_filename) < 1)
    {
        Log.printf(LOG_ERR, "[BLIST_TBL_V6] filename Error(%s)\n", m_filename);
        return(BLIST_FILENAME_ERR);
    }
    
    pthread_mutex_lock(&file_mutex);
    {
        if((fp = fopen(m_filename, "r")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[BLIST_TBL_V6] Can't Open %s file\n", m_filename);
            return(BLIST_FILEOPEN_ERR);
        }
        
        while(fgets(buf, 255, fp))
        {
            LIB_delete_special_character(buf, '#');     // comment 이후 부분 삭제
            LIB_delete_white_space(buf);                // 공백 제거
            
            if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
            if(strlen(buf) == 0) { continue; }                              // skip empty line
            if((buf[0] == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
            
            if (LIB_isIPv6(buf) == true)
            {
                if(inet_pton(AF_INET6, buf, (void *)&m_BlackListTable[nIndex_V6]) == 1) // 1 is OK
                {
                    if (++nIndex_V6 >= MAX_BLACK_LIST)
                    {
                        Log.printf(LOG_ERR, "[BLIST_TBL_V6] m_BlackListTable[] Table Full(%d, %d)\n", nIndex_V6, MAX_BLACK_LIST);
                        break;
                    }
                }
                else
                {
                    Log.printf(LOG_ERR, "[BLIST_TBL_V6] inet_pton(%s) Error(%d)\n", buf, errno);
                    continue;
                }
            }
            else
            {
                Log.printf(LOG_ERR, "[BLIST_TBL_V6] not IPv6(%s) Error()\n", buf);
                continue;
            }
        }
        
        m_nBlockListCount = nIndex_V6;
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
    print_log();
    return(BLIST_OK);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_black_list
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER    IN: ip6addr - Black List 조회할 ip6addr
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 ip6addr가 Black List인지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool BLIST_TBL_V6::is_black_list(struct in6_addr ip6addr)
{
    char	szIP[64];
    IP_TYPE		nIPType;

    nIPType = LIB_get_ip_type(ip6addr, szIP, sizeof(szIP));
    
    if (nIPType != IP_TYPE_V6) { return(false); }
    
    for(int i = 0; i < m_nBlockListCount; i ++)
    {
        // 20160321 bible - IPv6 Address 비교 로직 수정 - macro 사용
#ifdef __APPLE__
        if (IN6_ARE_ADDR_EQUAL(&ip6addr, &m_BlackListTable[i])) { return(true); }
#else
        if (IN6_ARE_ADDR_EQUAL(ip6addr.s6_addr32, m_BlackListTable[i].s6_addr32)) { return(true); }
#endif
    }
    
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_black_list
 * CLASS-NAME     : BLIST_TBL_V6
 * PARAMETER    IN: strIP - Black List 조회할 IP
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 strIP가 Black List인지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool BLIST_TBL_V6::is_black_list(const char *strIP)
{
    struct in6_addr v6_addr;
    
    if(inet_pton(AF_INET6, strIP, (void *)&v6_addr) != 1) // 1 is OK
    {
        return(false);
    }
    
    for(int i = 0; i < m_nBlockListCount; i ++)
    {
#ifdef __APPLE__
        if (IN6_ARE_ADDR_EQUAL(&v6_addr, &m_BlackListTable[i])) { return(true); }
#else
        if (IN6_ARE_ADDR_EQUAL(v6_addr.s6_addr32, m_BlackListTable[i].s6_addr32)) { return(true); }
#endif
    }
    
    return(false);
}
