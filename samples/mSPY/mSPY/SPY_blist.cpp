/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_blist.cpp
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

#include "SPY_blist.h"
#include "SPY_libv6.h"

extern NSM_LOG   Log;
extern void LIB_delete_special_character(char *str, int value);

#pragma mark -
#pragma mark 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : BLIST_TBL
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : BLIST_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
BLIST_TBL::BLIST_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    bzero(m_filename, sizeof(m_filename));
#ifdef IPV6_MODE
//    bzero(m_filename_V6, sizeof(m_filename_V6));
#endif
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~BLIST_TBL
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : BLIST_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
BLIST_TBL::~BLIST_TBL(void)
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
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 List 를 삭제
 * REMARKS        :
 **end*******************************************************/
void BLIST_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_BlackListTable, sizeof(m_BlackListTable));
        m_nBlockListCount = 0;
        
#ifdef IPV6_MODE
//        bzero(&m_BlackListTable_V6, sizeof(m_BlackListTable_V6));
//        m_nBlockListCount_V6 = 0;
#endif
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      :
 * RET. VALUE     : N
 * DESCRIPTION    : TABLE에 등록된 Black List 수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL::size(void)
{
    return(m_nBlockListCount);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : print_log
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 메모리로 읽어들인 Black List 정보를 로그파일로 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
void BLIST_TBL::print_log(void)
{
    struct in_addr  ip;
    
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, " BLACK LIST FILENAME = %s\n", m_filename);
    Log.printf(LOG_INF, " SIZEOF BLACK LIST   = %d\n", m_nBlockListCount);
    Log.printf(LOG_INF, "============================================\n");
    
    for(int i = 0; i < m_nBlockListCount; i++)
	{
        ip.s_addr   = m_BlackListTable[i];
		Log.printf(LOG_INF, "  [%03d] %s\n", i, inet_ntoa(ip));
	}

    Log.printf(LOG_INF, "============================================\n");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read_again
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MMC/WEB 요청에 의해 Black List를 다시 읽어 들이는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL::read_again(void)
{
    clear();            // 기존  Table 삭제
    return(read());     // Black List File Read
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read_again
 * CLASS-NAME     : BLIST_TBL
  * PARAMETER    IN: fname - Black List 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MMC/WEB 요청에 의해 Black List를 다시 읽어 들이는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL::read_again(const char *fname)
{
    clear();            // 기존  Table 삭제
    return(read(fname));     // Black List File Read
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER    IN: fname - Black List 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Black List 파일을 읽어서 TABLE에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL::read(const char *fname)
{
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[BLIST_TBL] filename length is too big >= %d [%s]\n", sizeof(m_filename), fname);
        return(BLIST_FILENAME_ERR);
    }
    
    strcpy(m_filename, fname);        // store filename(/.../extlist.cfg)
    return(read());
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : int
 * DESCRIPTION    : Black List 파일을 읽어서 TABLE에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
int BLIST_TBL::read(void)
{
    FILE        *fp;
    char        buf[256];
    int         nIndex = 0;
    uint32_t    nIP;
    
    if(strlen(m_filename) < 1)
    {
        Log.printf(LOG_ERR, "[BLIST_TBL] filename Error(%s)\n", m_filename);
        return(BLIST_FILENAME_ERR);
    }
    
    pthread_mutex_lock(&file_mutex);
    {
        if((fp = fopen(m_filename, "r")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[BLIST_TBL] Can't Open %s file\n", m_filename);
            return(BLIST_FILEOPEN_ERR);
        }
        
        while(fgets(buf, 255, fp))
        {
            LIB_delete_special_character(buf, '#');     // comment 이후 부분 삭제
            LIB_delete_white_space(buf);                // 공백 제거
            if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
            if(strlen(buf) == 0) { continue; }                              // skip empty line
            if((buf[0] == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
            
            if((nIP = (uint32_t)inet_addr(buf)) == -1)  // IP string을 uint로 변환, 비정상 IP인 경우 -1이 리턴됨
            {
                Log.printf(LOG_ERR, "[BLIST_TBL] Black List IP Error(%s)\n", buf);
                continue;
            }

            m_BlackListTable[nIndex] = nIP;
            
            if(++nIndex >= MAX_BLACK_LIST)    // Table Full
            {
                Log.printf(LOG_ERR, "[BLIST_TBL] Black List Table Full(%d, %d)\n", nIndex, MAX_BLACK_LIST);
                break;
            }
        }
        
        m_nBlockListCount = nIndex;
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
    print_log();
	return(BLIST_OK);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_black_list
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER    IN: nIP - Black List 조회할 IP
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 nIP가 Black List인지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool BLIST_TBL::is_black_list(uint32_t nIP)
{
    if(nIP == -1) { return(false); }
    
    for(int i = 0; i < m_nBlockListCount; i ++)
	{
		if(m_BlackListTable[i] == nIP)
		{
			return(true);   // Black List
		}
	}

    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_black_list
 * CLASS-NAME     : BLIST_TBL
 * PARAMETER    IN: strIP - Black List 조회할 IP
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 strIP가 Black List인지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool BLIST_TBL::is_black_list(const char *strIP)
{
    uint32_t    nIP;
    
    if((nIP = inet_addr(strIP)) == -1) { return(false); }
    
    for(int i = 0; i < m_nBlockListCount; i ++)
	{
		if(m_BlackListTable[i] == nIP)
		{
			return(true);   // Black List
		}
	}
    
    return(false);
}


