#pragma once

int svcStat_Insert(void);
int  userStatInit(void);
int statInsert(char *sStatName, int count);

#define IN1588_FILE "_office_stat, _cscf_stat, _dest_stat, _subscriber_stat, _rerouting_stat, _consult_stat, _conference_stat, _media_server_stat, _premium_stat" 
#define IN1577_FILE "_droffice_stat, _drcscf_stat, _drdest_stat, _drsubscriber_stat, _drrerouting_stat, _drconsult_stat, _drconference_stat, _drmedia_server_stat, _drpremium_stat" 
#define IN1541_FILE "_area_stat _ms_stat _mscmd_stat _ssw_stat _traffic_stat" 
#define INFP_FILE "_fpoffice_stat, _fpcscf_stat, _fpdest_stat, _fpsubscriber_stat, _fprerouting_stat, _fpconsult_stat, _fpconference_stat, _fpmedia_server_stat, _fppremium_stat" 
#define INTI_FILE "_tioffice_stat, _ticscf_stat, _tidest_stat, _tisubscriber_stat, _tirerouting_stat"
#define IN1588_TABLE_NUM 76



#define MAX_SVCKEY 100

//stat define
#ifdef _EXT_STAT_ADD
#define STAT_MAXIMUM 120 //2019.3.26 14yy EXT STAT ADD
#else
#define STAT_MAXIMUM 76
#endif

//stat 갯수  헤더를 제외한
#define CSCF_NUM 61
#define OFFICE_NUM 52
#define SUBSCRIBER_NUM 72
#define DEST_NBS_NUM 72
#define MEDIA_SERVER_NUM 8
#define REROUTING_NUM 30
#define CONSULT_NUM 31
#define CONFERENCE_NUM 30
#define PREMIUM_NUM 28 
#define CREATE_NUM 27
#define RESULT_NUM 27


//1541 헤더를 제외한 갯수 (시나리오에서 입력하는 통계 갯수)
#define SSW_NUM  45
#define AREA_NUM 11
#define MS_NUM   10
#define MSCMD_NUM 10
#define TRAFFIC_NUM 43
#define MILITARY_NUM 42
#define SMS_NUM 3
//1541 파일을 parsing 하여 DB에 들어가는 갯수 ('테이블 컬럼 수'-'key값')
#define SSW_DB 			51
#define MILITARY_DB 	45
#define AREA_DB 		14
#define MS_DB         	 3
#define MSCMD_DB		 5
#define TRAFFIC_DB		19
#define SMS_DB		    3


//CI 통계 갯수
#define CITOTAL_NUM		61
#define CIMS_NUM		3
#define CIMSCMD_NUM		5


//////////////////////////////////////////////////
//패치 적용시 아래 STRING 꼭 수정 1899 삭제
///////////////////////////////////////////////////
//stat Table name
#define CSCF_STAT 			"STAT_CSCF"
#define OFFICE_STAT 		"STAT_OFFICE"
#define SUBSCRIBER_STAT 	"STAT_SUBSCRIBER"
#define DEST_NBS_STAT 		"STAT_DEST_NBS"
#define MEDIA_SERVER_STAT 	"STAT_MEDIA_SERVER"
#define REROUTING_STAT 		"STAT_REROUTING"
#define CONSULT_STAT 		"STAT_CONSULT"
#define CONFERENCE_STAT 	"STAT_CONFERENCE"
#define PREMIUM_STAT		"STAT_PREMIUM"
//stat Table name
#define CSCF_STAT_1577 			"STAT_CSCF_1577"
#define OFFICE_STAT_1577 		"STAT_OFFICE_1577"
#define SUBSCRIBER_STAT_1577 	"STAT_SUBSCRIBER_1577"
#define DEST_NBS_STAT_1577 		"STAT_DEST_NBS_1577"
#define MEDIA_SERVER_STAT_1577 	"STAT_MEDIA_SERVER_1577"
#define REROUTING_STAT_1577 	"STAT_REROUTING_1577"
#define CONSULT_STAT_1577 		"STAT_CONSULT_1577"
#define CONFERENCE_STAT_1577 	"STAT_CONFERENCE_1577"
#define PREMIUM_STAT_1577		"STAT_PREMIUM_1577"
//stat TABLE 1541 name
#define AREA_STAT    "BCNACCST_AREAMCNTS"
#define SSW_STAT     "BCNACCST_SSWNCNTS"
#define MS_STAT	     "BCNACCST_MS_CALLCNTS"
#define MSCMD_STAT   "BCNACCST_MS_MESSAGECNTS"
#define TRAFFIC_STAT "BCNACCST_OFFICECNTS"
#define MILITARY_STAT     "BCNACCST_MILITARY"  
#define SMS_STAT     "BCNACCST_SMSCNTS"  
//stat TABLE 080 
#define CSCF_STAT_080                  "STAT_FPCSCF"
#define OFFICE_STAT_080                "STAT_FPOFFICE"                                        
#define SUBSCRIBER_STAT_080    "STAT_FPSUBSCRIBER"
#define DEST_NBS_STAT_080           "STAT_FPDEST_NBS"                                      
#define MEDIA_SERVER_STAT_080  "STAT_FPMEDIA_SERVER"                                          
#define REROUTING_STAT_080     "STAT_FPREROUTING"
#define CONSULT_STAT_080             "STAT_FPCONSULT"                                       
#define CONFERENCE_STAT_080    "STAT_FPCONFERENCE"
#define PREMIUM_STAT_080               "STAT_FPPREMIUM" 


