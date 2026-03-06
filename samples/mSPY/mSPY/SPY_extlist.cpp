/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_extlist.cpp
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
 DESCRIPTION    : extlist.cfg 파일을 읽어서 처리하고 관리하는 Class 함수
 REMARKS        :
 *end*************************************************************/

#include "SPY_extlist.h"


extern NSM_LOG   Log;

#ifndef ALARM_OFF
#   define		ALARM_OFF		0x00
#   define		ALARM_ON		0x01
#endif

#define DBM_MSG                 0xC001
#define MSG_ID_SGM_QUERY        0xD006


typedef struct
{
    char    header[16];
    char    strSQL[112];
} S_TO_ORACLE;


extern void SEND_ExtServerStatusAlarm(char *strSswID, char *strIP, int alarm_onoff, time_t time_down, int ot_type);
//extern  int SendAlarmMessage(int cmd_no, int type, int flag, int alarm_id, const char *extend, const char *comment);


extern  uint8_t MY_AS_ID;                // MY AS_INDEX (0 ~ 7)

#pragma mark -
#pragma mark -
#pragma mark [EXTLIST_TBL] 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : EXTLIST_TBL
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EXTLIST_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
EXTLIST_TBL::EXTLIST_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~EXTLIST_TBL
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EXTLIST_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
EXTLIST_TBL::~EXTLIST_TBL(void)
{
	clear();
}


//#pragma mark -
//#pragma mark [EXTLIST_TBL] Alive Check ?옜?OPTIONS 옜옜 옜/옜?옜?옜 옜

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : print_log
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 메모리로 읽어들인 Ext. Svr 정보를 로그파일로 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
void EXTLIST_TBL::print_log(void)
{
    
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "EXTERNAL SERVER CONFIG FILENAME = %s\n", m_filename);
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "#  SSWID =  IP: BLOCK: MONITOR: CHECK_TIME: TRY_COUNT\n");
    
    for(int i = 0; i < m_nExtSvrListCount; i++)
	{
		Log.printf(LOG_INF, "   %s  = %16s: %d: %d: %d: %d\n",
                  m_ExtSvrList[i].sswid,   m_ExtSvrList[i].ip,        m_ExtSvrList[i].block,
                  m_ExtSvrList[i].monitor, m_ExtSvrList[i].checkTime, m_ExtSvrList[i].tryCount);
	}
    Log.printf(LOG_INF, "    Number of List = %d \n", m_nExtSvrListCount);
    Log.printf(LOG_INF, "============================================\n");
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::read(const char *fname)
{
    FILE	*fp;
    char    buf[256], strSswId[64], strData[64];
    string  strKey;
    int     nCount = 0;
    STRs    parms;
    
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL] filename length is too big [%s]\n", fname);
        return(false);
    }
    
    strcpy(m_filename, fname);        // store filename(/.../extlist.cfg)
    
    if((fp = fopen(m_filename, "r")) == NULL)
	{
		Log.printf(LOG_ERR, "[EXTLIST_TBL] Can't Open %s file\n", m_filename);
		return(false);
	}
    
    while(fgets(buf, 255, fp))
    {
        LIB_delete_comment(buf, '#');       // comment 이후 부분 삭제
        LIB_delete_white_space(buf);        // 공백 제거
        
        if(strlen(buf) == 0) { continue; }                              // skip empty line
        if((*buf == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
        if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
        
        // SSW_ID=IP:BLOCK:MONITOR:CHECK_TIME:TRY_COUNT (INDEX 는 의미 없음)
        if(LIB_split_string_into_2(buf, '=', strSswId, strData) == false) { continue; }
        
        LIB_split_const_string(strData, ':', &parms);
        if(parms.cnt != 5)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] Ext. Server Info Format Error[%s] ... skip this line\n", strData);
            continue;
        }
        
        strncpy(m_ExtSvrList[nCount].sswid, strSswId,       3);
        strncpy(m_ExtSvrList[nCount].ip,    parms.str[0],   MAX_EXTSVR_IP_LEN);
        m_ExtSvrList[nCount].block     = atoi(parms.str[1]);
		m_ExtSvrList[nCount].monitor   = atoi(parms.str[2]);
		m_ExtSvrList[nCount].checkTime = atoi(parms.str[3]);
		m_ExtSvrList[nCount].tryCount  = atoi(parms.str[4]);
        
        m_ExtSvrList[nCount].fault     = 0;
        m_ExtSvrList[nCount].recvCount = 0;
        m_ExtSvrList[nCount].lastTime  = 0;
        
        if(++nCount > MAX_EXTSVR_LIST)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] Ext. Server count reach MAX[%d]\n", nCount);
            break;
        }
    }
    
    fclose(fp);
    
    m_nExtSvrListCount = nCount;

    print_log();
