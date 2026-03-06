/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : LIB_nsmlog.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : LIB
 SUBSYSTEM      : LIB
 SOR-NAME       :
 VERSION        : V1.X
 DATE           : 2014/07/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : Log 파일 생성/기록/백업 등을 관리하는 Class 함수
 REMARKS        :
 *end*************************************************************/


#include "libnsmlog.h"

const char LOG_LEVEL_STR[6][6] = { "", "LV1  ", "LV2  ", "LV3  ", "ERROR", "INFO " };

#pragma mark -
#pragma mark 생성자/파괴자 등 초기화 함수들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : NSM_LOG
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : NSM_LOG 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
NSM_LOG::NSM_LOG(void)
{
    snprintf(m_log_file_format, sizeof(m_log_file_format), "pid_%d_%%04d%%02d%%02d.txt", getpid());
    
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    
    m_fp        = NULL;
    m_level     = LOG_ERR;                  // default log level
    m_max_limit = 1024*1024*1024;           // default = 1G Byte
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~NSM_LOG
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : NSM_LOG 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
NSM_LOG::~NSM_LOG(void)
{
    if(m_fp != NULL) { fclose(m_fp); m_fp = 0; }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : init
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: path - Log 파일의 PATH 및 이름의 앞부분 (뒷부분은 YYYYMMDD.txt로 고정)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : NSM_LOG 초기화 함수 (로그파일명 생성 및 OPEN)
 * REMARKS        :
 **end*******************************************************/
bool NSM_LOG::init(const char *path)
{
    if(path[strlen(path)-1] == '_')
    {
        snprintf(m_log_file_format,    sizeof(m_log_file_format),    "%s%%04d%%02d%%02d.txt",    path);
        snprintf(m_backup_file_format, sizeof(m_backup_file_format), "%s%%04d%%02d%%02d.b%%03d", path);
    }
    else
    {
        snprintf(m_log_file_format,    sizeof(m_log_file_format),    "%s_%%04d%%02d%%02d.txt",    path);
        snprintf(m_backup_file_format, sizeof(m_backup_file_format), "%s_%%04d%%02d%%02d.b%%03d", path);
    }
    if((m_fp = fileopen()) == NULL) { return(false); }
    
    return(true);
}

#pragma mark -
#pragma mark Log file 관련 함수 들


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : get_filename
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : filename
 * DESCRIPTION    : Log 파일이름을 구하는 함수
 * REMARKS        :
 **end*******************************************************/
const char *NSM_LOG::get_filename(void)
{
    return(m_filename);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : fileopen
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : 0(ERR)/FILE *
 * DESCRIPTION    : LOG file을 open 하는 함수
 * REMARKS        :
 **end*******************************************************/
FILE *NSM_LOG::fileopen(void)
{
    struct  tm      p_now;
    struct  timeval t_now;
    
    if(gettimeofday(&t_now, NULL) != 0)            { return(0); }
    if(localtime_r(&t_now.tv_sec, &p_now) == NULL) { return(0); }
    
	snprintf(m_filename, sizeof(m_filename), m_log_file_format, p_now.tm_year+1900, p_now.tm_mon+1, p_now.tm_mday);
    m_open_day = p_now.tm_mday;    // open한 파일 날짜
    
    return(fopen(m_filename, "a+"));
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : close
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Log Handle을 close 하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_LOG::close(void)
{
    if(m_fp != 0) { fclose(m_fp); m_fp = 0; }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : check_file_size_limit
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : LOG file 크기가 m_max_limit 보다 크면 백업하는 함수
 * REMARKS        : 기존 파일을 move하고 기존 log이름으로 re-open
 **end*******************************************************/
void NSM_LOG::check_file_size_limit(struct tm *p_now)
{
    struct  stat statbuf;
    int     ret, nIndex;
    char    backup_filename[128];
    
    ret = stat(m_filename, &statbuf);
    
    if((ret == 0) && (statbuf.st_size > m_max_limit))
    {
        fclose(m_fp);     // 기존 log file close
        m_fp = NULL;      // RESET fp
        
        for(nIndex = 0; nIndex < 1000; nIndex ++)
        {
            snprintf(backup_filename, sizeof(backup_filename), m_backup_file_format, p_now->tm_year+1900, p_now->tm_mon+1, p_now->tm_mday, nIndex);
            if((stat(backup_filename, &statbuf) == -1) && (errno == ENOENT))
            {
                rename(m_filename, backup_filename);    // rename log file -> backup file
                m_fp = fileopen();
                return;
            }
        }
        
        // filename.100 까지 사용하고 있다?? BUG... 그래서 99로 강제 SET
        snprintf(backup_filename, sizeof(backup_filename), m_backup_file_format, p_now->tm_year+1900, p_now->tm_mon+1, p_now->tm_mday, 999);
        rename(m_filename, backup_filename);
        m_fp = fileopen();
    }
}


#pragma mark -
#pragma mark Log에 출력하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : printf_noansi
 * CLASS-NAME     : -
 * PARAMETER    IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: fmt       - 출력할 로그 format
 * RET. VALUE     : -
 * DESCRIPTION    : fmt에 해당하는 내용을 로드파일에 저장하는 함수
 * REMARKS        : 모두 흑백으로 출력 - close하지 않고 fflush()
 **end*******************************************************/
void NSM_LOG::printf_noansi(char nLogLevel, const char *fmt, ...)
{
    struct  tm      p_now;
    struct  timeval t_now;
    va_list arg;
    
    if(m_fp == NULL)        { return; }   // file open이 되어 있지 않음 (뭔가 오류 발생)
	if(nLogLevel < m_level) { return; }

    pthread_mutex_lock(&lock_mutex);
    {
        gettimeofday(&t_now, NULL);
        localtime_r(&t_now.tv_sec, &p_now);

        if(m_open_day != p_now.tm_mday)    // open한 날짜와 현재 날짜가 다름 - 날짜 변경
        {
            fclose(m_fp);
            if((m_fp = fileopen()) == NULL) { pthread_mutex_unlock(&lock_mutex); return; }
        }
        
        fprintf(m_fp, "[%02d:%02d:%02d.%06d][%s] ", p_now.tm_hour, p_now.tm_min, p_now.tm_sec, t_now.tv_usec, LOG_LEVEL_STR[nLogLevel]);
        
        va_start(arg, fmt);
        vfprintf(m_fp, fmt, arg);
        va_end(arg);
        
        fflush(m_fp);
        
        check_file_size_limit(&p_now);

    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : printf
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: fmt       - 출력할 로그 format
 * RET. VALUE     : -
 * DESCRIPTION    : fmt에 해당하는 내용을 로드파일에 저장하는 함수
 * REMARKS        : ERR인 경우 RED로 출력 - close하지 않고 fflush()
 **end*******************************************************/
void NSM_LOG::printf(char nLogLevel, const char *fmt, ...)
{
    struct  tm      p_now;
    struct  timeval t_now;
    va_list arg;
    
    if(m_fp == NULL)        { return; }   // file open이 되어 있지 않음 (뭔가 오류 발생)
	if(nLogLevel < m_level) { return; }
    
    pthread_mutex_lock(&lock_mutex);
    {
        gettimeofday(&t_now, NULL);
        localtime_r(&t_now.tv_sec, &p_now);
        
        if(m_open_day != p_now.tm_mday)    // open한 날짜와 현재 날짜가 다름 - 날짜 변경
        {
            fclose(m_fp);
            if((m_fp = fileopen()) == NULL) { pthread_mutex_unlock(&lock_mutex); return; }
        }
        
        if(nLogLevel == LOG_ERR) { fprintf(m_fp, "%s", COLOR_RED); }
        
        fprintf(m_fp, "[%02d:%02d:%02d.%06d][%s] ", p_now.tm_hour, p_now.tm_min, p_now.tm_sec, t_now.tv_usec, LOG_LEVEL_STR[nLogLevel]);
        
        va_start(arg, fmt);
        vfprintf(m_fp, fmt, arg);
        va_end(arg);
        
        if(nLogLevel == LOG_ERR) { fprintf(m_fp, "%s", COLOR_RESET); }
        
        fflush(m_fp);
        
        check_file_size_limit(&p_now);
        
    }
    pthread_mutex_unlock(&lock_mutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : color
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: color     - 출력 format에 적용할 ANSI color
 *              IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: fmt       - 출력할 로그 format
 * RET. VALUE     : -
 * DESCRIPTION    : fmt에 해당하는 내용을 로드파일에 저장하는 함수
 * REMARKS        : close하지 않고 fflush()
 **end*******************************************************/
void NSM_LOG::color(const char *color, char nLogLevel, const char *fmt, ...)
{
    struct  tm      p_now;
    struct  timeval t_now;
    va_list arg;
    
    if(m_fp == NULL)        { return; }   // file open이 되어 있지 않음 (뭔가 오류 발생)
	if(nLogLevel < m_level) { return; }
    
    pthread_mutex_lock(&lock_mutex);
    {
        gettimeofday(&t_now, NULL);
        localtime_r(&t_now.tv_sec, &p_now);
        
        if(m_open_day != p_now.tm_mday)    // open한 날짜와 현재 날짜가 다름 - 날짜 변경
        {
            fclose(m_fp);
            if((m_fp = fileopen()) == NULL) { pthread_mutex_unlock(&lock_mutex); return; }
        }
        
        
        fprintf(m_fp, "%s", color);
        fprintf(m_fp, "[%02d:%02d:%02d.%06d][%s] ", p_now.tm_hour, p_now.tm_min, p_now.tm_sec, t_now.tv_usec, LOG_LEVEL_STR[nLogLevel]);
        
        va_start(arg, fmt);
        vfprintf(m_fp, fmt, arg);
        va_end(arg);
        
        fprintf(m_fp, "%s", COLOR_RESET);
        
        fflush(m_fp);
        
        check_file_size_limit(&p_now);
        
    }
    pthread_mutex_unlock(&lock_mutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : cprintf
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: fmt       - 출력할 로그 format
 * RET. VALUE     : -
 * DESCRIPTION    : fmt에 해당하는 내용을 로드파일에 저장하는 함수
 * REMARKS        : [HH:MM:SS][LV ] 부분 없이 내용만 출력(C/C++ 함수와 동일)
 **end*******************************************************/
void NSM_LOG::cprintf(char nLogLevel, const char *fmt, ...)
{
    struct  tm      p_now;
    struct  timeval t_now;
    va_list arg;
    
    if(m_fp == NULL)        { return; }   // file open이 되어 있지 않음 (뭔가 오류 발생)
	if(nLogLevel < m_level) { return; }
    
    pthread_mutex_lock(&lock_mutex);
    {
        gettimeofday(&t_now, NULL);
        localtime_r(&t_now.tv_sec, &p_now);
        
        if(m_open_day != p_now.tm_mday)    // open한 날짜와 현재 날짜가 다름 - 날짜 변경
        {
            fclose(m_fp);
            if((m_fp = fileopen()) == NULL) { pthread_mutex_unlock(&lock_mutex); return; }
        }
        
        va_start(arg, fmt);
        vfprintf(m_fp, fmt, arg);
        va_end(arg);
        
        
        fflush(m_fp);
        
        check_file_size_limit(&p_now);
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : head
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: fmt       - 출력할 로그 format
 *              IN: strBuf    - 출력할 메세지
 *              IN: nLen      - 출력할 메세지 길이
 * RET. VALUE     : -
 * DESCRIPTION    : 메세지에서 처음부터 nLen 만큼만 출력하는 함수
 * REMARKS        : 긴 메세지의 일부만 출력시 사용
 **end*******************************************************/
void NSM_LOG::head(int nLogLevel, const char *fmt, char *strBuf, int nLen)
{
    char        temp;
    
    temp = strBuf[nLen];   // backup
    strBuf[nLen] = '\0';   // set NULL for print log
    printf(nLogLevel, "%s %s\n", fmt, strBuf);
    strBuf[nLen] = temp;   // restore
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : warning
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: fmt - 출력할 로그 format
 * RET. VALUE     : -
 * DESCRIPTION    : fmt에 해당하는 내용을 로그파일에 저장하는 함수
 * REMARKS        : LOG_LV3로 동작 - add 20160823
 **end*******************************************************/
void NSM_LOG::warning(const char *fmt, ...)
{
    struct  tm      p_now;
    struct  timeval t_now;
    va_list arg;
    
    if(m_fp == NULL)       { return; }      // file open이 되어 있지 않음 (뭔가 오류 발생)
    if(m_level >= LOG_OFF) { return; }      // LOG_OFF시 에는 warning 출력 안함 (실제로 Level 3와 동일한 Level로 간주 됨)
    
    pthread_mutex_lock(&lock_mutex);
    {
        gettimeofday(&t_now, NULL);
        localtime_r(&t_now.tv_sec, &p_now);
        
        if(m_open_day != p_now.tm_mday)    // open한 날짜와 현재 날짜가 다름 - 날짜 변경
        {
            fclose(m_fp);
            if((m_fp = fileopen()) == NULL) { pthread_mutex_unlock(&lock_mutex); return; }
        }

        fprintf(m_fp, "[%02d:%02d:%02d.%06d][%s] ", p_now.tm_hour, p_now.tm_min, p_now.tm_sec, t_now.tv_usec, "WARN ");
        
        va_start(arg, fmt);
        vfprintf(m_fp, fmt, arg);
        va_end(arg);

        fflush(m_fp);
        
        check_file_size_limit(&p_now);
        
    }
    pthread_mutex_unlock(&lock_mutex);
}

#define PRINT_COLUMN    69

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : hexdump
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLogLevel - 출력할 로그 LEVEL
 *              IN: hex_buf   - 출력할 메세지
 *              IN: nLen      - 출력할 메세지 길이
 *              IN: strTitle  - 출력할 Title
 * RET. VALUE     : -
 * DESCRIPTION    : 메세지에서 처음부터 nLen 만큼만 출력하는 함수
 * REMARKS        : 긴 메세지의 일부만 출력시 사용
 **end*******************************************************/
void NSM_LOG::hexdump(int nLogLevel, const char *hex_buf, int nLen, const char *strTitle)
{
    int     index_hex = 0, index_ascii = 0, line = 0;
    char    out_hex[256], out_ascii[128];
    
    // printf Title
    // printf(nLogLevel, "================= %s(%4d) =================\n", strTitle, nLen);
    {
        int i, j, tlen;
        int index = 0;
        
        if((tlen = (int)strlen(strTitle)) > 60)
        {
            printf(LOG_ERR, "hexdump() title too big(%d) > 60\n", tlen);
            return;
        }
        
        tlen += 10;     // title 좌우에 공백과 괄호를 포한한 길이를 출력하는데 10자리가 소모됨
        j = (PRINT_COLUMN - tlen) / 2;
        
        for(i = 0; i < j; i ++) { index += sprintf(&out_ascii[index], "="); }
        index += sprintf(&out_ascii[index], "  %s(%4d)  ", strTitle, nLen);
        for(i = 0; i < j; i ++) { index += sprintf(&out_ascii[index], "="); }
        if(strlen(out_ascii) < PRINT_COLUMN) { strcat(out_ascii, "="); }        // title 길이가 홀수인 경우에 "=" 하나 더 추가

        printf(nLogLevel, "%s\n", out_ascii);
    }

    // print contents
    for(int i = 0; i < nLen; i ++)
    {
        index_hex   += sprintf(&out_hex[index_hex],     "%02X ", hex_buf[i]);
        index_ascii += sprintf(&out_ascii[index_ascii], "%c", (isprint(hex_buf[i]) ? hex_buf[i] : '.'));
        
        if((i % 16) == 15)
        {
            if(i == 0) continue;
            printf(nLogLevel, "%02d: %s %s\n", line, out_hex, out_ascii);
            line ++;
            index_hex = 0;
            index_ascii = 0;
        }
    }
    
    if(index_hex != 0) { printf(nLogLevel, "%d: %-48s %s\n", line, out_hex, out_ascii); }
    
    // print tail
    printf(nLogLevel, "=====================================================================\n");
}

#pragma mark -
#pragma mark Log Level 및 limit 관련 함수들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : set_level
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLevel - 설정할 로그 LEVEL
 * RET. VALUE     : -
 * DESCRIPTION    : 로그 레벨을 설정하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_LOG::set_level(int8_t nLevel)
{
    switch(nLevel)
    {
// 20210916 bible :  AS    
//		case LOG_LV1: m_level = LOG_LV2; return;    // 20160823 - level 1은 setting 하지 못하게 변경 (운용자 실수 방지)
		case LOG_LV1:
        case LOG_LV2:
        case LOG_LV3: m_level = nLevel;  return;
        default:      m_level = LOG_ERR; return;
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : set_debug
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 로그 레벨을 LOG_LV1으로 설정하는 함수
 * REMARKS        : add 20160823
 **end*******************************************************/
void NSM_LOG::set_debug()
{
    m_level = LOG_LV1;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : get_level
 * CLASS-NAME     : NSM_LOG
 * PARAMETER      : -
 * RET. VALUE     : n -  현재 설정된 로그 LEVEL
 * DESCRIPTION    : 로그 레벨을 조회 하는 함수
 * REMARKS        :
 **end*******************************************************/
int8_t NSM_LOG::get_level(void)
{
    return(m_level);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : set_max_limit
 * CLASS-NAME     : NSM_LOG
 * PARAMETER    IN: nLimit - 로그 파일의 최대 크기
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 로그 파일의 최대 크기를 설정하는 함수
 * REMARKS        : 100M <= nLimit < 2G
 **end*******************************************************/
bool NSM_LOG::set_max_limit(uint32_t nLimit)
{
    if((100000000 <= nLimit) && (nLimit < (uint32_t)(2000000000)))
    {
        m_max_limit = nLimit;
        return(true);
    }
    return(false);
}


