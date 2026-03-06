/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_svckey.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/04/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : service_key.cfg 파일을 읽어서 처리하고 관리하는 Class 함수
 REMARKS        :
 *end*************************************************************/

#include "SPY_svckey.h"
#include "libnsmlog.h"

extern NSM_LOG   Log;

//#pragma mark -
//#pragma mark 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SVCKEY_TBL
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SVCKEY_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
SVCKEY_TBL::SVCKEY_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    
    clear();
    m_nSvcKeyListCount = 0;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~SVCKEY_TBL
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SVCKEY_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
SVCKEY_TBL::~SVCKEY_TBL(void)
{
	clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : print_log
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 메모리로 읽어들인 SvcKey 정보를 로그파일로 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
void SVCKEY_TBL::print_log(void)
{
    
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "SERVICE_KEY CONFIG FILENAME = %s\n", m_filename);
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "#          [SERVICE_NAME]  =  [SERVICE_KEY]\n");
 
    for(int i = 0; i < m_nSvcKeyListCount; i++)
	{
		Log.printf(LOG_INF, "[%02d]  %20s =  %s\n", i, m_SvcKeyList[i].strSvcName, m_SvcKeyList[i].strSvcKey);
	}
    Log.printf(LOG_INF, "   Number of List = %d\n", m_nSvcKeyListCount);
    Log.printf(LOG_INF, "============================================\n");
}

//#pragma mark -
//#pragma mark service_key.cfg를 읽어서 TABLE에 INSERT하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER    IN: fname - service_key.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : service_key.cfg 파일을 읽어서 TABEL에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SVCKEY_TBL::read(const char *fname)
{
    FILE	*fp;
    char    buf[256], strSvcName[64], strSvcKey[64];
    int     nCount = 0;
    
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[SVCKEY_TBL] filename length is too big [%s]\n", fname);
        return(false);
    }
    
    strcpy(m_filename, fname);        // store full path(/.../service_key.cfg)
    
    if((fp = fopen(m_filename, "r")) == NULL)
	{
		Log.printf(LOG_ERR, "[SVCKEY_TBL] Can't Open %s file\n", m_filename);
		return(false);
	}
    
    while(fgets(buf, 255, fp))
    {
        LIB_delete_comment(buf, '#');       // comment 이후 부분 삭제
        LIB_delete_white_space(buf);        // 공백 제거
        
        if(strlen(buf) == 0) { continue; }                              // skip empty line
        if((*buf == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
        if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
        
        // SvcName=SvcKey
        if(LIB_split_string_into_2(buf, '=', strSvcName, strSvcKey) == false) { continue; }
        
        // Service 이름과 Service Key를 TABLE에 저장
        strcpy(m_SvcKeyList[nCount].strSvcName,  strSvcName);
		strcpy(m_SvcKeyList[nCount++].strSvcKey, strSvcKey);
    }
    
    m_nSvcKeyListCount = nCount;
    fclose(fp);
    
    print_log();
	return(true);
}

//#pragma mark -
//#pragma mark MAP 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void SVCKEY_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_SvcKeyList, sizeof(SVCKEY_LIST)*MAX_SVCKEY_LIST);
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER      : -
 * RET. VALUE     : TABLE에 등록된 Service Key List 수
 * DESCRIPTION    : 전체 Service Key List 수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int SVCKEY_TBL::size(void)
{
	return(m_nSvcKeyListCount);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : get_service_key
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER    IN: strRURI       - SIP Request URI
 *             OUT: strServiceKey - RURI로 분석한 ServiceKey
 * RET. VALUE     : BOOL
 * DESCRIPTION    : RURI를 분석하여 ServiceKey를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SVCKEY_TBL::get_service_key(const char *strRURI, char *strServiceKey)
{
    for(int i = 0; i <  m_nSvcKeyListCount; i++)
	{
		if(strstr(strRURI, m_SvcKeyList[i].strSvcName))
		{
			strcpy(strServiceKey, m_SvcKeyList[i].strSvcKey);
			return(true);
		}
	}
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_service_key
 * CLASS-NAME     : SVCKEY_TBL
 * PARAMETER    IN: strServiceKey - 입력 ServiceKey
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 ServiceKey가 등록되어 있는지를 확인하는 함수
 * REMARKS        : 20160614 add for mPBX BMT PSI trigger
 **end*******************************************************/
bool SVCKEY_TBL::is_service_key(const char *strServiceKey)
{
    for(int i = 0; i <  m_nSvcKeyListCount; i++)
    {
        if(strstr(strServiceKey, m_SvcKeyList[i].strSvcName)) { return(true); }
    }
    
    return(false);
}


