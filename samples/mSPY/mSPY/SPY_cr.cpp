/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_cr.cpp
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
 DESCRIPTION    : Call Register를 할당/해제/관리하는 Class 함수
 REMARKS        :
 *end*************************************************************/

#include "SPY_cr.h"
#include "libnsmlog.h"

extern NSM_LOG   Log;

extern void shut_down(int reason);

//#pragma mark -
//#pragma mark [SPY_CR] 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_CR
 * CLASS-NAME     : SPY_CR
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SPY_CR 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
SPY_CR::SPY_CR(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    
    clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~SPY_CR
 * CLASS-NAME     : SPY_CR
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SPY_CR 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
SPY_CR::~SPY_CR(void)
{

}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : SPY_CR
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SPY_CR 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
void SPY_CR::clear(void)
{
    malloc_err_cnt     = 0;         // malloc이 연속적으로 실패한 횟수
    sum_malloc_err_cnt = 0;         // malloc 실패 횟 수
    sum_malloc_cnt     = 0;         // malloc 성공 횟 수
    sum_free_cnt       = 0;         // free 횟 수 (malloc_cnt != free_cnt이면 문제가 있음)
}

//#pragma mark -
//#pragma mark [SPY_CR] extlist.cfg를 읽어서 TABLE에 INSERT하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : alloc
 * CLASS-NAME     : SPY_CR
 * PARAMETER      : -
 * RET. VALUE     : NULL(ERR)/ptr
 * DESCRIPTION    : 새로은 CR을 할당하는 함수
 * REMARKS        :
 **end*******************************************************/
CR_DATA *SPY_CR::alloc(void)
{
    CR_DATA *pCR;

    if((pCR = (CR_DATA *)malloc(sizeof(CR_DATA))) == NULL)
    {
        ++ sum_malloc_err_cnt;
        ++ malloc_err_cnt;
        
        if(malloc_err_cnt > 5)
        {
            Log.printf(LOG_ERR, "[CR] malloc() fail[%d] ERROR count reach to MAX so shut down\n", malloc_err_cnt);
            shut_down(55);
        }
        return(NULL);
    }
    
    pthread_mutex_lock(&lock_mutex);
    malloc_err_cnt = 0;     // RESET - malloc() OK
    ++ sum_malloc_cnt;
    pthread_mutex_unlock(&lock_mutex);
    
    bzero(pCR, sizeof(CR_DATA));
    pCR->audit.t_alloc = time(NULL);
    
    // FIXIT: 할당 정보를 MAP에 등록하는 부분 추가
    return(pCR);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : free
 * CLASS-NAME     : SPY_CR
 * PARAMETER      : -
 * RET. VALUE     : NULL(ERR)/ptr
 * DESCRIPTION    : 새로은 CR을 할당하는 함수
 * REMARKS        :
 **end*******************************************************/
void SPY_CR::free(CR_DATA *pCR)
{
    // FIXIT: 할당 정보를 MAP에서 삭제하는 부분 추가
    
    if(pCR->audit.bFree == true) { Log.printf(LOG_ERR, "[CR] already free() \n"); return; }
    pthread_mutex_lock(&lock_mutex);
    sum_free_cnt ++;
    pCR->audit.bFree = true;
    pthread_mutex_unlock(&lock_mutex);
    ::free(pCR);
}