//    all_alarm_off();        // add 20160922 - dead lock
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : reread
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::reread(void)
{
    bool    result;
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
        
        result = read(m_filename);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(result);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : reread_from_WEB
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : web에서 업로드한 extlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        : WEB과 SPY 실행 계정이 서로 달라서 WEB은 SPY의 extlist.cfg 파일을 직접 업로드 할 수 없다.
 *                : 그래서 web이 올린 위치를 알려주면 SPY가 직접 옮긴후 파일을 읽는다.
 **end*******************************************************/
bool EXTLIST_TBL::reread_from_WEB(const char *web_extlist_filename)
{
    bool    result;
    char    backup_name[256];
                                                                                                             
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    rename(m_filename, backup_name);                // copy 전 원래 파일을 백업파일로 move
    
    rename(web_extlist_filename, m_filename);       // web에서 down받은 파일이름을 원래 extlist.cfg위치로 옮긴다.

    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
        
        result = read(m_filename);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(result);
}

#pragma mark -
#pragma mark [EXTLIST_TBL]에 INSERT/UPDATE/DELETE하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr      - External Server Information
 *              IN: bInsertFile - 파일에도 추가할 것인지 여부를 나타냄
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에 추가하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::insert(EXTSVR_LIST *extSvr, bool bInsertFile)
{
    int     index;

    if(exist(extSvr) == true) { return(true); }      // 이미 등록됨 ID - OK

    index = m_nExtSvrListCount;
    
    if(index >= MAX_EXTSVR_LIST)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL] Ext. Server List full[%d]\n", index);
        return(false);
    }
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
        
        strncpy(m_ExtSvrList[index].sswid, extSvr->sswid,    3);
        strncpy(m_ExtSvrList[index].ip,    extSvr->ip,       MAX_EXTSVR_IP_LEN);
        m_ExtSvrList[index].block     = extSvr->block;
		m_ExtSvrList[index].monitor   = extSvr->monitor;
		m_ExtSvrList[index].checkTime = extSvr->checkTime;
		m_ExtSvrList[index].tryCount  = extSvr->tryCount;
        
        m_ExtSvrList[index].fault     = 0;
        m_ExtSvrList[index].recvCount = 0;
        m_ExtSvrList[index].lastTime  = 0;
    }
    
    m_nExtSvrListCount ++;
    pthread_mutex_unlock(&lock_mutex);
    
    if(bInsertFile == true) { insert_file(extSvr); }    // update extlist.cfg file
    
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에 UPDATE 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::update(EXTSVR_LIST *extSvr)
{
    int     index;
    
    if(exist(extSvr) == false) { return(false); }      // 등록되어 있지 않음

    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
            {
                // 현재 ALARM이 떠 있는데 모니터링 안하게 UPDATE되면 떠 있는 ALARM은 OFF를 함.
//                if((m_ExtSvrList[index].fault == true) && (extSvr->monitor == 0))
                // 일단 Alarm을 OFF한다. 아래에서 m_ExtSvrList[index].fault = 0 으로 하기 때문에 Alarm이 있으면 다시 발생된다.
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
                    m_ExtSvrList[index].fault = ALARM_OFF;
                }
                
                bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
                
                strncpy(m_ExtSvrList[index].sswid, extSvr->sswid,    3);
                strncpy(m_ExtSvrList[index].ip,    extSvr->ip,       MAX_EXTSVR_IP_LEN);
                m_ExtSvrList[index].block     = extSvr->block;
                m_ExtSvrList[index].monitor   = extSvr->monitor;
                m_ExtSvrList[index].checkTime = extSvr->checkTime;
                m_ExtSvrList[index].tryCount  = extSvr->tryCount;
                
                m_ExtSvrList[index].fault     = 0;
                m_ExtSvrList[index].recvCount = 0;
                m_ExtSvrList[index].lastTime  = 0;
                
                pthread_mutex_unlock(&lock_mutex);

                update_file(extSvr);             // update extlist.cfg file
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    Log.printf(LOG_ERR, "[EXTLIST_TBL] update TABLE not found %s = %s: %d: %d: %d: %d\n", extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
	return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : select
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *             OUT: bBlock - SSW BLOCK 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 해당하는 IP의 EXTLIST 등록여부와 BLOCK 여부를 확인하는 함수
 * REMARKS        : mutex_lock을 사용해야 하는지 검토 필요 (속도 이슈)
 **end*******************************************************/
bool EXTLIST_TBL::select(const char *strIP, bool *bBlock)
{
    int     index;
    
//    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(strcmp(m_ExtSvrList[index].ip, strIP) == 0)
            {
                *bBlock = m_ExtSvrList[index].block;
//                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
//    pthread_mutex_unlock(&lock_mutex);

	return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에서 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::erase(EXTSVR_LIST *extSvr)
{
    int     index;
    bool    bErased = false;
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(bErased == true)
            {
                // 삭제가 되었으면 삭제된 뒷 부분 한 칸씩 앞으로 copy.....
                memcpy(&m_ExtSvrList[index-1], &m_ExtSvrList[index], sizeof(EXTSVR_LIST));
            }
            else
            {
                if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
                {
                    if(m_ExtSvrList[index].fault)       // 삭제전 ALARM이 ON 되어 있는 상태이면 - ALARM_OFF 처리
                    {
                        SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
                        m_ExtSvrList[index].fault = ALARM_OFF;
                    }
                    
                    bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
                    bErased = true;
                }
            }
        }
        if(bErased == true) { -- m_nExtSvrListCount; }  // 삭제되었으면 COUNT를 하나 줄임
    }
    pthread_mutex_unlock(&lock_mutex);
    
    if(bErased == true)  // lock_mutex 부분 시간을 줄이기 위해서 뒤로 뺐음
    {
        erase_file(extSvr);
    }
    else
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL] erase TABLE not found %s = %s: %d: %d: %d: %d\n", extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
    }
    
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : all_alarm_off
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 모든 ALARM을 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::all_alarm_off(void)
{
    int     index;
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
			// 20250714 bible : Add 초기화
			m_ExtSvrList[index].lastTime  = time(NULL);
			m_ExtSvrList[index].recvCount = 0;

#ifdef OAMSV5_MODE
			// 20250714 bible : 알람 초기화 (-1)로 요청
			SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, -1, m_ExtSvrList[index].lastTime, 0);
#else
            SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
#endif
            m_ExtSvrList[index].fault = ALARM_OFF;
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(true);
}


