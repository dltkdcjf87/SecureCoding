// QueueManager.h: interface for the CQueueManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QUEUEMANAGER_H__6EDF4FE7_6CC3_4404_A8B2_BA57C9C59D96__INCLUDED_)
#define AFX_QUEUEMANAGER_H__6EDF4FE7_6CC3_4404_A8B2_BA57C9C59D96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MessageQueue.h"

class CQueueManager  
{
private:
	CMsgQueue m_messageQueue;

public:
	CQueueManager();
	virtual ~CQueueManager();

	void initialize(void);
	void uninitialize(void);

	inline bool getMsg(CMsgQueue::_MSG_& msg) { return m_messageQueue.getMsg(msg); };
	inline void putMsg(long id, char* pData, int nLength, long nParam) { 
		m_messageQueue.addMsg(id, pData, nLength, nParam); 
	};

	friend CQueueManager& theQueueMgr(void);
};

extern CQueueManager& theQueueMgr(void);

#endif // !defined(AFX_QUEUEMANAGER_H__6EDF4FE7_6CC3_4404_A8B2_BA57C9C59D96__INCLUDED_)