//stat TABLE 060 
#define CSCF_STAT_060                  "STAT_TICSCF"
#define OFFICE_STAT_060                "STAT_TIOFFICE"                                        
#define SUBSCRIBER_STAT_060    "STAT_TISUBSCRIBER"
#define DEST_NBS_STAT_060           "STAT_TIDEST_NBS"                                      
#define REROUTING_STAT_060     "STAT_TIREROUTING"
//pv,pb,cris
#define CSCF_STAT_PB                 "STAT_PBCSCF"
#define MEDIA_SERVER_STAT_PB         "STAT_PB_MEDIA_SERVER"

#define CSCF_STAT_PV                 "STAT_PVCSCF"
#define SUBSCRIBER_STAT_PV           "STAT_PVSUBSCRIBER"
#define MEDIA_SERVER_STAT_PV         "STAT_PV_MEDIA_SERVER"
#define CREATE_STAT_PV				 "STAT_PVCREATE"
#define RESULT_STAT_PV				 "STAT_PVRESULT_CHECK"

#define CSCF_STAT_CRIS               "STAT_CRISCSCF"
#define OFFICE_STAT_CRIS             "STAT_CRISOFFICE"
#define MEDIA_SERVER_STAT_CRIS       "STAT_CRIS_MEDIA_SERVER"


#define TOTAL_STAT_CI				"STAT_CICFP_TOTAL"
#define MS_STAT_CI					"STAT_CICFP_MEDIA_SERVER"
#define MSCMD_STAT_CI				"STAT_CICFP_MS_CMD"


//file name 1 = dr ,  0 = include not dr
#define IN1577_STAT 1
#define IN1588_STAT 0
#define IN1899_STAT 0 
#define INFP_STAT 2
#define INTI_STAT 3 
#define INPB_STAT 4
#define INPV_STAT 5
#define INCRIS_STAT 6
#define INCI_STAT 7
#define SERVICE_STAT_FILE 1
//error cod
#define NOT_SERVICE_STAT_FILE 0



#define COLLECTCALL_NUM 1541
#define REPRESENT_NUM 1899 //1899, 1541

//AS NUM
#define AS15 15
#define AS25 25



//AREA ETC

#define AREA_ETC_NUM 22
typedef struct stat_data
{
	char RESERV_KEY1[64];
	char RESERV_KEY2[64];
	char STAT_NAME[30];
	int NUM_OF_STAT;
	int ref;
	int param_num_check;
	int service_id;
	int AREA_ETC;	
}STAT_DATA;





typedef struct svc_data
{
	char file_name[30];
	int  stat_num;
	char TABLE_NAME[30];
	int  header_count;
}STAT_ARRAY;













//추후 업데이트
//#define IN1577_FILE "_office"
//#define IN1899_FILE "xxx"


















