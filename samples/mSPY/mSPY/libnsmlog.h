//
//  libsmlog.h
//  LIB
//
//  Created by SMCHO on 2014. 4. 16..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __LIB_NSMLOG_H__
#define __LIB_NSMLOG_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

// Log.printf
#define	LOG_INF         5
#define LOG_ERR         4
#define LOG_OFF         LOG_ERR
#define	LOG_LV3         3
#define	LOG_LV2         2
#define	LOG_LV1         1

// ANSI COLOR CODE
#define COLOR_GRAY      "\033[0;47;30m"
#define COLOR_RED       "\033[0;40;31m"
#define COLOR_GREEN     "\033[0;40;32m"
#define COLOR_YELLOW    "\033[0;40;33m"
#define COLOR_BLUE      "\033[0;40;34m"
#define COLOR_MAGENTA   "\033[0;40;35m"
#define COLOR_CYAN      "\033[0;40;36m"
#define COLOR_WHITE     "\033[0;40;37m"

// R: Reverse
#define COLOR_R_GRAY    "\033[0;40;37m"
#define COLOR_R_RED     "\033[0;41;37m"
#define COLOR_R_GREEN   "\033[0;42;37m"
#define COLOR_R_YELLOW  "\033[0;43;37m"
#define COLOR_R_BLUE    "\033[0;44;37m"
#define COLOR_R_MAGENTA "\033[0;45;37m"
#define COLOR_R_CYAN    "\033[0;46;37m"
#define COLOR_R_WHITE   "\033[0;47;30m"

#define COLOR_RESET     "\033[0m"


#ifndef MAX_LOG_NAME_LEN
#   define MAX_LOG_NAME_LEN     128
#endif

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : NSM_LOG
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : Log Class
 *                : 1. 한번 file open을 하면 close를 하지 않고 지속적으로 write
 *                :    하며 매번 fflush()를 수행한다.
 *                : 2. 특정 size이상 또는 날짜가 바뀌면 close를 하고 새로운 파일을 open
 * REMARKS        :
 **end*******************************************************/
class NSM_LOG
{
private:
    pthread_mutex_t lock_mutex;     // mutex for memory access
    pthread_mutex_t file_mutex;     // mutex for file access
    
    int         m_open_day;         // 현재 open된 로그 파일 이름의 날짜 - 날이 바뀌면 파일을 바꾸기 위해
    uint32_t    m_max_limit;        // 최대 file size (단위는 BYTE)
    int8_t      m_level;            // Log Level
    
    char        m_log_file_format[MAX_LOG_NAME_LEN+1];      // 로그 파일의 Format
    char        m_backup_file_format[MAX_LOG_NAME_LEN+1];   // 로그 백업파일의 Format
    char        m_filename[MAX_LOG_NAME_LEN+1];            // open된 Log 파일 이름
    FILE        *m_fp;              // Log file pointer
    FILE        *fileopen(void);    // Log file 을 Open
    void        check_file_size_limit(struct tm *p_now);    // Log 파일 size가 limit 보다 큰지 검사하는 함수
    
public:
    NSM_LOG(void);
	~NSM_LOG(void);
    
    bool    init(const char *path);         // Log 파일의 PATH 및 이름의 앞부분 (뒷부분은 YYYYMMDD.txt로 고정)
    void    set_level(int8_t nLevel);       // Log Level을 설정하는 함수
    int8_t  get_level(void);                // 설정된 Log Level을 조회하는 함수
    bool    set_max_limit(uint32_t nLimit); // Log 파일의 최대 Size를 지정하는 함수
    const char *get_filename(void);         // Log 파일이름을 구하는 함수
    void    close(void);                    // Open되어 있는 Log FP를 close하는 함수
    
    void    printf(char nLogLevel, const char *fmt, ...);           // Log printf (ERROR이면 RED color)
    void    printf_noansi(char nLogLevel, const char *fmt, ...);    // ANSI 적용안함 (ERROR도 mono)
    void    cprintf(char nLogLevel, const char *fmt, ...);          // [HH:MM:SS][LV ] 없이 그대로 출력만하는 함수
    void    head(int nLogLevel, const char *fmt, char *strBuf, int nLen);   // 문자열 앞부분만 출력하는 함수
    void    color(const char *color, char nLogLevel, const char *fmt, ...);   // color로 printf
    
    void    warning(const char *fmt, ...);                                      // add 160823 by SMCHO
    void    hexdump(int nLogLevel, const char *hex_buf, int nLen, const char *strTitle); // add 160823 by SMCHO
    void    set_debug();                                                        // add 160823 by SMCHO - log level을 DEBUG level(LV1)로 설정
};

#endif /* defined(__LIB_NSMLOG_H__) */
