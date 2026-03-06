#include "flcm_manager.h"
//#include "SPY_def.h"
//#include "SPY_xref.h"


FLCM_MANAGER::FLCM_MANAGER()
{
	pthread_mutex_init(&mtx, NULL);
}

FLCM_MANAGER::~FLCM_MANAGER()
{
	pthread_mutex_destroy(&mtx);
}

void FLCM_MANAGER::Fault_Init(BYTE ID, int fault_type, char *category, char *faultLog, int set_sec)
{

	MY_ID = ID;
	set_time = set_sec;

	g_FaultType = fault_type;

	if(g_FaultType == ASSIGN_MODE)
	{
		pthread_create(&t_timer, NULL, &FLCM_MANAGER::CheckTimer, (void *)this);
		pthread_detach(t_timer);
	}

	strncpy(Flcm_Send_Info.category, category, MAX_CATEGORY_SIZE);
	strncpy(Flcm_Send_Info.FlcmFaultLog, faultLog, MAX_FAULTLOG_SIZE);

	Fault_Clear();

}

void FLCM_MANAGER::Send_Fault_FLCM_RELAY(char* data)
{
	if(g_FaultType != RELAY_MODE)
	{
		return;
	}

	FAULT_MSG	flcm_msg;
	bzero(&flcm_msg, sizeof(flcm_msg));

	flcm_msg.header.Len	= sizeof(flcm_msg);
	flcm_msg.header.MsgID  = MSG_ID_FAULTLOG;
	flcm_msg.header.From   = MY_ID;
	flcm_msg.header.Time   = (int)time(NULL);
	flcm_msg.header.To     = MGM;
	flcm_msg.header.Type   = MMC_TYPE_END;

	strncpy(flcm_msg.category, 	Flcm_Send_Info.category, 	MAX_CATEGORY_SIZE - 1);
	strncpy(flcm_msg.fault_log, data, 						MAX_FAULTLOG_SIZE - 1);

	msendsig(sizeof(flcm_msg), MGM, MSG_ID_FAULTLOG, (uint8_t *)&flcm_msg);

}

void FLCM_MANAGER::Send_Fault_FLCM_RELAY()
{   
	if(g_FaultType != RELAY_MODE)
    {
        return;
    }

	FAULT_MSG   flcm_msg;
    bzero(&flcm_msg, sizeof(flcm_msg));
    
    flcm_msg.header.Len = sizeof(flcm_msg);
    flcm_msg.header.MsgID  = MSG_ID_FAULTLOG;
    flcm_msg.header.From   = MY_ID;
    flcm_msg.header.Time   = (int)time(NULL);
    flcm_msg.header.To     = MGM;
    flcm_msg.header.Type   = MMC_TYPE_END;
    
	strncpy(flcm_msg.category,  Flcm_Send_Info.category,        MAX_CATEGORY_SIZE - 1);
    strncpy(flcm_msg.fault_log, Flcm_Send_Info.FlcmFaultLog,    MAX_FAULTLOG_SIZE - 1);

    msendsig(sizeof(flcm_msg), MGM, MSG_ID_FAULTLOG, (uint8_t *)&flcm_msg);
    
}

void FLCM_MANAGER::Send_Fault(char* key, int value)
{
	char temp_msg[1024];
	int  ret;

	FAULT_MSG   flcm_msg;
    bzero(&flcm_msg, sizeof(flcm_msg));
	memset(temp_msg, 0x00, sizeof(temp_msg));

    flcm_msg.header.Len = sizeof(flcm_msg);
    flcm_msg.header.MsgID  = MSG_ID_FAULTLOG;
    flcm_msg.header.From   = MY_ID;
    flcm_msg.header.Time   = (int)time(NULL);
    flcm_msg.header.To     = MGM;
    flcm_msg.header.Type   = MMC_TYPE_END;

	strncpy(flcm_msg.category,  Flcm_Send_Info.category,        MAX_CATEGORY_SIZE - 1);

	ret = output_flcm_msg(key, value, temp_msg);
	if (ret < 0) // CATEGORY SEARCH FAIL
	{
		strncpy(flcm_msg.fault_log, Flcm_Send_Info.FlcmFaultLog,    MAX_FAULTLOG_SIZE - 1);
	    sprintf(temp_msg, "[%s][%d]", key,value);
    	strcat(flcm_msg.fault_log, temp_msg);

	}
	else if( ret >= 0) // CATEGORY SEARCH SUCC
	{
		strncpy(flcm_msg.fault_log, temp_msg,  MAX_FAULTLOG_SIZE - 1);
	}

	msendsig(sizeof(flcm_msg), MGM, MSG_ID_FAULTLOG, (uint8_t *)&flcm_msg);

}

