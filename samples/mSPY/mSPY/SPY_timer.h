
#ifndef __SPY_TIMER_H__
#define	__SPY_TIMER_H__

#include <map>
using namespace std;

typedef int (*TO_FUNC)(size_t, size_t);

#define TIME_MSEC        0
#define TIME_SEC         1
#define TIME_MIN         2
#define TIME_HOUR        3
#define TIME_DAY         4

#define RET_TIME_UNIT_ERR       -1
#define RET_TIME_TOO_SMALL      -2
#define RET_MEM_ERR             -3
#define RET_TIMER_TABLE_FULL	-4

typedef struct
{
	int64_t id;
	struct  timeval  to;
	int		unit;
	TO_FUNC	func;
	size_t  arg1;
	size_t  arg2;
} TMRTBL;

class TIMER
{
    friend void *timer_thread(void *arg);

private:
    int64_t m_make_timer_id;
    int     m_reg_timer_cnt;
    bool    b_run_flag;
    int     m_err_unit, m_err_tsmall, m_err_mem, m_err_full;
    pthread_t           thread_id;
    pthread_mutex_t     m_timer_mutex;
    map<int64_t, TMRTBL *>  m_timer_map;

    void    check_time_out(void);
    
public:
	TIMER();
	~TIMER();
	void    *start(void *arg);
	int64_t set(int time, int unit, TO_FUNC func, size_t arg1, size_t arg2);
	int     cancel(int64_t tid);
	int     check(int64_t tid);
    int     get_reg_timer_cnt(void);
    void    get_timer_err_cnt(int *unit_err, int *tsmall_err, int *mem_err, int *tbl_full);
    size_t  size();
};


#endif // __SPY_TIMER_H__
