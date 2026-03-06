
/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : LIB_timer2.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : LIB
 SUBSYSTEM      :
 SOR-NAME       :
 VERSION        : V2.0
 DATE           : 2013/12/30
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : MAP을 이용한 Timer
 REMARKS        :
 *end*************************************************************/

#include    <map>
#include    <list>

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include 	<sys/time.h>
#include	<pthread.h>

#include	"SPY_timer.h"


#define	MAX_TIMER_TABLE		20000
#define USEC_1SEC           1000000
//#define RTC_CHECK_UNIT      100000          // 100000 usec = 100 msec 마다 한번씩 T/O check를 함.
//#define RTC_CHECK_UNIT      50000          // 50000 usec = 50 msec 마다 한번씩 T/O check를 함.
#define RTC_CHECK_UNIT      25000          // 25000 usec = 25 msec 마다 한번씩 T/O check를 함.

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : get_reg_timer_cnt
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : cnt(현재 등록중인 타이머 수)
 * DESCRIPTION    : 현재 등록중인 타이머 수를 조회하는 함수
 * REMARKS        :
 **end*******************************************************/
void TIMER::get_timer_err_cnt(int *unit_err, int *tsmall_err, int *mem_err, int *tbl_full)
{
    
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : get_reg_timer_cnt
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : cnt(현재 등록중인 타이머 수)
 * DESCRIPTION    : 현재 등록중인 타이머 수를 조회하는 함수
 * REMARKS        :
 **end*******************************************************/
int TIMER::get_reg_timer_cnt(void)
{
    return(m_reg_timer_cnt);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : set
 * CLASS-NAME     : TIMER
 * PARAMETER    IN: t    - Timer 설정 값
 *              IN: unit - 타이머(t) 설정 단위(MSEC/SEC/MIN/HOUR/DAY)
 *              IN: func - timeout 이 발생되면 callback 될 함수
 *              IN: arg1 - callback 되는 함수의 첫번째 인자
 *              IN: arg2 - callback 되는 함수의 두번 인자
 * RET. VALUE     : timer_id/-(ERR)
 * DESCRIPTION    : 새로운 timer를 설정하는 함수
 * REMARKS        :
 **end*******************************************************/
int64_t TIMER::set(int t, int unit, TO_FUNC func, size_t arg1, size_t arg2)
{
	TMRTBL	*tmr;
    struct  timeval now;
    int     usec, sec;
    
	if(m_reg_timer_cnt >= MAX_TIMER_TABLE) { m_err_full ++;   return(RET_TIMER_TABLE_FULL); }
	if((t < 100) && (unit == TIME_MSEC))   { m_err_tsmall ++; return(RET_TIME_TOO_SMALL); }    // minimum 100 msec
    
    gettimeofday(&now, NULL);
    
	switch(unit)
	{
		case TIME_MSEC:
            sec  = t / 1000;                // /1000 = msec -> sec
            usec = (t % 1000) * 1000;       // *1000 = msec -> usec
            
            now.tv_sec  += sec;
            now.tv_usec += usec;
            
            if(now.tv_usec > USEC_1SEC)         // 1초가 넘으면
            {
                now.tv_sec  += 1;
                now.tv_usec -= USEC_1SEC;
            }
            break;
		case TIME_SEC:  now.tv_sec += t;            break;
		case TIME_MIN:  now.tv_sec += (t * 60);     break;
		case TIME_HOUR: now.tv_sec += (t * 3600);   break;
		case TIME_DAY:  now.tv_sec += (t * 86400);  break;
		default:
            m_err_unit ++;
            return(RET_TIME_UNIT_ERR);
	}

    // malloc된 TMRTBL은 timeout이나 cancel시에 free()된다.
	if((tmr = (TMRTBL *)malloc(sizeof(TMRTBL))) == 0) { m_err_mem ++; return(RET_MEM_ERR); }

	tmr->to   = now;        // timeout 되는 시간
	tmr->func = func;       // timeout 되면 callback 되는 함수
	tmr->arg1 = arg1;       // timeout 될때 호출되는 callback 함수의 첫 번째 인자
	tmr->arg2 = arg2;       // timeout 될때 호출되는 callback 함수의 두 번째 인자

	// Lock for multi-thread
	pthread_mutex_lock(&m_timer_mutex);
	{
		if((++m_make_timer_id) < 0) { m_make_timer_id = 0; }
		tmr->id = m_make_timer_id;       // 할당된 timer_id

        m_timer_map.insert(make_pair((int)tmr->id, tmr));
		m_reg_timer_cnt ++;
	}
	pthread_mutex_unlock(&m_timer_mutex);
    
	return(tmr->id);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : cancel
 * CLASS-NAME     : TIMER
 * PARAMETER    IN: timer_id - 취소 할 timer_id 값
 * RET. VALUE     : bool
 * DESCRIPTION    : timer_id에 해당하는 Timer를 취소하는 함수
 * REMARKS        :
 **end*******************************************************/
int TIMER::cancel(int64_t timer_id)
{
    TMRTBL	*tmr;
	map<int64_t, TMRTBL *>::iterator it;

	pthread_mutex_lock(&m_timer_mutex);
    {
        it = m_timer_map.find(timer_id);
        
        if(it != m_timer_map.end())
        {
            tmr = it->second;
            free(tmr);                              // free memory, malloced in set()
            m_timer_map.erase(it);
            m_reg_timer_cnt --;
            pthread_mutex_unlock(&m_timer_mutex);
            return(1);
        }
    }
    pthread_mutex_unlock(&m_timer_mutex);
    
	return(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : cancel
 * CLASS-NAME     : TIMER
 * PARAMETER    IN: timer_id - 남은 시간을 check할 timer_id
 * RET. VALUE     : -1/MSEC (timeout 까지 남은 시간)
 * DESCRIPTION    : TIMEOUT 까지 얼마나 남았는지 Check하는 함수
 * REMARKS        :
 **end*******************************************************/
int TIMER::check(int64_t timer_id)
{
	TMRTBL	*tmr;
    struct timeval now, remain;
	map<int64_t, TMRTBL *>::iterator it;

    it = m_timer_map.find(timer_id);
    
    if(it != m_timer_map.end())
    {
        tmr = it->second;
        gettimeofday(&now, NULL);
        
        if(tmr->to.tv_usec > now.tv_usec)
        {
            remain.tv_sec  = tmr->to.tv_sec  - now.tv_sec;
            remain.tv_usec = tmr->to.tv_usec - now.tv_usec;
        }
        else
        {
            remain.tv_sec  = tmr->to.tv_sec  - now.tv_sec  - 1;
            remain.tv_usec = tmr->to.tv_usec - now.tv_usec + USEC_1SEC;
        }
        
        if(remain.tv_sec < 0) { return(0); }                        // something err... (아마도 T/O 시점에 check를 호출한 경우로 예상됨)
        
        return((int)((remain.tv_sec*1000) + (remain.tv_usec/1000)));       //  SEC->MSEC + USEC->MSEC
    }
	return(-1);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : check_time_out
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 주기적(100 msec)으로 TMRTNL이 timeout이 되었는지 check 하고
 *                : timeout이 된 TMRTBL은 저장된 함수와 인자로 callback 한다.
 * REMARKS        :
 **end*******************************************************/
void TIMER::check_time_out(void)
{
    map<int64_t, TMRTBL *>::iterator    map_it;
    list<TMRTBL *>                  tmr_list;
    list<TMRTBL *>::iterator        tmr_it;
    struct timeval                  now;
    TMRTBL                          *tmr;
    
    tmr_list.clear();       // init

    while(true)
    {
        usleep(RTC_CHECK_UNIT);
        
        gettimeofday(&now, NULL);

        // Check Timeout
        pthread_mutex_lock(&m_timer_mutex);
        {
            for(map_it = m_timer_map.begin(); map_it != m_timer_map.end(); )
            {
                tmr = map_it->second;
                
                if ((tmr->to.tv_sec < now.tv_sec)
                || ((tmr->to.tv_sec == now.tv_sec) && (tmr->to.tv_usec <= now.tv_usec)))
                {
                    tmr_list.push_back(map_it->second);     // TMRTBL address
                    m_timer_map.erase(map_it++);            // 주의: for(;; map_it++)를 하면 runtime error 발생 됨
                    m_reg_timer_cnt --;
                }
                else
                {
                    ++map_it;
                }
            }
        }
        pthread_mutex_unlock(&m_timer_mutex);

        // callback timeout functions
        for(tmr_it = tmr_list.begin(); tmr_it != tmr_list.end(); tmr_it ++)
        {
            tmr = *tmr_it;
            
            (*tmr->func)(tmr->arg1, tmr->arg2);
            free(tmr);
        }
        tmr_list.clear();
	}
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : timer_thread
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Timer Thread
 * REMARKS        : friend
 **end*******************************************************/
void *timer_thread(void *arg)
{
    TIMER *timer = (TIMER *)arg;
    
    if(timer->b_run_flag == true) { return(NULL); }  // already running
    
    timer->b_run_flag = true;
	timer->check_time_out();		// endless loop
    timer->b_run_flag = false;
    return(NULL);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : msp size
 * REMARKS        :
 **end*******************************************************/
size_t TIMER::size()
{
    return(m_timer_map.size());
}


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : TIMER
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 생성자
 * REMARKS        :
 **end*******************************************************/
TIMER::TIMER()
{

	m_reg_timer_cnt = 0;
    m_make_timer_id = 0;
    b_run_flag      = false;
    m_err_unit = m_err_tsmall = m_err_mem = m_err_full = 0;

//    m_timer_mutex   = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&m_timer_mutex, NULL);

    // FIXIT: thread 우선 순위 조정 필요??
	pthread_create(&thread_id, 0, timer_thread, (void *)this);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : ~TIMER
 * CLASS-NAME     : TIMER
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : 소멸자
 * REMARKS        :
 **end*******************************************************/
TIMER::~TIMER()
{
}