bool FLCM_MANAGER::ADD_Fault_FLCM(char* key, int value)
{
	char key_save[1024];
	map<string, int>::iterator it;

	strcpy(key_save, key);
	string str(key);
	
	pthread_mutex_lock(&mtx);

	it = Fault_STS.find(str);

	if(it != Fault_STS.end())
	{
		Fault_STS[str] += value;
	}
	else
	{
		Fault_STS.insert(make_pair(str, value));
	}

	pthread_mutex_unlock(&mtx);

	return true;
}

void* FLCM_MANAGER::CheckTimer(void* aThis)
{
	FLCM_MANAGER *pThis = (FLCM_MANAGER *)aThis;
	map<string, int>::iterator iter;

	time_t current_time;
	char temp[1024]; 
	int	sec;

	sec = pThis->set_time;

	if(sec <= 0) sec = 5 * 60; //5MIN

	while(1)
	{
		current_time = time(NULL);

		if(current_time % sec == 0)	
		{
			pthread_mutex_lock(&(pThis->mtx));

			for(iter = pThis->Fault_STS.begin(); iter != pThis->Fault_STS.end(); iter++)
			{
				memset(temp, 0x00, sizeof(temp));
				pThis->string_to_char(iter->first, temp);
				pThis->Send_Fault(temp, iter->second);
			}

			pthread_mutex_unlock(&(pThis->mtx));
			pThis->Fault_Clear();
		}

		sleep(1);
	}
}

void FLCM_MANAGER::Fault_Clear()
{
	pthread_mutex_lock(&mtx);

	Fault_STS.clear();

	pthread_mutex_unlock(&mtx);
}

void FLCM_MANAGER::string_to_char(string string_msg, char* char_msg)
{
	const char* charArray = string_msg.c_str();

	strcpy(char_msg, charArray);
}

int FLCM_MANAGER::output_flcm_msg(char* key, int value, char* flcm_msg)
{
	if(strcmp(Flcm_Send_Info.category, "INVALID MSG")==0)
	{
		sprintf(flcm_msg, M_INVALID_MSG, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "NOTIFY FAIL")==0)
	{
		sprintf(flcm_msg, M_NOTIFY_FAIL, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "ACK TIMEOUT")==0)
    {
		sprintf(flcm_msg, M_ACK_TIMEOUT, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "RSP TIMEOUT")==0)
    {
		sprintf(flcm_msg, M_RSP_TIMEOUT, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "BAD SYNTAX")==0)
    {
		sprintf(flcm_msg, M_BAD_SYNTAX, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "SCENARIO_UPLOAD_FAIL")==0)
    {
		sprintf(flcm_msg, M_SCENARIO_UPLOAD_FAIL);
	}
	else if (strcmp(Flcm_Send_Info.category, "SCENARIO_LOOP_FAIL")==0)
    {
		sprintf(flcm_msg, M_SCENARIO_LOOP_FAIL);
	}
	else if (strcmp(Flcm_Send_Info.category, "INGW")==0)
    {
		sprintf(flcm_msg, M_INGW_FAIL, key, value);
	}
	else if (strcmp(Flcm_Send_Info.category, "CDR FILE")==0)
    {
		sprintf(flcm_msg, M_CDR_FILE_FAIL);
	}
	else if (strcmp(Flcm_Send_Info.category, "CDR AVP")==0)
    {
		sprintf(flcm_msg, M_CDR_AVP_FAIL, key);
	}
	else if (strcmp(Flcm_Send_Info.category, "OFCS")==0)
    {
		sprintf(flcm_msg, M_OFCS_FAIL, key, value);
	}
	else
	{
		return -1;
	}

	return 1;
}
