// QueueManager.cpp: implementation of the CQueueManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "QueueManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQueueManager& theQueueMgr(void)
{
	static CQueueManager theQueue;

	return theQueue;
}

CQueueManager::CQueueManager()
{

}

CQueueManager::~CQueueManager()
{

}

void CQueueManager::initialize(void)
{
	m_messageQueue.initialize();
}

void CQueueManager::uninitialize(void)
{
	m_messageQueue.uninitialize();
}