#pragma mark -
#pragma mark [EXTLIST_TBL] 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void EXTLIST_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : TABLE에 등록된 EXT. SERVER 수
 * DESCRIPTION    : TABLE에 등록된 전체 EXT. SERVER 개수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int EXTLIST_TBL::size(void)
{
	return(m_nExtSvrListCount);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : exist
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MAP에 해당하는 IP의 SSW가 등록되어 있는지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::exist(EXTSVR_LIST *extSvr)
{
    int     index;
    
    for(index = 0; index < m_nExtSvrListCount; index ++)
    {
        if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
        {
            return(true);
        }
    }
	return(false);
}


//#pragma mark -
//#pragma mark [EXTLIST_TBL] Alive Check ?옜?OPTIONS 옜옜 옜/옜?옜?옜 옜

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : receive_options
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: strIP   - External Server Information
 *              IN: ot_type - 발생한 곳이 ORIG인지 TERM인지를 나타냄
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT. Server에서 OPTIONS 메세지 수신 처리를 하는 함수
 * REMARKS        : OPTIONS를 수신하면 살아 있는것으로 판단
 **end*******************************************************/
bool EXTLIST_TBL::receive_options(const char *strIP, int ot_type)
{
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < m_nExtSvrListCount; i++)
        {
            // FIXIT - V6 case
            if (strcmp(m_ExtSvrList[i].ip, strIP) == 0)
            {
                m_ExtSvrList[i].lastTime  = time(NULL);
                m_ExtSvrList[i].recvCount = 0;
                
                if(m_ExtSvrList[i].fault)       // 이전에 ALARM이 ON 되어 있는 상태에서 OPTION을 수신 - ALARM_OFF 처리
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, ALARM_OFF, m_ExtSvrList[i].lastTime, ot_type);
                    m_ExtSvrList[i].fault = ALARM_OFF;
                }
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_options_received
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 일정시간 OPTIONS가 수신안된 서버에 대한 ALARM을 전송하는 함수
 * REMARKS        : fault를 true로 변경
 **end*******************************************************/
void EXTLIST_TBL::is_options_received(void)
{
    time_t  clock;
    
    clock = time(NULL);
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < m_nExtSvrListCount; i ++)
        {
            // && (!m_ExtSvrList[i].block) 추가 -  CSCF block 시 OPTIONS 체크하지 않도록 수정
            if((m_ExtSvrList[i].monitor) && (!m_ExtSvrList[i].block) && (!m_ExtSvrList[i].fault) && (m_ExtSvrList[i].lastTime + m_ExtSvrList[i].checkTime <= clock))
            {
                m_ExtSvrList[i].recvCount++;
                m_ExtSvrList[i].lastTime = clock;
                if (m_ExtSvrList[i].recvCount >= m_ExtSvrList[i].tryCount)
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, ALARM_ON, clock, 0);
                    m_ExtSvrList[i].fault = ALARM_ON;
                }
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_tbl_ssw_config
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 살아있는 외부 서버 정보를 ORACLE TBL_SSW_CONFIG에 UPDATE하는 함수
 * REMARKS        : SGM을 통해서 ORACLE에 UPDATE
 **end*******************************************************/
void EXTLIST_TBL::update_tbl_ssw_config(void)
{
    char    strSend[512];
	long    clock;
	struct  tm  *mydate;
    int     nLen;
    
	for(int i = 0; i < m_nExtSvrListCount; i ++)
	{
		if (!m_ExtSvrList[i].fault)
		{
            clock   = m_ExtSvrList[i].lastTime;
			mydate = localtime(&clock);
            
			bzero(strSend, sizeof(strSend));
			strSend[0] = 1;
            
			nLen    = 16;   // 16 = sizeof(header) ??
			nLen += snprintf(strSend+nLen, sizeof(strSend)-16, "UPDATE TBL_SSW_CONFIG SET UPDATE_DATE='%04d%02d%02d%02d%02d%02d' WHERE AS_IDX=%d AND SSW_ID=%s",
                            mydate->tm_year+1900, mydate->tm_mon+1, mydate->tm_mday, mydate->tm_hour, mydate->tm_min, mydate->tm_sec,
                            MY_AS_ID, m_ExtSvrList[i].sswid);
            
			msendsig(nLen, SGM, MSG_ID_SGM_QUERY, (uint8_t *)strSend);
            
			Log.printf(LOG_LV1, "Update SSW Time [%d][%s] = %s : last=%u\n", i, m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, m_ExtSvrList[i].lastTime);
		}
	}
}


#pragma mark -
#pragma mark [EXTLIST_TBL] extlist.cfg 파일을 수정하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : backup_file
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist.cfg 파일을 extlist.cfg.time으로 백업하는 함수
 * REMARKS        : time은 현재시간(time_t)
 **end*******************************************************/
bool EXTLIST_TBL::backup_file(void)
{
    FILE	*src, *dest;
    char    backup_name[256];
    char    buf[256];
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));       // BACKUP filename = filename+time
    
    if((src = fopen(m_filename,  "r")) == NULL)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL] backup_file() Can't Open src file %s \n", m_filename);
		return(false);
    }
    
    if((dest = fopen(backup_name,  "w")) == NULL)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL] backup_file() Can't Open dest file %s \n", backup_name);
        fclose(src);
		return(false);
    }
    
    while(fgets(buf, 255, src))
    {
        fputs(buf, dest);
    }
    
    fclose(src);
    fclose(dest);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert_file
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에 추가하는 함수
 * REMARKS        : 파일의 끝에 append
 **end*******************************************************/
