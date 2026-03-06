#include	<stdio.h>
#include	<stdint.h>
#include    <iostream>
#include    <pthread.h>
#include    <map>
#include    <string.h>
#include	<unistd.h>

#include    <btxbus3.h>


#include "MMC_head.h"

typedef unsigned char   BYTE;
using namespace std;

#define ASSIGN_MODE	0
#define	RELAY_MODE	1

//FLCM MESSAGG TELPLETE DEFINE
#define M_INVALID_MSG		"CSCF IP %s, Invalid SIP 메세지 %d건 수신"
#define M_NOTIFY_FAIL		"%s Notify fail %d건 발생"
#define M_ACK_TIMEOUT		"공통 DB %s ACK 미수신 %d건 발생"
#define	M_RSP_TIMEOUT		"공통 DB %s RESPONSE 미수신%d건 발생"
#define	M_BAD_SYNTAX		"CSCF %s BAD SYNTAX 수신 %d건 발생"
#define	M_SCENARIO_UPLOAD_FAIL	"SCENARIO FILE UPLOAD FAIL"
#define	M_SCENARIO_LOOP_FAIL	"SCENAIO LOOP FAIL"
#define	M_INGW_FAIL		"INGW 세션 %s 실패응답 및 DISCONNECT %d건 발생"
#define	M_ISMC_FAIL		"ISMC 세션 %s 실패응답 및 DISCONNECT %d건 발생"
#define	M_FNPS_FAIL		"FNPS 세션 %s 실패응답 및 DISCONNECT %d건 발생"
#define	M_CDR_FILE_FAIL		"CDR-DATA DISK WRITE FAIL"
#define	M_CDR_AVP_FAIL		"INVALID AVP %s"
#define M_OFCS_FAIL		"OFCS %s 실패 응답 %d건 발생"



typedef struct
{
	char category[MAX_CATEGORY_SIZE];
    char FlcmFaultLog[MAX_FAULTLOG_SIZE];
} FLCM_DATA;


class FLCM_MANAGER
{
	private:
		static void *CheckTimer(void* aThis);
		void Fault_Clear();
		void Send_Fault(char* key, int value);
		void string_to_char(string string_msg, char* char_msg);
		int output_flcm_msg(char* key, int value, char* flcm_msg);

		FLCM_DATA Flcm_Send_Info;
		int	g_FaultType;
	
		pthread_t		t_timer;
		pthread_mutex_t	mtx;
		map<string, int> Fault_STS;
		BYTE			MY_ID;

		int		set_time;

	public:
		FLCM_MANAGER();
		~FLCM_MANAGER();

		void Fault_Init(BYTE ID, int fault_type, char *category, char *faultLog, int set_sec);
		void Send_Fault_FLCM_RELAY(char* data);
		void Send_Fault_FLCM_RELAY();
		bool ADD_Fault_FLCM(char* key, int value);

};
