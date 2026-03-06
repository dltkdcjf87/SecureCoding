
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

#include "libnsmtimer.h"

#pragma mark -
#pragma mark NSM_SEC_TIMER class에서 사용하는 non-member 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : nsm_no_operation
 * CLASS-NAME     :
 * PARAMETER    IN: key  - not used
 *              IN: arg1 - not used
 *              IN: arg2 - not used
 * RET. VALUE     : -
 * DESCRIPTION    : 아무런 일도 하지 않는 함수
 * REMARKS        : callback 함수 초기화에 사용한다.
 **end*******************************************************/
void nsm_no_operation(size_t key, size_t arg1, size_t arg2)
{
    return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : nsm_sec_timer_thread
 * CLASS-NAME     :
 * PARAMETER    IN: arg1 - not used
 *              IN: arg2 - not used
 * RET. VALUE     : -
 * DESCRIPTION    : T/O을 check하는 Thread를 실행하는 함수
 * REMARKS        : endless function
 **end*******************************************************/
void *nsm_sec_timer_thread(void *arg)
{
    NSM_SEC_TIMER *timer = (NSM_SEC_TIMER *)arg;
    
    timer->check_time_out();
    return(NULL);
}


#pragma mark -
#pragma mark NSM_SEC_TIMER member 함수

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : check_time_out
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 주기적(100 msec)으로 TMRTNL이 timeout이 되었는지 check 하고
 *                : timeout이 된 TMRTBL은 저장된 함수와 인자로 callback 한다.
 * REMARKS        : T/O check 시 vector가 list 보다 30%정도 빠르다
 **end*******************************************************/
void NSM_SEC_TIMER::check_time_out(void)
{
#ifdef USE_HASH_MAP
    hash_map<size_t, STMR_TBL *>::iterator  map_it;
#else
    map<size_t, STMR_TBL *>::iterator  map_it;
#endif
    vector<STMR_TBL *>::iterator            tmr_it;
    vector<STMR_TBL *>  tmr_vector;
    time_t      now;
    STMR_TBL    *tmr;

    tmr_vector.clear();         // init
    
    while(true)
    {
        sleep(1);               // sec_timer는 아주 정확하지 않아도 되기 때문에 usleep()이 아닌 sleep() 사용

        now = time(NULL);
        
        // Check Timeout
        pthread_mutex_lock(&m_stmr_mutex);
        {
            for(map_it = m_stmr_map.begin(); map_it != m_stmr_map.end(); )
            {
                tmr = map_it->second;
                
                if(tmr->t_TO <= now)
                {
                    tmr_vector.push_back(map_it->second);     // TMRTBL address
                    m_stmr_map.erase(map_it++);             // 주의: for(;; map_it++)를 하면 runtime error 발생 됨
                    m_stat_to ++;
                }
                else
                {
                    ++map_it;
                }
            }
            m_stat_count = m_stmr_map.size();
        }
        pthread_mutex_unlock(&m_stmr_mutex);
        
        // callback timeout functions
        for(tmr_it = tmr_vector.begin(); tmr_it != tmr_vector.end(); tmr_it ++)
        {
            tmr = *tmr_it;
            
            (*m_to_func)(tmr->key, tmr->arg1, tmr->arg2);
            free(tmr);
        }
        
        tmr_vector.clear();
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : NSM_SEC_TIMER
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : constructor
 * REMARKS        :
 **end*******************************************************/
NSM_SEC_TIMER::NSM_SEC_TIMER()
{
    m_to_func       = nsm_no_operation;     // T/O 발생시 callback 될 함수
    
    m_stat_count    = 0;
    m_stat_set      = 0;
    m_stat_cancel   = 0;
    m_stat_to       = 0;
    
    pthread_mutex_init(&m_stmr_mutex, NULL);
    
    pthread_create(&m_tid, 0, nsm_sec_timer_thread, (void *)this);
    pthread_detach(m_tid);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~NSM_SEC_TIMER
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : destructor
 * REMARKS        :
 **end*******************************************************/
NSM_SEC_TIMER::~NSM_SEC_TIMER()
{
    pthread_cancel(m_tid);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : init
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER    IN: n_sec - 타이머 등록시 T/O될 시간
 *              IN: to_func - T/O이 되면 callback될 함수
 * RET. VALUE     : t_result
 * DESCRIPTION    : 타이머를 동작시킬 시간값을 설정한다.
 * REMARKS        : 가변 시간은 지원하지 않고 고정됨 시간에 대한 타이머를 설정한다.
 **end*******************************************************/
NSM_RESULT NSM_SEC_TIMER::init(STMR_CALLBACK to_func)
{
    m_to_func = to_func;

    return(ST_OK);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : set
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER    IN: key   - The key of this timer
 *              IN: n_sec - n_sec 시간이 지난 후에 T/O 발생
 *              IN: arg1  - callback 되는 함수의 첫번째 인자
 *              IN: arg2  - callback 되는 함수의 두번 인자
 * RET. VALUE     : NSM_RESULT
 * DESCRIPTION    : 새로운 timer를 등록하는 함수
 * REMARKS        :
 **end*******************************************************/
NSM_RESULT NSM_SEC_TIMER::set(size_t key, uint32_t n_sec, size_t arg1, size_t arg2)
{
    STMR_TBL   *table;

    if(n_sec >= MAX_TIME_VALUE) { return(ST_ERR_TIME_TOO_BIG); }
    
    pthread_mutex_lock(&m_stmr_mutex);
    {
        if(m_stmr_map.size() >= MAX_TIMER_COUNT) { pthread_mutex_unlock(&m_stmr_mutex); return(ST_ERR_TABLE_FULL); }
        
        if((table = (STMR_TBL *)malloc(sizeof(STMR_TBL))) == NULL)
        {
            pthread_mutex_unlock(&m_stmr_mutex);
            return(ST_ERR_MALLOC_FAIL);
        }
        
        table->key  = key;
        table->t_TO = time(NULL) + n_sec;      // 현재 시간 + m_time이 T/O이 발생되어야 할 시간
        table->arg1 = arg1;
        table->arg2 = arg2;
        m_stat_set ++;                 // stat.
        
        m_stmr_map.insert(make_pair(key, table));
        m_stat_count ++;           // table count 증가
        
    }
    pthread_mutex_unlock(&m_stmr_mutex);
    
    return(ST_OK);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : update
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER    IN: key   - The key of this timer
 *              IN: n_sec - n_sec 시간이 지난 후에 T/O 발생
 *              IN: arg1 - callback 되는 함수의 첫번째 인자
 *              IN: arg2 - callback 되는 함수의 두번 인자
 * RET. VALUE     : NSM_RESULT
 * DESCRIPTION    : 기존 timer를 UPDATE하는 함수
 * REMARKS        :
 **end*******************************************************/
NSM_RESULT NSM_SEC_TIMER::update(size_t key, uint32_t n_sec, size_t arg1, size_t arg2)
{
    STMR_TBL   *table;
#ifdef USE_HASH_MAP
    hash_map<size_t, STMR_TBL *>::iterator  it;
#else
    map<size_t, STMR_TBL *>::iterator  it;
#endif
    
    if(n_sec >= MAX_TIME_VALUE) { return(ST_ERR_TIME_TOO_BIG); }
    
    pthread_mutex_lock(&m_stmr_mutex);
    {
        if((it = m_stmr_map.find(key)) != m_stmr_map.end())    // 기존에 있었음 - UPDATE
        {
            table = it->second;
            
            table->t_TO = time(NULL) + n_sec;      // 현재 시간 + m_time이 T/O이 발생되어야 할 시간
            table->arg1 = arg1;
            table->arg2 = arg2;
        }
        else    // 기존에 없었음 - INSERT
        {
            if(m_stmr_map.size() >= MAX_TIMER_COUNT) { pthread_mutex_unlock(&m_stmr_mutex); return(ST_ERR_TABLE_FULL); }
            
            if((table = (STMR_TBL *)malloc(sizeof(STMR_TBL))) == NULL)
            {
                pthread_mutex_unlock(&m_stmr_mutex);
                return(ST_ERR_MALLOC_FAIL);
            }
            
            table->key  = key;
            table->t_TO = time(NULL) + n_sec;      // 현재 시간 + m_time이 T/O이 발생되어야 할 시간
            table->arg1 = arg1;
            table->arg2 = arg2;
            m_stat_set ++;                 // stat.
            
            m_stmr_map.insert(make_pair(key, table));
            m_stat_count ++;           // table count 증가
        }
    }
    pthread_mutex_unlock(&m_stmr_mutex);
    
    return(ST_OK);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : cancel
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER    IN: key   - The key of this timer
 * RET. VALUE     : t_result
 * DESCRIPTION    : 등록된 timer를 취소하는 함수
 * REMARKS        :
 **end*******************************************************/
NSM_RESULT NSM_SEC_TIMER::cancel(size_t key)
{
    STMR_TBL   *table;
#ifdef USE_HASH_MAP
    hash_map<size_t, STMR_TBL *>::iterator it;
#else
    map<size_t, STMR_TBL *>::iterator  it;
#endif
    
    pthread_mutex_lock(&m_stmr_mutex);
    {
        it = m_stmr_map.find(key);
        
        if(it != m_stmr_map.end())
        {
            table = it->second;
            free(table);                                    // free memory, malloced in set()
            m_stmr_map.erase(it);
            m_stat_cancel ++;
            if(m_stat_count != 0) { m_stat_count --; }      // count underflow error 방지
            
            pthread_mutex_unlock(&m_stmr_mutex);
            return(ST_OK);
        }
    }
    pthread_mutex_unlock(&m_stmr_mutex);
    return(ST_ERR_TIMER_NOT_SETTED);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : check
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER    IN: key   - The key of this timer
 *              IN: n_sec - T/O 까지 남은 시간
 * RET. VALUE     : NSM_RESULT
 * DESCRIPTION    : 새로운 timer를 등록하는 함수
 * REMARKS        :
 **end*******************************************************/
NSM_RESULT NSM_SEC_TIMER::check(size_t key, uint32_t *n_sec)
{
    STMR_TBL   *table;
#ifdef USE_HASH_MAP
    hash_map<size_t, STMR_TBL *>::iterator it;
#else
    map<size_t, STMR_TBL *>::iterator  it;
#endif
    
    pthread_mutex_lock(&m_stmr_mutex);
    {
        it = m_stmr_map.find(key);
        
        if(it != m_stmr_map.end())
        {
            table = it->second;
            *n_sec = (uint32_t)(table->t_TO - time(NULL));
            
            pthread_mutex_unlock(&m_stmr_mutex);
            return(ST_OK);
        }
    }
    pthread_mutex_unlock(&m_stmr_mutex);
    
    return(ST_ERR_TIMER_NOT_SETTED);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : count
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : count
 * DESCRIPTION    : 현재 등록된 타이머 갯수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
size_t NSM_SEC_TIMER::count(void)
{
    return(m_stat_count);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : stat
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER   OUT: n_set    - timer 등록한 횟 수
 *             OUT: n_cancel - timer를 취소한 횟 수
 *             OUT: n_to     - time out이 발생한 횟 수
 * RET. VALUE     : count
 * DESCRIPTION    : 타이머와 관련된 통계를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_SEC_TIMER::stat(size_t *n_set, size_t *n_cancel, size_t *n_to)
{
    *n_set    = m_stat_set;
    *n_cancel = m_stat_cancel;
    *n_to     = m_stat_to;
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : stat_reset
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 타이머와 관련된 통계를 RESET하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_SEC_TIMER::stat_reset(void)
{
    pthread_mutex_lock(&m_stmr_mutex);
    {
        m_stat_set      = 0;
        m_stat_cancel   = 0;
        m_stat_to       = 0;
    }
    pthread_mutex_unlock(&m_stmr_mutex);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : NSM_SEC_TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 타이머와 관련된 통계를 RESET하는 함수
 * REMARKS        :
 **end*******************************************************/
size_t NSM_SEC_TIMER::size(void)
{
    return(m_stmr_map.size());
}


