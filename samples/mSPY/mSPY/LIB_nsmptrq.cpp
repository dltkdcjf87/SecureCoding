/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : LIB_smptrq.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : LIB
 SUBSYSTEM      : LIB
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/03/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : QUEUE Class 함수 들
 REMARKS        : 데이터를 실제 PUSH/POP하지 않고 pounter만 PUSH/POP을 한다.
                : (메모리 copy를 줄여서 속도를 높이기 위해)
                : 실제 데이터는 NSM_PTR_Q를 사용하는 소스에서 알아서 malloc하고 free해야 한다.
 *end*************************************************************/

#include "libnsmptrq.h"

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : NSM_PTR_Q
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : constructor
 * REMARKS        :
 **end*******************************************************/
NSM_PTR_Q::NSM_PTR_Q()
{
    pthread_mutex_init(&q_mutex, NULL);
    
	header  = NULL;
	tail    = NULL;
	counter = 0;
    
    malloc_cnt = 0;
    free_cnt   = 0;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~NSM_PTR_Q
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : destructor
 * REMARKS        :
 **end*******************************************************/
NSM_PTR_Q::~NSM_PTR_Q()
{
	Q_PTR	*tmp, *imsi;
    
	tmp = header;
    
	for(; tmp; )
	{
        imsi = tmp->next;
		delete tmp;
		tmp = imsi;
	}
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_empty
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    :
 * REMARKS        : lock
 **end*******************************************************/
bool NSM_PTR_Q::is_empty()
{
    pthread_mutex_lock(&q_mutex);
    {
        if(counter == 0)         { header = NULL; pthread_mutex_unlock(&q_mutex);return(true); }
        if(header == NULL)       {                pthread_mutex_unlock(&q_mutex);return(true); }    // empty
        if(header->data == NULL) { header = NULL; pthread_mutex_unlock(&q_mutex); return(true); }   // empty
    }
    pthread_mutex_unlock(&q_mutex);
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : q_is_empty
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    :
 * REMARKS        : lock을 걸지 않는 버전
 **end*******************************************************/
bool NSM_PTR_Q::q_is_empty()
{

    if(counter == 0)         { header = NULL; return(true); }
	if(header == NULL)       { return(true); }                  // empty
	if(header->data == NULL) { header = NULL; return(true); }   // empty
    
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : LibQ
 * PARAMETER      : -
 * RET. VALUE     : int
 * DESCRIPTION    : number of items in queue
 * REMARKS        :
 **end*******************************************************/
int NSM_PTR_Q::size()
{
    return(counter);
}

/* Procedure Header
 *pdh********************************************************
 * PROCEDURE-NAME : NSM_PTR_Q::enqueue
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER    IN: data - Q에 저장할 데이터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : push data to queue
 * REMARKS        : data를 실제 저장하지 않고 pointer만 저장
 **end*******************************************************/
bool NSM_PTR_Q::enqueue(void *data)
{
    //	if(len > MAX_QUEUE_DATA_LEN) { return(false); }
    
	pthread_mutex_lock(&q_mutex);
    {
        if(q_is_empty())
        {
			// QUEUE에 넣을 데이터의 메모리를 할당
            if((header = (Q_PTR *)malloc(sizeof(Q_PTR))) == NULL)
            {
                pthread_mutex_unlock(&q_mutex);
                return(false);
            }
            malloc_cnt ++;          // malloc한 횟 수 (for debugging)
            header->next = 0;
            tail   = header;
        }
        else
        {
            if((tail->next = (Q_PTR *)malloc(sizeof(Q_PTR))) == NULL)
            {
                pthread_mutex_unlock(&q_mutex);
                return(false);
            }
            malloc_cnt ++;          // malloc한 횟 수 (for debugging)
            tail = tail->next;
            tail->next = 0;
        }
        
        tail->data = data;  // 실제 데이터는 copy하지 않고 pointer만 저장
        
        counter ++;
    }
	pthread_mutex_unlock(&q_mutex);
    
	return(true);
}

/* Procedure Header
 *pdh********************************************************
 * PROCEDURE-NAME : NSM_PTR_Q::dequeue
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER   OUT: data - Q에서 꺼낸 데이터의 ptr'ptr (call by ref.)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : pop data from queue
 * REMARKS        :
 **end*******************************************************/
bool NSM_PTR_Q::dequeue(void **data)
{
	Q_PTR	*tmp;
    
	pthread_mutex_lock(&q_mutex);
    {
        if(q_is_empty()) { pthread_mutex_unlock(&q_mutex); return(false); }
 
        *data = header->data;
        
        tmp = header;
        header = header->next;
        free(tmp);
        free_cnt ++;          // free()한 횟 수 (for debugging)
        counter --;
    }
	pthread_mutex_unlock(&q_mutex);
    
	return(true);
}

/* Procedure Header
 *pdh********************************************************
 * PROCEDURE-NAME : NSM_PTR_Q::malloc_size
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : N(malloc_cnt)
 * DESCRIPTION    : NSM_PTR_Q에서 malloc한 횟 수를 리턴하는 함수
 * REMARKS        : 디버깅 또는 Audit 용
 **end*******************************************************/
uint32_t NSM_PTR_Q::malloc_size(void)
{
    return(malloc_cnt);
}

/* Procedure Header
 *pdh********************************************************
 * PROCEDURE-NAME : NSM_PTR_Q::free_size
 * CLASS-NAME     : NSM_PTR_Q
 * PARAMETER      : -
 * RET. VALUE     : N(free_cnt)
 * DESCRIPTION    : NSM_PTR_Q에서 free한 횟 수를 리턴하는 함수
 * REMARKS        : 디버깅 또는 Audit 용
 **end*******************************************************/
uint32_t NSM_PTR_Q::free_size(void)
{
    return(free_cnt);
}