bool EXTLIST_TBL::insert_file(EXTSVR_LIST *extSvr)
{
    FILE	*fp;
    
    pthread_mutex_lock(&file_mutex);
    {
        backup_file();      // insert전 원래 파일을 백업
        
        if((fp = fopen(m_filename, "a+")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[EXTLIST_TBL] insert_file() Can't Open %s file\n", m_filename);
            return(false);
        }
        
        fprintf(fp, "%s = %s:%d:%d:%d:%d        # INSERTED by EXTLIST_TBL.insert_file()\n",
                  extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);

        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_file
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 Block 여부를 extlist.cfg 파일에 UPDATE하는 함수
 * REMARKS        : 원본을 백업으로 변경하고 백업을 원본이름으로 write..
 **end*******************************************************/
bool EXTLIST_TBL::update_file(EXTSVR_LIST *extSvr)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    strSswId[256], strData[256];
    bool    done = false;
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    
    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] update_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] update_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] update_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 extlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // update가 완료되었음
            {
                fputs(buf, new_fp);     // update가 끝난 나머지 부분들을 복사
            }
            else
            {
                // sswid 부분을 추출하기 위해서
                if(LIB_split_const_string_into_2(buf, '=', strSswId, strData) == true)
                {
                    LIB_delete_comment(strSswId, '#');
                    LIB_delete_white_space(strSswId);
                    
                    if(strlen(strSswId) > 3) { continue; }  // 뭔가 sswid를 잘못 가져온 경우
                    
                    if(strcmp(strSswId, extSvr->sswid) == 0)
                    {
                        fprintf(new_fp, "%s = %s:%d:%d:%d:%d        # UPDATED by EXTLIST_TBL.insert_file()\n",
                                extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
                        
                        done = true;    // update완료
                    }
                }
                else
                {
                    fputs(buf, new_fp);     // 파일에 그대로 복사
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase_file
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에서 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL::erase_file(EXTSVR_LIST *extSvr)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    strSswId[256], strData[256];
    bool    done = false;
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    
    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] erase_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] erase_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL] erase_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 extlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // delete가 완료되었음
            {
                fputs(buf, new_fp); // delete가 끝난 나머지 부분들을 복사
            }
            else
            {
                // sswid 부분을 추출하기 위해서
                if(LIB_split_const_string_into_2(buf, '=', strSswId, strData) == true)
                {
                    LIB_delete_comment(strSswId, '#');
                    LIB_delete_white_space(strSswId);
                    
                    if(strlen(strSswId) > 3) { continue; }  // 뭔가 sswid를 잘못 가져온 경우
                    
                    if(strcmp(strSswId, extSvr->sswid) == 0)
                    {
                        // 여기서 fputs를 안하면 라인 삭제 됨
                        done = true;    // update완료
                    }
                }
                else
                {
                    fputs(buf, new_fp);     // 파일에 그대로 복사
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : save_file
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에 추가하는 함수
 * REMARKS        : 파일의 끝에 append
 **end*******************************************************/
bool EXTLIST_TBL::save_file(void)
{
    FILE	*fp;
    
    pthread_mutex_lock(&file_mutex);
    {
        backup_file();      // insert전 원래 파일을 백업
        
        if((fp = fopen(m_filename, "a+")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[EXTLIST_TBL] insert_file() Can't Open %s file\n", m_filename);
            return(false);
        }
        
        for(int i = 0; i < m_nExtSvrListCount; i ++)
        {
            fprintf(fp, "%s = %s:%d:%d:%d:%d        #   \n",
                m_ExtSvrList->sswid, m_ExtSvrList->ip, m_ExtSvrList->block, m_ExtSvrList->monitor, m_ExtSvrList->checkTime, m_ExtSvrList->tryCount);
        }
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
	return(true);
}

#pragma mark -
#pragma mark ERRSVR_TBL
#pragma mark -
#pragma mark [ERRSVR_TBL] 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ERRSVR_TBL
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : ERRSVR_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
ERRSVR_TBL::ERRSVR_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
//    pthread_mutex_init(&file_mutex, NULL);
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~ERRSVR_TBL
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : ERRSVR_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
ERRSVR_TBL::~ERRSVR_TBL(void)
{
	clear();
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void ERRSVR_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ErrSvrList, sizeof(ERRSVR_LIST)*MAX_ERRSVR_LIST);
//        m_nErrSvrListCount = 0;
    }
    pthread_mutex_unlock(&lock_mutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : add_unreg
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER    IN: strIP - 메세지를 보내온 미등록 SSW IP
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 메세지를 보내온 등록되지 않은 SSW(MS,..) 정보를 기록하는 함수
 * REMARKS        : 앞에서 부터 차례대로 저장되고 한꺼번에 삭제됨... 중간이 비는 경우는 없음
 **end*******************************************************/
bool ERRSVR_TBL::add_unreg(const char *strIP)
{
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < MAX_ERRSVR_LIST; i++)
        {
            if(strcmp(m_ErrSvrList[i].strIP, strIP) == 0)   // 이미 등록되어 있는 경우
            {
                m_ErrSvrList[i].nUnRegCount  ++;

                Log.printf(LOG_LV1, "[ERRSVR_TBL] add_unreg(%s) nUnRegCount=[%d]\n", strIP, m_ErrSvrList[i].nUnRegCount);
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
            else if(m_ErrSvrList[i].strIP[0] == '\0')       // 빈 LIST
            {
                strcpy(m_ErrSvrList[i].strIP, strIP);
                m_ErrSvrList[i].nUnRegCount = 1;
                
                Log.printf(LOG_LV1, "[ERRSVR_TBL] add_unreg(%s) nUnRegCount=[1] - New REG\n", strIP);
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    Log.printf(LOG_ERR, "[ERRSVR_TBL] add_unreg(%s) Fail List Full\n", strIP);
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : add_unreg
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER    IN: strIP - 잘못된 SIP 메세지를 보낸 SSW IP
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 잘못된 SIP 메세지를 보낸 SSW(MS,...) 정보를 기록하는 함수
 * REMARKS        : 앞에서 부터 차례대로 저장되고 한꺼번에 삭제됨... 중간이 비는 경우는 없음
 **end*******************************************************/
bool ERRSVR_TBL::add_msgerr(const char *strIP)
{
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < MAX_ERRSVR_LIST; i++)
        {
            if(strcmp(m_ErrSvrList[i].strIP, strIP) == 0)   // 이미 등록되어 있는 경우
            {
                m_ErrSvrList[i].nMsgErrCount  ++;
                
                Log.printf(LOG_LV1, "[ERRSVR_TBL] add_unreg(%s) nMsgErrCount=[%d]\n", strIP, m_ErrSvrList[i].nMsgErrCount);
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
            else if(m_ErrSvrList[i].strIP[0] == '\0')       // 빈 LIST
            {
                strcpy(m_ErrSvrList[i].strIP, strIP);
                m_ErrSvrList[i].nMsgErrCount = 1;
                
                Log.printf(LOG_LV1, "[ERRSVR_TBL] add_unreg(%s) nMsgErrCount=[1] - New REG\n", strIP);
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    Log.printf(LOG_ERR, "[ERRSVR_TBL] add_unreg(%s) Fail List Full\n", strIP);
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : report_to_ems
 * CLASS-NAME     : ERRSVR_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 잘못된 SSW(MS,...) 정보를 EMS Oracle로 전송하는 함수
 * REMARKS        : SGM 경유
 **end*******************************************************/
void ERRSVR_TBL::report_to_ems(void)
{
    S_TO_ORACLE  send;
    int     nLen;
    int     i;
    
    for(i = 0; i < MAX_ERRSVR_LIST; i++)
    {
        if(m_ErrSvrList[i].strIP[0] == '\0') { break; }     // 빈 LIST - END of LIST

        bzero(&send, sizeof(send));
        
        send.header[0] = 1;
        nLen  = 16;           // sizeof(send.header) = 16
        nLen += snprintf(send.strSQL, sizeof(send.strSQL), "begin sp_unknownSSW(%d, '%s', %d, %d); end;",
                        MY_AS_ID, m_ErrSvrList[i].strIP, m_ErrSvrList[i].nUnRegCount, m_ErrSvrList[i].nMsgErrCount );
        
        msendsig(nLen, SGM, MSG_ID_SGM_QUERY, (uint8_t *)&send);
        
        Log.printf(LOG_LV1, "[ERRSVR_TBL] report_to_ems(%s)\n", send.strSQL);
    }
    
    if( i > 0 ) { clear(); }     // CLEAR LIST
}

#pragma mark -
#pragma mark EXTLIST_TBL_V6
#pragma mark -

#ifdef IPV6_MODE

#pragma mark -
#pragma mark [EXTLIST_TBL_V6] 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : EXTLIST_TBL_V6
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EXTLIST_TBL_V6 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
EXTLIST_TBL_V6::EXTLIST_TBL_V6(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~EXTLIST_TBL_V6
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EXTLIST_TBL_V6 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
EXTLIST_TBL_V6::~EXTLIST_TBL_V6(void)
{
    clear();
}


//#pragma mark -
//#pragma mark [EXTLIST_TBL_V6] extlist.cfg를 읽어서 TABLE에 INSERT하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : print_log
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 메모리로 읽어들인 Ext. Svr 정보를 로그파일로 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
void EXTLIST_TBL_V6::print_log(void)
{
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "EXTERNAL SERVER CONFIG FILENAME = %s\n", m_filename);
    Log.printf(LOG_INF, "============================================\n");
    Log.printf(LOG_INF, "#  SSWID =  IP, BLOCK, MONITOR, CHECK_TIME, TRY_COUNT\n");
    
    for(int i = 0; i < m_nExtSvrListCount; i++)
    {
        Log.printf(LOG_INF, "   %s  = %16s, %d, %d, %d, %d\n",
                   m_ExtSvrList[i].sswid,   m_ExtSvrList[i].ip,        m_ExtSvrList[i].block,
                   m_ExtSvrList[i].monitor, m_ExtSvrList[i].checkTime, m_ExtSvrList[i].tryCount);
    }
    Log.printf(LOG_INF, "    Number of List = %d \n", m_nExtSvrListCount);
    Log.printf(LOG_INF, "============================================\n");
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist_v6.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        : 파라미터 구분자가 ',' 임
 **end*******************************************************/
bool EXTLIST_TBL_V6::read(const char *fname)
{
    FILE	*fp;
    char    buf[256], strSswId[64], strData[64];
    string  strKey;
    int     nCount = 0;
    STRs    parms;
    
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] filename length is too big [%s]\n", fname);
        return(false);
    }
    
    strcpy(m_filename, fname);        // store filename(/.../extlist.cfg)
    
    if((fp = fopen(m_filename, "r")) == NULL)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] Can't Open %s file\n", m_filename);
        return(false);
    }
    
    while(fgets(buf, 255, fp))
    {
        LIB_delete_comment(buf, '#');       // comment 이후 부분 삭제
        LIB_delete_white_space(buf);        // 공백 제거
        
        if(strlen(buf) == 0) { continue; }                              // skip empty line
        if((*buf == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
        if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
        
        // SSW_ID=IP:BLOCK:MONITOR:CHECK_TIME:TRY_COUNT (INDEX 는 의미 없음)
        if(LIB_split_string_into_2(buf, '=', strSswId, strData) == false) { continue; }
        
        LIB_split_const_string(strData, ',', &parms);
        if(parms.cnt != 5)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] Ext. Server Info Format Error[%s] ... skip this line\n", strData);
            continue;
        }
        
        strncpy(m_ExtSvrList[nCount].sswid, strSswId,       3);
        strncpy(m_ExtSvrList[nCount].ip,    parms.str[0],   MAX_EXTSVR_IP_LEN);
        inet_pton(AF_INET6, m_ExtSvrList[nCount].ip, &m_ExtSvrList[nCount].v6_addr);
        m_ExtSvrList[nCount].block     = atoi(parms.str[1]);
        m_ExtSvrList[nCount].monitor   = atoi(parms.str[2]);
        m_ExtSvrList[nCount].checkTime = atoi(parms.str[3]);
        m_ExtSvrList[nCount].tryCount  = atoi(parms.str[4]);
        
        m_ExtSvrList[nCount].fault     = 0;
        m_ExtSvrList[nCount].recvCount = 0;
        m_ExtSvrList[nCount].lastTime  = 0;
        
#ifdef DEBUG_MODE
        {
            char    strTempIP[64] = "";
            
            inet_ntop(AF_INET6, (void *)&m_ExtSvrList[nCount].v6_addr, strTempIP, sizeof(strTempIP));
            Log.color(COLOR_MAGENTA, LOG_LV1, "[EXTLIST_TBL_V6] add V6 ExtServer: ID=%d IP=%s, inet_ntop=[%s]\n", m_ExtSvrList[nCount].sswid, m_ExtSvrList[nCount].ip, strTempIP);
        }
#endif
        if(++nCount > MAX_EXTSVR_LIST)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] Ext. Server count reach MAX[%d]\n", nCount);
            break;
        }
    }
    
    fclose(fp);
    
    m_nExtSvrListCount = nCount;
    
    print_log();
//    all_alarm_off();        // add 20160922 - dead lock
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : reread
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::reread(void)
{
    bool    result;
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
        
        result = read(m_filename);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(result);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : reread_from_WEB
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: fname - extlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : web에서 업로드한 extlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        : WEB과 SPY 실행 계정이 서로 달라서 WEB은 SPY의 extlist.cfg 파일을 직접 업로드 할 수 없다.
 *                : 그래서 web이 올린 위치를 알려주면 SPY가 직접 옮긴후 파일을 읽는다.
 **end*******************************************************/
bool EXTLIST_TBL_V6::reread_from_WEB(const char *web_extlist_filename)
{
    bool    result;
    char    backup_name[256];
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    rename(m_filename, backup_name);                // copy 전 원래 파일을 백업파일로 move
    
    rename(web_extlist_filename, m_filename);       // web에서 down받은 파일이름을 원래 extlist.cfg위치로 옮긴다.
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
        
        result = read(m_filename);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(result);
}

#pragma mark -
#pragma mark [EXTLIST_TBL_V6]에 INSERT/UPDATE/DELETE하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr      - External Server Information
 *              IN: bInsertFile - 파일에도 추가할 것인지 여부를 나타냄
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에 추가하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::insert(EXTSVR_LIST *extSvr, bool bInsertFile)
{
    int     index;
    
    if(exist(extSvr) == true) { return(true); }      // 이미 등록됨 ID - OK
    
    index = m_nExtSvrListCount;
    
    if(index >= MAX_EXTSVR_LIST)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] Ext. Server List full[%d]\n", index);
        return(false);
    }
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
        
        strncpy(m_ExtSvrList[index].sswid, extSvr->sswid,    3);
        strncpy(m_ExtSvrList[index].ip,    extSvr->ip,       MAX_EXTSVR_IP_LEN);
        inet_pton(AF_INET6, m_ExtSvrList[index].ip, &m_ExtSvrList[index].v6_addr);
        m_ExtSvrList[index].block     = extSvr->block;
        m_ExtSvrList[index].monitor   = extSvr->monitor;
        m_ExtSvrList[index].checkTime = extSvr->checkTime;
        m_ExtSvrList[index].tryCount  = extSvr->tryCount;
        
        m_ExtSvrList[index].fault     = 0;
        m_ExtSvrList[index].recvCount = 0;
        m_ExtSvrList[index].lastTime  = 0;
    }
    
    m_nExtSvrListCount ++;
    pthread_mutex_unlock(&lock_mutex);
    
    if(bInsertFile == true) { insert_file(extSvr); }    // update extlist.cfg file
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에 UPDATE 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::update(EXTSVR_LIST *extSvr)
{
    int     index;
    
    if(exist(extSvr) == false) { return(false); }      // 등록되어 있지 않음
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
            {
                // 현재 ALARM이 떠 있는데 모니터링 안하게 UPDATE되면 떠 있는 ALARM은 OFF를 함.
                if((m_ExtSvrList[index].fault == true) && (extSvr->monitor == 0))
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
                    m_ExtSvrList[index].fault = ALARM_OFF;
                }
                
                bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
                
                strncpy(m_ExtSvrList[index].sswid, extSvr->sswid,    3);
                strncpy(m_ExtSvrList[index].ip,    extSvr->ip,       MAX_EXTSVR_IP_LEN);
                inet_pton(AF_INET6, m_ExtSvrList[index].ip, &m_ExtSvrList[index].v6_addr);
                m_ExtSvrList[index].block     = extSvr->block;
                m_ExtSvrList[index].monitor   = extSvr->monitor;
                m_ExtSvrList[index].checkTime = extSvr->checkTime;
                m_ExtSvrList[index].tryCount  = extSvr->tryCount;
                
                m_ExtSvrList[index].fault     = 0;
                m_ExtSvrList[index].recvCount = 0;
                m_ExtSvrList[index].lastTime  = 0;
                
                pthread_mutex_unlock(&lock_mutex);
                
                update_file(extSvr);             // update extlist.cfg file
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] update TABLE not found %s = %s: %d: %d: %d: %d\n", extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : select
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: strIP  - SSW IP address
 *             OUT: bBlock - SSW BLOCK 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 해당하는 IP의 EXTLIST 등록여부와 BLOCK 여부를 확인하는 함수
 * REMARKS        : mutex_lock을 사용해야 하는지 검토 필요 (속도 이슈)
 **end*******************************************************/
bool EXTLIST_TBL_V6::select(struct in6_addr *v6_addr, bool *bBlock)
{
    int     index;
    
//    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(IN6_ARE_ADDR_EQUAL(v6_addr, &m_ExtSvrList[index].v6_addr))
            {
                *bBlock = m_ExtSvrList[index].block;
//                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
//    pthread_mutex_unlock(&lock_mutex);
    
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 TABLE에서 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::erase(EXTSVR_LIST *extSvr)
{
    int     index;
    bool    bErased = false;
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
            if(bErased == true)
            {
                // 삭제가 되었으면 삭제된 뒷 부분 한 칸씩 앞으로 copy.....
                memcpy(&m_ExtSvrList[index-1], &m_ExtSvrList[index], sizeof(EXTSVR_LIST));
            }
            else
            {
                if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
                {
                    if(m_ExtSvrList[index].fault)       // 삭제전 ALARM이 ON 되어 있는 상태이면 - ALARM_OFF 처리
                    {
                        SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
                        m_ExtSvrList[index].fault = ALARM_OFF;
                    }
                    
                    bzero(&m_ExtSvrList[index], sizeof(EXTSVR_LIST));       // TABLE 초기화
                    bErased = true;
                }
            }
        }
        if(bErased == true) { -- m_nExtSvrListCount; }  // 삭제되었으면 COUNT를 하나 줄임
    }
    pthread_mutex_unlock(&lock_mutex);
    
    if(bErased == true)  // lock_mutex 부분 시간을 줄이기 위해서 뒤로 뺐음
    {
        erase_file(extSvr);
    }
    else
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] erase TABLE not found %s = %s: %d: %d: %d: %d\n", extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
    }
    
    return(false);
    
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : all_alarm_off
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT, SERVER를 모든 ALARM을 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::all_alarm_off(void)
{
    int     index;
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(index = 0; index < m_nExtSvrListCount; index ++)
        {
			// 20250714 bible : Add 초기화
			m_ExtSvrList[index].lastTime  = time(NULL);
            m_ExtSvrList[index].recvCount = 0;
#ifdef OAMSV5_MODE
            // 20250714 bible : 알람 초기화 (-1)로 요청
            SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, -1, m_ExtSvrList[index].lastTime, 0);
#else
            SEND_ExtServerStatusAlarm(m_ExtSvrList[index].sswid, m_ExtSvrList[index].ip, ALARM_OFF, m_ExtSvrList[index].lastTime, 0);
#endif
            m_ExtSvrList[index].fault = ALARM_OFF;
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(true);
}

#pragma mark -
#pragma mark [EXTLIST_TBL_V6] 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void EXTLIST_TBL_V6::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_ExtSvrList, sizeof(EXTSVR_LIST)*MAX_EXTSVR_LIST);
        m_nExtSvrListCount = 0;
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : TABLE에 등록된 EXT. SERVER 수
 * DESCRIPTION    : TABLE에 등록된 전체 EXT. SERVER 개수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int EXTLIST_TBL_V6::size(void)
{
    return(m_nExtSvrListCount);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : exist
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MAP에 해당하는 IP의 SSW가 등록되어 있는지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::exist(EXTSVR_LIST *extSvr)
{
    int     index;
    
    for(index = 0; index < m_nExtSvrListCount; index ++)
    {
        if(strcmp(m_ExtSvrList[index].sswid, extSvr->sswid) == 0)
        {
            return(true);
        }
    }
    return(false);
}


//#pragma mark -
//#pragma mark [EXTLIST_TBL_V6] Alive Check 를 위해서 OPTIONS 메세지를 수신/미수신 처리를 하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : receive_options
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: strIP   - External Server Information
 *              IN: ot_type - 발생한 곳이 ORIG인지 TERM인지를 나타냄
 * RET. VALUE     : BOOL
 * DESCRIPTION    : EXT. Server에서 OPTIONS 메세지 수신 처리를 하는 함수
 * REMARKS        : OPTIONS를 수신하면 살아 있는것으로 판단
 **end*******************************************************/
bool EXTLIST_TBL_V6::receive_options(struct in6_addr *v6_addr, int ot_type)
{
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < m_nExtSvrListCount; i++)
        {
            if(IN6_ARE_ADDR_EQUAL(v6_addr, &m_ExtSvrList[i].v6_addr))
            {
                m_ExtSvrList[i].lastTime  = time(NULL);
                m_ExtSvrList[i].recvCount = 0;
                
                if(m_ExtSvrList[i].fault)       // 이전에 ALARM이 ON 되어 있는 상태에서 OPTION을 수신 - ALARM_OFF 처리
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, ALARM_OFF, m_ExtSvrList[i].lastTime, ot_type);
                    m_ExtSvrList[i].fault = ALARM_OFF;
                }
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_options_received
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 일정시간 OPTIONS가 수신안된 서버에 대한 ALARM을 전송하는 함수
 * REMARKS        : fault를 true로 변경
 **end*******************************************************/
void EXTLIST_TBL_V6::is_options_received(void)
{
    time_t  clock;
    
    clock = time(NULL);
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(int i = 0; i < m_nExtSvrListCount; i ++)
        {
            // && (!m_ExtSvrList[i].block) 추가 -  CSCF block 시 OPTIONS 체크하지 않도록 수정
            if((m_ExtSvrList[i].monitor) && (!m_ExtSvrList[i].block) && (!m_ExtSvrList[i].fault) && (m_ExtSvrList[i].lastTime + m_ExtSvrList[i].checkTime <= clock))
            {
                m_ExtSvrList[i].recvCount++;
                m_ExtSvrList[i].lastTime = clock;
                if (m_ExtSvrList[i].recvCount >= m_ExtSvrList[i].tryCount)
                {
                    SEND_ExtServerStatusAlarm(m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, ALARM_ON, clock, 0);
                    m_ExtSvrList[i].fault = ALARM_ON;
                }
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_tbl_ssw_config
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 살아있는 외부 서버 정보를 ORACLE TBL_SSW_CONFIG에 UPDATE하는 함수
 * REMARKS        : SGM을 통해서 ORACLE에 UPDATE
 **end*******************************************************/
void EXTLIST_TBL_V6::update_tbl_ssw_config(void)
{
    char    strSend[512];
    long    clock;
    struct  tm  *mydate;
    int     nLen;
    
    for(int i = 0; i < m_nExtSvrListCount; i ++)
    {
        if (!m_ExtSvrList[i].fault)
        {
            clock   = m_ExtSvrList[i].lastTime;
            mydate = localtime(&clock);
            
            bzero(strSend, sizeof(strSend));
            strSend[0] = 1;
            
            nLen    = 16;   // 16 = sizeof(header) ??
            nLen += snprintf(strSend+nLen, sizeof(strSend)-16, "UPDATE TBL_SSW_CONFIG SET UPDATE_DATE='%04d%02d%02d%02d%02d%02d' WHERE AS_IDX=%d AND SSW_ID=%s",
                             mydate->tm_year+1900, mydate->tm_mon+1, mydate->tm_mday, mydate->tm_hour, mydate->tm_min, mydate->tm_sec,
                             MY_AS_ID, m_ExtSvrList[i].sswid);
            
            msendsig(nLen, SGM, MSG_ID_SGM_QUERY, (uint8_t *)strSend);
            
            Log.printf(LOG_LV1, "Update SSW Time [%d][%s] = %s : last=%u\n", i, m_ExtSvrList[i].sswid, m_ExtSvrList[i].ip, m_ExtSvrList[i].lastTime);
        }
    }
}


#pragma mark -
#pragma mark [EXTLIST_TBL_V6] extlist.cfg 파일을 수정하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : backup_file
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : extlist.cfg 파일을 extlist.cfg.time으로 백업하는 함수
 * REMARKS        : time은 현재시간(time_t)
 **end*******************************************************/
bool EXTLIST_TBL_V6::backup_file(void)
{
    FILE	*src, *dest;
    char    backup_name[256];
    char    buf[256];
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));       // BACKUP filename = filename+time
    
    if((src = fopen(m_filename,  "r")) == NULL)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] backup_file() Can't Open src file %s \n", m_filename);
        return(false);
    }
    
    if((dest = fopen(backup_name,  "w")) == NULL)
    {
        Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] backup_file() Can't Open dest file %s \n", backup_name);
        fclose(src);
        return(false);
    }
    
    while(fgets(buf, 255, src))
    {
        fputs(buf, dest);
    }
    
    fclose(src);
    fclose(dest);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert_file
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에 추가하는 함수
 * REMARKS        : 파일의 끝에 append
 **end*******************************************************/
bool EXTLIST_TBL_V6::insert_file(EXTSVR_LIST *extSvr)
{
    FILE	*fp;
    
    pthread_mutex_lock(&file_mutex);
    {
        backup_file();      // insert전 원래 파일을 백업
        
        if((fp = fopen(m_filename, "a+")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] insert_file() Can't Open %s file\n", m_filename);
            return(false);
        }
        
        fprintf(fp, "%s = %s,%d,%d,%d,%d        # INSERTED by EXTLIST_TBL_V6.insert_file()\n",
                extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
        
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_file
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 Block 여부를 extlist.cfg 파일에 UPDATE하는 함수
 * REMARKS        : 원본을 백업으로 변경하고 백업을 원본이름으로 write..
 **end*******************************************************/
bool EXTLIST_TBL_V6::update_file(EXTSVR_LIST *extSvr)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    strSswId[256], strData[256];
    bool    done = false;
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    
    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] update_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] update_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] update_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 extlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // update가 완료되었음
            {
                fputs(buf, new_fp);     // update가 끝난 나머지 부분들을 복사
            }
            else
            {
                // sswid 부분을 추출하기 위해서
                if(LIB_split_const_string_into_2(buf, '=', strSswId, strData) == true)
                {
                    LIB_delete_comment(strSswId, '#');
                    LIB_delete_white_space(strSswId);
                    
                    if(strlen(strSswId) > 3) { continue; }  // 뭔가 sswid를 잘못 가져온 경우
                    
                    if(strcmp(strSswId, extSvr->sswid) == 0)
                    {
                        fprintf(new_fp, "%s = %s,%d,%d,%d,%d        # UPDATED by EXTLIST_TBL_V6.insert_file()\n",
                                extSvr->sswid, extSvr->ip, extSvr->block, extSvr->monitor, extSvr->checkTime, extSvr->tryCount);
                        
                        done = true;    // update완료
                    }
                }
                else
                {
                    fputs(buf, new_fp);     // 파일에 그대로 복사
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase_file
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에서 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
bool EXTLIST_TBL_V6::erase_file(EXTSVR_LIST *extSvr)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    strSswId[256], strData[256];
    bool    done = false;
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    
    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] erase_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] erase_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] erase_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 extlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // delete가 완료되었음
            {
                fputs(buf, new_fp); // delete가 끝난 나머지 부분들을 복사
            }
            else
            {
                // sswid 부분을 추출하기 위해서
                if(LIB_split_const_string_into_2(buf, '=', strSswId, strData) == true)
                {
                    LIB_delete_comment(strSswId, '#');
                    LIB_delete_white_space(strSswId);
                    
                    if(strlen(strSswId) > 3) { continue; }  // 뭔가 sswid를 잘못 가져온 경우
                    
                    if(strcmp(strSswId, extSvr->sswid) == 0)
                    {
                        // 여기서 fputs를 안하면 라인 삭제 됨
                        done = true;    // update완료
                    }
                }
                else
                {
                    fputs(buf, new_fp);     // 파일에 그대로 복사
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : save_file
 * CLASS-NAME     : EXTLIST_TBL_V6
 * PARAMETER    IN: extSvr  - External Server Information
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 extlist.cfg 파일에 추가하는 함수
 * REMARKS        : 파일의 끝에 append
 **end*******************************************************/
bool EXTLIST_TBL_V6::save_file(void)
{
    FILE	*fp;
    
    pthread_mutex_lock(&file_mutex);
    {
        backup_file();      // insert전 원래 파일을 백업
        
        if((fp = fopen(m_filename, "a+")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[EXTLIST_TBL_V6] insert_file() Can't Open %s file\n", m_filename);
            return(false);
        }
        
        for(int i = 0; i < m_nExtSvrListCount; i ++)
        {
            fprintf(fp, "%s = %s,%d,%d,%d,%d        #   \n",
                    m_ExtSvrList->sswid, m_ExtSvrList->ip, m_ExtSvrList->block, m_ExtSvrList->monitor, m_ExtSvrList->checkTime, m_ExtSvrList->tryCount);
        }
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
    
    return(true);
}

#endif  // IPV6_MODE

