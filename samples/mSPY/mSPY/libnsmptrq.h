//
//  LIB_smptrq.h
//  smSPY
//
//  Created by SMCHO on 2014. 4. 29..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __LIB_NSMPTRQ_H__
#define __LIB_NSMPTRQ_H__

//#include "SPY_cr.h"
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef struct _Q_PTR
{
	struct _Q_PTR	*next;
	void     *data;
} Q_PTR;


/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : NSM_PTR_Q
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : Pointer Queue Class
 *                :
 * REMARKS        : 실제 데이터는 copy하지 않고 pointer만 Queuing 함.
 *                : 실제 데이터에 대한 malloc/free는 Class 외부에서 별도로 처리 해야 함
 **end*******************************************************/
class NSM_PTR_Q
{
private:
	Q_PTR	*header, *tail;
	int		counter;
    pthread_mutex_t q_mutex;
    uint32_t    malloc_cnt;             // Queue에서 malloc한 횟 수 (for Debugging)
    uint32_t    free_cnt;               // Queue에서 free한 횟 수 (for Debugging)
    bool        q_is_empty(void);
    
public:
	NSM_PTR_Q();
	~NSM_PTR_Q();
    
	bool    is_empty(void);
    int     size(void);
	bool    enqueue(void *packet);
	bool    dequeue(void **packet);
    uint32_t    malloc_size(void);
    uint32_t    free_size(void);
};


#endif /* defined(__LIB_NSMPTRQ_H__) */
