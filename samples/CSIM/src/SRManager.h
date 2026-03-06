// SRManager.h: interface for the CSRManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SRMANAGER_H__AB8F3419_1637_4F15_987B_3FE17E1EE64A__INCLUDED_)
#define AFX_SRMANAGER_H__AB8F3419_1637_4F15_987B_3FE17E1EE64A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/* HA status */
#define HA_UNDEFINED        0x00
#define HA_ACTIVE           0x01
#define HA_STANDBY          0x02

class CSRManager  
{
protected:
	THREAD_HANDLE m_hThread;

	int m_nHAStatus;

protected:
	
	static void ha_CallBackUnAssign(int nParam);
	static void ha_CallBackActive(int nParam);
	static void ha_CallBackStandby(int nParam);
	static void ha_CallBackAddStandby(int nParam);
	static void ha_CallBackRemoveStandby(int nParam);

	static void ha_CallBackModuleStateChange(unsigned char mod_code, int nAlive);
	
public:
	CSRManager();
	virtual ~CSRManager();

	bool initialize(void);
	void uninitialize(void);

	inline void setStatus(int nStatus) { m_nHAStatus = nStatus; };
	inline int getStatus(void) { return m_nHAStatus; };
	inline bool isActive(void) { return (m_nHAStatus == HA_ACTIVE); };
	void changeStatus(int nStatus);

	friend CSRManager& theSRManager(void);
};

CSRManager& theSRManager(void);

typedef CSRManager* CSRManagerPtr;

#endif // !defined(AFX_SRMANAGER_H__AB8F3419_1637_4F15_987B_3FE17E1EE64A__INCLUDED_)

