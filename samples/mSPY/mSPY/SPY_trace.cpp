/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_trace.cpp
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
 DESCRIPTION    : Call Trace Class 함수
 REMARKS        : FIXIT:
                : 기존에는 SIP Message별로 From/To/Call_ID를 비교해서 Trace하게 했는데....
                : Initial Message만 비교하고 그 다음 부터 그 board/channel로 호가 오는건 모두 Trace하게 변경....
 *end*************************************************************/

#include "SPY_trace.h"
#include "libnsmlog.h"

extern NSM_LOG   Log;

#pragma mark -
#pragma mark 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : TRACE_TBL
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TRACE_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
TRACE_TBL::TRACE_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~TRACE_TBL
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TRACE_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
TRACE_TBL::~TRACE_TBL(void)
{
	clear();
}


#pragma mark -
#pragma mark MAP 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TABLE에 저장된 모든 Trace 를 삭제
 * REMARKS        :
 **end*******************************************************/
void TRACE_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_TraceList, sizeof(TRACE_LIST)*MAX_TRACE_LIST);
        m_nTraceListCount = 0;
        m_ActiveCount     = 0;
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : TABLE에 등록된 Trace 수
 * DESCRIPTION    : TABLE에 등록된 전체 Trace 수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int TRACE_TBL::size(void)
{
	return(m_nTraceListCount);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : recount_list
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : TRACE_TBL에 등록된 Trace 수를 다시 계산하는 함수
 * REMARKS        : lock은 호출전에 수행 됨
 **end*******************************************************/
void TRACE_TBL::recount_list(void)
{
    m_nTraceListCount = 0;
    m_ActiveCount     = 0;
    
	for(int i = 0; i < MAX_TRACE_LIST; i ++)
	{
        if((m_TraceList[i].strFrom[0] != '\0') || (m_TraceList[i].strTo[0] != '\0'))
		{
            m_nTraceListCount ++;
            if(m_TraceList[i].nMaxCount > m_TraceList[i].nCount) { m_ActiveCount ++; }
        }
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : get_trace_filename
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER   OUT: fname - TRACE file name
 * RET. VALUE     : -
 * DESCRIPTION    : Trace 하던 파일을 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
void TRACE_TBL::get_trace_filename(char *fname)
{
    long    now;
	struct  tm *today;

	now   = time(NULL);
	today = localtime(&now);
	sprintf(fname, TRACE_FILE_FORMAT, today->tm_year+1900, today->tm_mon+1, today->tm_mday);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase_file
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Trace 하던 파일을 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
void TRACE_TBL::erase_file(void)
{
    char    filename[128];
    
    get_trace_filename(filename);
    
	remove(filename);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : already_reg
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: trace    - Trace 할 대상 정보(From/To/SvcKey/...)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : TABEL에 해당하는 Trace가 등록되어 있는지 확인하는 함수
 * REMARKS        : '-' mean ALL
 **end*******************************************************/
bool TRACE_TBL::already_reg(TRACE_LIST *trace)
{
    if(m_nTraceListCount == 0) { return(false); }      // 일반적으로 Trace Count가 0이므로 속도 계선을 위해...
    
	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
        if((m_TraceList[i].strSvcKey[0] == '-') || (strcmp(m_TraceList[i].strSvcKey, trace->strSvcKey) == 0))
        {
            if((m_TraceList[i].strFrom[0] == '-') || (strcmp(m_TraceList[i].strFrom, trace->strFrom) == 0))
            {
                if((m_TraceList[i].strTo[0] == '-') || (strcmp(m_TraceList[i].strTo, trace->strTo) == 0))
                {
                    return(true);
                }
            }
        }
	}
    
	return(false);
}


#pragma mark -
#pragma mark TABLE에 INSERT/UPDATE/DELETE하는 함수


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : exist
 * CLASS-NAME     : EXTLIST_TBL
 * PARAMETER    IN: strFrom    - From address(user@doamin or tel_number)
 *              IN: strTo      - To address(user@doamin or tel_number)
 *              IN: strSvcKey  - Service Key
 *              IN: nBoardNo   - SES biard number
 *              IN: nChannelNo - SES channel number
 * RET. VALUE     : -1(NO Trace)/+n(YES Trace)
 * DESCRIPTION    : TABEL에 해당하는 Trace가 등록되어 있는지 확인하는 함수
 * REMARKS        : '-' mean ALL
 **end*******************************************************/
int TRACE_TBL::exist(const char *strFrom, const char *strTo, const char *strSvcKey, int nBoardNo, int nChannelNo)
{
    if(m_ActiveCount <= 0)      // 일반적으로 Trace Count가 0이므로 속도 계선을 위해...
    {
        return(-1);
    }
    
#ifdef DEBUG_MODE
//    Log.printf(LOG_LV1, "m_ActiveCount = %d\n", m_ActiveCount);
#endif

	// 20230627 myung.ss
	int 	check_result = -1;
    check_result = check(nBoardNo, nChannelNo);
    if(check_result >= 0) return (check_result);
	//
               
	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
        if((m_TraceList[i].strSvcKey[0] == '-') || (strcmp(m_TraceList[i].strSvcKey, strSvcKey) == 0))
        {
            if((m_TraceList[i].strFrom[0] == '-') || (strstr(strFrom, m_TraceList[i].strFrom) != NULL))
            {
                if((m_TraceList[i].strTo[0] == '-') || (strstr(strTo, m_TraceList[i].strTo) != NULL))
                {
                    m_TraceList[i].nBoardNo   = nBoardNo;
                    m_TraceList[i].nChannelNo = nChannelNo;
                    
                    Log.printf(LOG_LV1, "[TRACE_TBL] exist() From[%s], To[%s], SvcKey[%s], Board[%d], Channel[%d]\n", strFrom, strTo, strSvcKey, nBoardNo, nChannelNo);
                    return(i);
                }
            }
        }
	}
    
	return(-1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER    IN: trace    - Trace 할 대상 정보(From/To/SvcKey/...)
 *             OUT: nIndex - 성공시에는 등록된 index 번호, 실패시에는 ERROR CODE
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Trace List를 TABLE에 추가하는 함수
 * REMARKS        :
 **end*******************************************************/
bool TRACE_TBL::insert(TRACE_LIST *trace, int *nIndex)
{
    if(already_reg(trace) == true)  // 중복
    {
        *nIndex = TRACE_ERROR_DUPLICATED;
        return(false);
    }
    
    erase_file();       // Trace 중이던 파일이 있으면 삭제 (Trace는 파일에 저장 됨)

	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
		if((m_TraceList[i].strFrom[0] == '\0') && (m_TraceList[i].strTo[0] == '\0'))    // Find 빈 TABLE
		{
            pthread_mutex_lock(&lock_mutex);
            {
                memcpy(&m_TraceList[i], trace, sizeof(TRACE_LIST));     // 입력정보 그대로 copy
                
                m_TraceList[i].nCount     = 0;          // initial
                m_TraceList[i].nBoardNo   = -1;         // initial
                m_TraceList[i].nChannelNo = -1;         // initial
                
                recount_list();     // 등록된 count와 active count를 다시 계산
            }
            pthread_mutex_unlock(&lock_mutex);
            *nIndex = i;
            return(true);
		}
	}

    *nIndex = TRACE_ERROR_TABLE_FULL;
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER    IN: nTrace - 삭제할 Trace List Index
 * RET. VALUE     : BOOL
 * DESCRIPTION    : TABLE 에서 Trace 정보 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
bool TRACE_TBL::erase(int nTrace)
{
    if((nTrace < 0) || (nTrace >= MAX_TRACE_LIST))
    {
        Log.printf(LOG_ERR, "[TRACE_TBL] erase(%d) Index Error\n", nTrace);
        return(false);
    }
    
    pthread_mutex_lock(&lock_mutex);
    {
        bzero(&m_TraceList[nTrace], sizeof(TRACE_LIST));     // 입력정보 그대로 delete
        m_TraceList[nTrace].nBoardNo   = -1;         // initial
        m_TraceList[nTrace].nChannelNo = -1;         // initial
        
        recount_list();     // 등록된 count와 active count를 다시 계산
    }
    pthread_mutex_unlock(&lock_mutex);
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : check
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER    IN: nBoardNo   - SES biard number
 *              IN: nChannelNo - SES channel number
 * RET. VALUE     : -1(NO Trace)/+n(YES Trace)
 * DESCRIPTION    : 해당 board/cnanel이 Trace가 걸려있는지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
int TRACE_TBL::check(int nBoardNo, int nChannelNo)
{
	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
        if((m_TraceList[i].nBoardNo == nBoardNo) && (m_TraceList[i].nChannelNo == nChannelNo))
		{
            return(i);
        }
    }
    
    return(-1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : check
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER    IN: nTrace     - Trace List Index
 *              IN: nBoardNo   - SES biard number
 *              IN: nChannelNo - SES channel number
 *              IN: ot_type    - ORIG/TERM 정보
 *              IN: strToIP    - SIP 메세지를 보낸 Dest IP
 *              IN: nToPort    - SIP 메세지를 보낸 Dest Port
 *              IN: strSendBuf - SIP 메세지 내용
 * RET. VALUE     : -1(NO Trace)/+n(YES Trace)
 * DESCRIPTION    : 해당 board/cnanel이 Trace가 걸려있는지 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
bool TRACE_TBL::trace(int nTrace, int nBoardNo, int nChannelNo, int ot_type, const char *strToIP, int nToPort, const char *strSendBuf)
{
    int     index;
    char    filename[128];
    FILE    *fp;

    if((nTrace < 0) || (nTrace >= MAX_TRACE_LIST))  // nTrace 범위 에러
    {
        Log.printf(LOG_ERR, "[TRACE_TBL] trace() nTrace Index[%d] Error\n", nTrace);
        return(false);
    }
    
    index = check(nBoardNo, nChannelNo);    // get nTrace Index
    if(nTrace != index)         //  다른 nTrace 값으로 Trace 요청하는 경우
    {
        Log.printf(LOG_LV2, "[TRACE_TBL] trace() Index[%d != %d] mismatch\n", nTrace, index); // FIXIT - mPBX BMT 과정에서 너무 많이 찍히는 경우가 발생 - 데이터 미스메치
        return(false);
    }
    
    if(m_TraceList[nTrace].nCount >= m_TraceList[nTrace].nMaxCount)  // MaxCount 만큼 처리 했음
    {
        return(false);
    }

    get_trace_filename(filename);
    
    pthread_mutex_lock(&file_mutex);
    {
        fp = fopen(filename, "a+");
        if(fp != NULL)
        {
            long            now;
            struct  tm      *today;
            struct  timeval LogTime;
            
            now   = time(NULL);
            today = localtime(&now);
            gettimeofday(&LogTime, NULL);

            fprintf(fp, "[%02d:%02d:%02d.%06d][INFO ][%c] ", today->tm_hour, today->tm_min, today->tm_sec, (int)LogTime.tv_usec, (ot_type=OT_TYPE_ORIG) ? 'O' : 'T');
            fprintf(fp, ">>>--->>> Send Message(0) To=[%s:%d] Trace=%d, Count=%d >>>--->>>\n%s\n", strToIP, nToPort, nTrace, m_TraceList[nTrace].nCount, strSendBuf);
            fclose(fp);
            
            Log.printf(LOG_LV1, "[TRACE_TBL] trace(%s) \n[%02d:%02d:%02d.%06d][INFO ][%c] >>>--->>> Send Message(0) To=[%s:%d] Trace=%d, Count=%d >>>--->>>\n%s\n", filename, today->tm_hour, today->tm_min, today->tm_sec, (int)LogTime.tv_usec, (ot_type=OT_TYPE_ORIG) ? 'O' : 'T', strToIP, nToPort, nTrace, m_TraceList[nTrace].nCount, strSendBuf);
        }
        
        m_TraceList[nTrace].nCount ++;
        if(m_TraceList[nTrace].nCount >= m_TraceList[nTrace].nMaxCount) { m_ActiveCount --; }    // MaxCount 만큼 처리 했음
    }
    pthread_mutex_unlock(&file_mutex);
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : sprint_trace
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER   OUT: strTrace   - Trace List 정보를 출력한 String
 * RET. VALUE     : strTrace Length
 * DESCRIPTION    : Trace List 정보를 Sting에 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
int TRACE_TBL::sprint_trace(char *strTrace)
{
    int     nIndex;
    
    // 20170207 snprintf -> sprintf로 변경 (함수 파라미터를 추가하지 않으면 strTrace size 를 알수 없음)
	nIndex = sprintf(strTrace, "0000\r\nINDEX\t2(0.0)\t200\tFROM\t48(0.0)\t200\tTO\t48(0.0)\t200\tSVC\t32(0.0)\t200\tMAX\t4(0.0)\t200\tCOUNT\t4(0.0)\t200\r\n");
    
	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
		nIndex += sprintf(strTrace+nIndex, "\1%02d\2\t\1%-48s\2\t\1%-48s\2\t\1%-32s\2\t\1%04d\2\t\1%04d\2\t\r\n",
                          i, m_TraceList[i].strFrom, m_TraceList[i].strTo, m_TraceList[i].strSvcKey, m_TraceList[i].nMaxCount, m_TraceList[i].nCount);
	}
    
    return(nIndex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : get_trace
 * CLASS-NAME     : TRACE_TBL
 * PARAMETER    IN: nTrace    - copy할 Trace List Index
 *             OUT: strFrom   - Trace List Index의 strFrom
 *             OUT: strTo     - Trace List Index의 strTo
 *             OUT: strSvcKey - Trace List Index의 strSvcKey
 *             OUT: nMaxCount - Trace List Index의 nMaxCount
 *             OUT: nCount    - Trace List Index의 nCount
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Trace List 정보를 복사하는 함수
 * REMARKS        :
 **end*******************************************************/
bool TRACE_TBL::get_trace(int nTrace, char *strFrom, char *strTo, char *strSvcKey, int *nMaxCount, int *nCount)
{
    if((nTrace < 0) || (nTrace >= MAX_TRACE_LIST))
    {
        Log.printf(LOG_ERR, "[TRACE_TBL] erase(%d) Index Error\n", nTrace);
        return(false);
    }
    
    strncpy(strFrom,   m_TraceList[nTrace].strFrom,   31);
    strncpy(strTo,     m_TraceList[nTrace].strTo,     31);
    strncpy(strSvcKey, m_TraceList[nTrace].strSvcKey, 31);
    *nMaxCount      = m_TraceList[nTrace].nMaxCount;
    *nCount         = m_TraceList[nTrace].nCount;
    
    return(true);
}


