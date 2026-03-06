
/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : LIB_nsmtimer.h
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : LIB
 SUBSYSTEM      : LIB
 SOR-NAME       :
 VERSION        : V1.X
 DATE           :
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        : 2015/11/27
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : New Timer Library
 REMARKS        : insert/delete시 MAP 보다는 HASH MAP이 30% 정도 속도가 빠르다.
                : T/O 처리 시 List 보다는 vector가 30% 정도 빠르다
 *end*************************************************************/

#ifndef __LIB_NSMTIMER_H__
#define __LIB_NSMTIMER_H__

#ifdef USE_HASH_MAP
#   include <ext/hash_map>
using namespace __gnu_cxx;
#else
#   include <map>
#endif

#include <vector>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>


using namespace std;

typedef void (*STMR_CALLBACK)(size_t, size_t, size_t);

typedef enum
{
    ST_OK                     = 0,
    ST_ERR_NOT_INITIALIZED    = -1,
    ST_ERR_MALLOC_FAIL        = -2,
    ST_ERR_TIME_TOO_BIG       = -3,
    ST_ERR_TIME_TOO_SMALL     = -4,
    ST_ERR_TABLE_FULL         = -5,
    ST_ERR_TIMER_NOT_SETTED   = -6,
    ST_ERR_UNDEFINED          = -99
} NSM_RESULT;

typedef struct
{
    size_t      key;        // map에 insert/update/find하는데 사용하는 key
    time_t      t_TO;       // timeout이 되는 시간
    size_t		arg1;       // func이 실행될때 파라미터
    size_t		arg2;       // func이 실행될때 파라미터
} STMR_TBL;

#define     MAX_TIME_VALUE      3600
#define     MAX_TIMER_COUNT     50000

// Local Timer Class (내가 1/2 SE Timer를 구동하는 경우)
class NSM_SEC_TIMER
{
    friend void *hm_local_timer_thread(void *arg);
    
private:
#ifdef USE_HASH_MAP
    hash_map<size_t, STMR_TBL *>    m_stmr_map;
#else
    map<size_t, STMR_TBL *>         m_stmr_map;
#endif
    pthread_mutex_t                 m_stmr_mutex;
    pthread_t                       m_tid;
    STMR_CALLBACK                   m_to_func;          // T/O 발생시 callback 될 함수

    size_t      m_stat_count;
    size_t      m_stat_set;         // set count
    size_t      m_stat_cancel;      // cancel count
    size_t      m_stat_to;          // T/O count
    
public:
    NSM_SEC_TIMER();
    ~NSM_SEC_TIMER();
    
    NSM_RESULT  init(STMR_CALLBACK to_func);                                    // Timer Class 초기화
    NSM_RESULT  set(size_t key, uint32_t n_sec, size_t arg1, size_t arg2);      // 새로운 타이머를 SET
    NSM_RESULT  update(size_t key, uint32_t n_sec, size_t arg1, size_t arg2);   // 기존 타이머를 UPDATE
    NSM_RESULT  cancel(size_t key);                                             // 기존 타이머 취소
    NSM_RESULT  check(size_t key, uint32_t *n_sec);                             // Timer가 얼마 남았는지 check하는 함수
    void        check_time_out(void);                                           // T/O 여부를 주기적으로 check
    size_t      count(void);                                                    // 현제 등록된 timer_count를 출력
    void        stat(size_t *n_set, size_t *n_cancel, size_t *n_to);            // 통계
    void        stat_reset(void);                                               // 통계 RESET
    size_t      size();
};


#endif // __LIB_NSMTIMER_H__
