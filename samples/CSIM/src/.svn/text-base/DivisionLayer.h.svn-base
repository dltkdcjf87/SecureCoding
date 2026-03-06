// SessionLayer.h: interface for the CSessionLayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIVISIONLAYER_H__E8FC2CFE_8BA5_4D06_81CE_68CF63DA2FA8__INCLUDED_)
#define AFX_DIVISIONLAYER_H__E8FC2CFE_8BA5_4D06_81CE_68CF63DA2FA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDivisionLayer  
{
protected:
	bool m_bInit;

	typedef int(*DivisionCallBack)(int nLength, unsigned short int nID, unsigned char nFrom, unsigned char* pData);
	typedef std::map<unsigned short int, DivisionCallBack> CALLBACK_MAP;

	CALLBACK_MAP m_mapCallBack;

	bool findCallBack(unsigned short int nID, DivisionCallBack& pFunc);

public:
	static int eventCallBack_MBus(int nLength, u_pchar pData);
	static int eventCallBack_XBus(int nLength, u_pchar pData);

	static unsigned char isActiveModule(unsigned short proc_id, unsigned char self_id);
	static int send(unsigned short length, unsigned char to_id, unsigned short message_id, unsigned char* pMessage);
	static int ssend(unsigned short length, unsigned short message_id, unsigned char* pMessage);

public:
	CDivisionLayer();
	virtual ~CDivisionLayer();

	bool initialize(void);
	void uninitialize(void);

	void setCallBack(unsigned short int nID, DivisionCallBack pFunc);

	friend CDivisionLayer& theDivisionLayer(void);
};

CDivisionLayer& theDivisionLayer(void);

#endif // !defined(AFX_DIVISIONLAYER_H__E8FC2CFE_8BA5_4D06_81CE_68CF63DA2FA8__INCLUDED_)


/*
				ÀÀ¿ëÃþ(Application  Layer,  Á¦7Ãþ)
SAM,SRM,MMC		Ç¥ÇöÃþ(Presentation  Layer,  Á¦6Ãþ)
DIVISION		¼¼¼ÇÃþ(Session  Layer,  Á¦5Ãþ)
XBUS			Àü¼ÛÃþ(Transport  Layer,  Á¦4Ãþ)
				³×Æ®¿öÅ©Ãþ(Network  Layer,  Á¦3Ãþ)
				µ¥ÀÌÅÍ¸µÅ©Ãþ(Datalink  Layer,  Á¦2Ãþ)
				¹°¸®Ãþ(Physical  Layer,  Á¦1Ãþ)
*/
