
#include <time.h>
#include <sys/stat.h>

#include "Environment.h"

#ifdef OCCI
#include "OCCIExt.h"
#endif
#ifdef ALTI_V6
#include "AltiCinf.h"
#endif

#include "Logger.h"
#include "Utility.h"
#include "Global.h"
#include "UserStat.h"

//#define UserStatNewPath "/backup/service/usr_stat_new"
//#define UserStatDonePath "/backup/service/usr_stat_done"

#ifdef OCCI
COCCIExt    m_occi;
#endif

#ifdef ALTI_V6
AltiCinf  m_atci;
#endif

unsigned long thrUserStat(void *param);
int  userStatMain(void);
int  getUserStatFile( char *sFileName );
int  regUserStatFile( char *sStatFileName );
int  insUserStatToDB( char *sData, char *sCatName );
bool getItem(char* & pSorce, char* pBuff, char cSplit);
int moveComplete( const char *sFileName );
int moveError( const char *sFileName );

// LOGGER(TRACE_LEVEL2, "service daemon started. (port:%d)", nPort);

char UserStatNewPath[128] = {0,};
char UserStatDonePath[128] = {0,};
char UserStatErrPath[128] = {0,};
char SubKeyStatList[1024] = {0,};
//hwang

char UserSvcKeyList[MAX_SVCKEY][64] = {0,}; //svckey list
void insUserStatTo1541(STAT_DATA *sf_stat_data, char* & orgin_data, char *file_name);
void InsertAreaSTS(char* &pDest, int *pSrc);
void InsertOfficeSTS(char* &pDest, int *pSrc);
void InsertMsSTS(char* &pDest, int *pSrc);
void InsertMscmdSTS(char* &pDest, int *pSrc);
void InsertSswSTS(char* &pDest, int *pSrc);    
void InsertSMSSTS(char* &pDest, int *pSrc);    
void InsertMiliSTS(char* &pDest, int *pSrc);
void Parsing_CollectCall_Num(char* &pDest, int* pSrc, STAT_DATA *sf_stat_data);

STAT_ARRAY g_stat_array[MAX_SVCKEY];
//전체 통계 갯수 카운트
int stat_count = 0;

int stat_temp_count =0;
int userStatInit(void)
{
	char szUserID[32];
	char szPasswd[32];
	char szConnStr[32];
	GetPrivateProfileStringv2("USERSTAT", "SUBKEYSTAT", "", SubKeyStatList, sizeof(SubKeyStatList), theGlobal().getCSIMConfigFile()); 

	GetPrivateProfileStringv2("USERSTAT", "NEW_DIR", "", UserStatNewPath, sizeof(UserStatNewPath), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("USERSTAT", "DONE_DIR", "", UserStatDonePath, sizeof(UserStatDonePath), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("USERSTAT", "ERR_DIR", "", UserStatErrPath, sizeof(UserStatErrPath), theGlobal().getCSIMConfigFile()); 

	GetPrivateProfileStringv2("CSIM", "ORACLE_USERID", "sysdba", szUserID, sizeof(szUserID), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("CSIM", "ORACLE_PASSWD", "oracle", szPasswd, sizeof(szPasswd), theGlobal().getCSIMConfigFile()); 
	GetPrivateProfileStringv2("CSIM", "ORACLE_CONNSTR", "", szConnStr, sizeof(szConnStr), theGlobal().getCSIMConfigFile()); 

//hwang
	 memset(g_stat_array, 0x00, sizeof(g_stat_array));
	
//	 LOGGER(TRACE_INFO, " 11111"); 	
	svcStat_Insert();
//hwang
#ifdef OCCI
	m_occi.initialize(szUserID, szPasswd, szConnStr);
#endif
#ifdef ALTI_V6
    char    ip[128];
    GetPrivateProfileStringv2("CSIM", "LOCAL_IP", "", ip, sizeof(ip), theGlobal().getCSIMConfigFile());
	m_atci.initialize(szUserID, szPasswd, ip);
#endif

	u_long ulThreadID = 0;
	THREAD_HANDLE m_hThread;

	_CreateThread(m_hThread, NULL, 0, (LPTHREAD_START_ROUTINE) thrUserStat, 0x00, 0, &ulThreadID);
}

///////////////////////////////////////////////////////////
//hwang
int svcStat_Insert()
{
//////
// svckey_count = 등록할 svc 갯수

	char *pPos;
	int i = 0;
	int n = 0;
    int svckey_count = 0;
	char sCatList[8000];

	memset(sCatList, 0x00, sizeof(sCatList));

	GetPrivateProfileStringv2("SVC", "SVCSTAT", "", sCatList, sizeof(sCatList), theGlobal().getCSIMConfigFile());
	pPos = sCatList;		
    LOGGER(TRACE_INFO, "svc list:[%s] \n",sCatList);
	while(*pPos)
	{
		if(*pPos == ',' ) { i++; n = 0; pPos++; svckey_count++; continue; } // , 만날때마다 i 배열 증가 svckey_count 증가, n 배열에 string insert
		if(*pPos == ' ' ) { pPos++; continue; }
		UserSvcKeyList[i][n++] = *pPos++;
		
	}
	for(i = 0; i < svckey_count+1; i ++)
	{
		if(UserSvcKeyList[0][0] == 0x00) continue;
	 	statInsert(UserSvcKeyList[i], i);			
	}	

}

int statInsert(char *sStatName, int count)
{
	char *pPos;
	char sCatList[2048];
    char filename[30];
	int i, n;
    int array_count = 0;
#ifdef _EXT_STAT_ADD
	char stat_num[4]; // 2019.03.26 MIN : 14YY EXT STAT ADD 
#else
	char stat_num[3];
#endif
	char header_num[3];
	char table_name[30];
 
	memset(stat_num, 0x00, sizeof(stat_num));
	memset(header_num, 0x00, sizeof(header_num));
	GetPrivateProfileStringv2("STATINFO", sStatName, "", sCatList, sizeof(sCatList), theGlobal().getCSIMConfigFile());
     pPos = sCatList;
    i = 0; n = 0;
	while(*pPos)
	{
		if(*pPos == ',') { pPos++; array_count++; i=0; continue;}
		if(array_count == 0)
		{
			g_stat_array[count].file_name[i++] = *pPos++;  
		}
		if(array_count == 1)
		{
			 stat_num[i++] = *pPos++;
		}
		if(array_count == 2)
		{
			header_num[i++] = *pPos++;
		}
		if(array_count == 3)
		{
			g_stat_array[count].TABLE_NAME[i++] = *pPos++;
		}

	}
	
	g_stat_array[count].stat_num = atoi(stat_num);
	g_stat_array[count].header_count = atoi(header_num);

	stat_count++;

    LOGGER(TRACE_INFO, "filename:%s,stat_num:%s,header:%s,table_name:%s[%d] \n", g_stat_array[count].file_name, stat_num, header_num, g_stat_array[count].TABLE_NAME, count);
}


//hwang
unsigned long thrUserStat(void *param)
{
	int ret = 0;

	while( 1 )
	{
		ret = userStatMain();
		if(ret < 0)
		{
			if(ret == -2)LOGGER(TRACE_LEVEL1, "db connection error = %d ", ret );
		}
		Sleep(1000 * 10);
	}

	return 0;
}


int userStatMain(void)
{
	int  nCode;
	char sStatFileName[128];

	memset(sStatFileName, 0x00, sizeof(sStatFileName));
#ifdef OCCI
	if( m_occi.isConnect() == false ) {
#endif
#ifdef ALTI_V6
	if( m_atci.isConnect() == false ) {
#endif
		LOGGER(TRACE_LEVEL1, "statRegMain : dbDisconnected " );
		return -2;
	}
	while(1)
	{
		nCode = getUserStatFile( sStatFileName );	
		if( nCode < 0 ) return -1;
#ifdef OCCI
		if( m_occi.isConnect() == false ) 
#endif
#ifdef ALTI_V6
		if( m_atci.isConnect() == false )
#endif
		{
			LOGGER(TRACE_LEVEL1, "db connection error ");
			return -2;
		}
		regUserStatFile( sStatFileName );
	}
	return 0;
}


int getUserStatFile( char *sFileName )
{
	DIR *dp;
	struct dirent *dir_entry;
	char sTmpName[128];
	char sTmpFile[128];

	sTmpName[0] = sFileName[0] = 0x00;
	memset(sTmpFile, 0x00, sizeof(sTmpFile));
	memset(sTmpName, 0x00, sizeof(sTmpName));

	if((dp = opendir(UserStatNewPath)) == null) {
		return -2;
	}
	while(true) {
		if((dir_entry = readdir(dp)) == null) {
			closedir(dp);
			return -3;
		}

		if( strstr(dir_entry->d_name, ".sts") != 0x00 ) { 
			strcpy( sTmpName, dir_entry->d_name );
			break;
		}
		
	}
	closedir(dp);

	if( sTmpName[0] == 0x00 ) return -1;

	strcpy( sFileName, sTmpName );

	return 0;
}

int regUserStatFile( char *sStatFileName )
{
	char sFullName[512];
	struct stat iStat;
	int  nCode;
	FILE *fh;
	char sLine[1024];
	bool bFail;
	char sCatName[512];
	char *p;


	p = strstr( sStatFileName+1, "_stat" );
	if( p != 0x00 ) {
		memset( sCatName, 0x00, sizeof(sCatName) );
		strncpy( sCatName, sStatFileName, p - sStatFileName );
		strcat(sCatName, "_stat"); 
	} else sCatName[0] = 0x00;

	memset( sFullName, 0x00, sizeof(sFullName));
	
	sprintf( sFullName, "%s/%s", UserStatNewPath, sStatFileName );

	nCode = stat( sFullName, &iStat );
	if( nCode == -1 ) {
		return -1;
	}
	fh = fopen( sFullName, "rt" );
	if( fh == 0x00 ) {
		fclose(fh);
		return -1;
	}
	
	bFail = false;
	while( !feof( fh ) )
	{
		memset( sLine, 0x00, sizeof( sLine ) );
		fgets( sLine, sizeof( sLine ), fh );
		if( sLine[0] == 0x00 ) continue;
		nCode = insUserStatToDB( sLine, sCatName );
	
		if( nCode < 0 )	bFail = true;
		else
		{
			 //Lyan 130108 : when column value is null, difference lines for insert. 
#ifdef OCCI
			m_occi.commit();
#endif
#ifdef ALTI_V6
			m_atci.commit();
#endif
		}
	}
	fclose(fh);
	
	if( bFail == false ) {
	//Lyan 130108 : when column value is null, difference lines for insert.   
	//	m_occi.commit(); /
		moveComplete( sStatFileName );
	} else {
		moveError( sStatFileName );
	}
	sleep(1);
	return 0;
}

int moveComplete( const char *sFileName )
{
	char srcname[256], destname[256];
	char fullpath[256];
	char cmd[512];

	sprintf( srcname, "%s/%s", UserStatNewPath, sFileName );
	sprintf( destname, "%s/%s", UserStatDonePath, sFileName );

	sprintf(cmd, "mv -f %s %s", srcname, destname);
	system(cmd);


	return 0;
};

int moveError( const char *sFileName )
{
	char srcname[256], destname[256];
	char fullpath[256];
	char cmd[512];

	sprintf( srcname, "%s/%s", UserStatNewPath, sFileName );
	sprintf( destname, "%s/%s", UserStatErrPath, sFileName );

	sprintf(cmd, "mv -f %s %s", srcname, destname);
	system(cmd);


	return 0;
};




void InsertMscmdSTS(char* &pDest, int *pSrc)
{
	int i;
	  for( i = 0; i < MSCMD_DB; i++ )
        {
            pDest += sprintf( pDest, ",%d", pSrc[i] );
        }
}



void insUserStatToSt(STAT_DATA *sf_stat_data, char* & orgin_data, char *file_name)
{
	bool bNext;
	int s;
	memset(sf_stat_data, 0x00, sizeof(STAT_DATA));
	int temp_flag = 0;
//헤더 갯수 체크하는 부분 삭제
	 sf_stat_data->service_id = REPRESENT_NUM;

	 LOGGER(TRACE_LEVEL1, "file_name:%s \n", file_name);

     if(strstr(file_name, "subscriber"))
     {
         sf_stat_data->AREA_ETC = 1;

         if((strstr(file_name, "cpp") != NULL) || 
            (strstr(file_name, "vdss") != NULL))
         {
             sf_stat_data->AREA_ETC = 0;
         }
     }
     else if(strstr(file_name, "dest"))
     {
         sf_stat_data->AREA_ETC = 1;

         if((strstr(file_name, "cpp") != NULL))
         {
             sf_stat_data->AREA_ETC = 0;
         }
     }

	for(s = 0; s < stat_count; s++)
	{
///////////////////////////////////////////////////////////////////////////////////////////
		if(strstr(file_name, g_stat_array[s].file_name))
		{
			strcpy(sf_stat_data->STAT_NAME, g_stat_array[s].TABLE_NAME);
			if(g_stat_array[s].header_count == 3)
			{
				bNext = getItem( orgin_data, sf_stat_data->RESERV_KEY1  , ',' );	
				sf_stat_data->param_num_check = 1;
			}
			else
			{
				bNext = getItem( orgin_data, sf_stat_data->RESERV_KEY1  , ',' );
        		bNext = getItem( orgin_data, sf_stat_data->RESERV_KEY2  , ',' );
        		sf_stat_data->param_num_check = 0;
			}
			sf_stat_data->NUM_OF_STAT = g_stat_array[s].stat_num;
			break;
		}
	}
}
 
int insUserStatToDB( char *sData, char *sCatName )
{
	char * pPos = (char *)sData;
	//char sLogTime[32], sASIdx[10], sTSAID[32], sSubKey[32], sItem[20];
	char sLogStart_Time[32],sLogEnd_Time[32], sASIdx[10],sItem[20], sSubKey[32];
	char svkeyParam[30];
	int  nItem[STAT_MAXIMUM];
	int  bItem[STAT_MAXIMUM];
	int  nCnt, nAS;
		char sSQLCmd[2048];
	char *pTmp;
	bool bNext;
	int  i;
	std::string sResult;
	bool bOk;

	int db_area_column= 0;
	int nDb;

	STAT_DATA sStat_Data;

	nCnt = 0;
	nAS = 0;
	sSubKey[0] = 0x00;
	strcpy(sSubKey, "-");

	memset(svkeyParam, 0x00, sizeof(svkeyParam));
	sprintf(svkeyParam, "%s", sCatName);
	
	for( i = 0; i < STAT_MAXIMUM; i++ ) nItem[i] = 0;
	for( i = 0; i < STAT_MAXIMUM; i++ ) bItem[i] = 0;	
//공통 field value	
    bNext = getItem( pPos, sLogStart_Time  , ',' ); //start_time
	bNext = getItem( pPos, sASIdx  , ',' );

//1541 AS check
    nAS= atoi(sASIdx);

//1588,1577,1899
	
        if( (nAS == AS15 || nAS == AS25) &&  strstr(svkeyParam, "_ti") == NULL && strstr(svkeyParam, "_vds") == NULL && strstr(svkeyParam, "_ivr") == NULL && strstr(svkeyParam, "_cicf") == NULL ) 
        {
//1541
                LOGGER(TRACE_LEVEL2, "#### insUserStatTo1541");

                insUserStatTo1541(&sStat_Data, pPos, svkeyParam); 
                if(nAS == 15)
                {
                        nAS=0; 
                        sprintf(sASIdx,"%d",nAS);
                }
                else if(nAS == 25)
                {
                        nAS=1; 
                        sprintf(sASIdx,"%d",nAS);
                }       
        }
	else
        {
                LOGGER(TRACE_LEVEL2, "#### else logic");

                if(nAS == 15)
                {
                        nAS = 0;
                        sprintf(sASIdx,"%d",nAS);
                }
                if(nAS == 25)
                {
                        nAS = 1;
                    sprintf(sASIdx,"%d",nAS);
                }
//1588,1577,1899
                insUserStatToSt(&sStat_Data, pPos, svkeyParam);
        }
	for( nCnt = 0; nCnt < sStat_Data.NUM_OF_STAT; nCnt++ )
	{
		
		if( bNext == false ) break;
		bNext = getItem( pPos, sItem, ',' );
		nItem[ nCnt ] = atoi( sItem );
	}

#ifdef _SKIP_AREA_ETC
    if(sStat_Data.AREA_ETC == 1)
    {
			db_area_column = nItem[sStat_Data.NUM_OF_STAT - 1];
			for( nDb = AREA_ETC_NUM ; nDb < sStat_Data.NUM_OF_STAT; nDb++)		 	
			{
				if(nDb == (sStat_Data.NUM_OF_STAT - 1) )bItem[AREA_ETC_NUM] = db_area_column;
				else
				{ 
					bItem[nDb+1] = nItem[nDb];
				}
			}

			for( nDb = AREA_ETC_NUM ; nDb < sStat_Data.NUM_OF_STAT; nDb++)
			{
				nItem[nDb] = bItem[nDb];
	
			}		    	 
    }
	
#endif

	pTmp = sSQLCmd;


//header가 3개인 경우
	if(sStat_Data.param_num_check) 
	{
//20161206 only mpbx
#ifdef MPBX_MODE
		pTmp += sprintf(sSQLCmd, "INSERT INTO %s VALUES(to_date('%s','YYYYMMDDHH24MISS'),'%s','%s','A','B',0,0,0", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1);
#else
		pTmp += sprintf(sSQLCmd, "INSERT INTO %s VALUES(to_date('%s','YYYYMMDDHH24MISS'),'%s','%s'", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1);
#endif
		LOGGER(TRACE_LEVEL2, "Header 3 INSERT INTO %s VALUES('%s','%s','%s' \n", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1);
	}
//header가 4개인 경우
	else
	{
//20161206 only mpbx
#ifdef MPBX_MODE
		pTmp += sprintf(sSQLCmd, "INSERT INTO %s VALUES(to_date('%s','YYYYMMDDHH24MISS'),'%s','%s','%s','A','B',0,0,0", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1, &sStat_Data.RESERV_KEY2);
#else
		pTmp += sprintf(sSQLCmd, "INSERT INTO %s VALUES(to_date('%s','YYYYMMDDHH24MISS'),'%s','%s','%s'", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1, &sStat_Data.RESERV_KEY2);
#endif
		LOGGER(TRACE_LEVEL2, "Header 4 INSERT INTO %s VALUES('%s','%s','%s','%s' \n", &sStat_Data.STAT_NAME, sLogStart_Time,  sASIdx, &sStat_Data.RESERV_KEY1, &sStat_Data.RESERV_KEY2);

	}

	if(sStat_Data.service_id == COLLECTCALL_NUM)
        {
                Parsing_CollectCall_Num(pTmp, nItem,  &sStat_Data );
        }
	else if (sStat_Data.service_id == REPRESENT_NUM)
	{
	   for( i = 0; i < sStat_Data.NUM_OF_STAT; i++ )
            {
                pTmp += sprintf( pTmp, ",%d", nItem[i] );
            }
	}

	strcpy( pTmp, ")" );
		


	
    LOGGER(TRACE_LEVEL2,"sqlExecute: %s\n",sSQLCmd);
#ifdef OCCI
	bOk = m_occi.sqlExecute( sSQLCmd, sResult );
	m_occi.commit();
#endif
#ifdef ALTI_V6
    bOk = m_atci.sqlExecute( sSQLCmd, sResult );
    m_atci.commit();
#endif

	if( bOk == false ) return -2;
	return 0;
}

bool getItem(char* & pSrc, char* pBuff, char cSplit)
{
	while(*pSrc!= 0x00 && *pSrc!= cSplit && *pSrc!= '\n' && *pSrc!= '\r') *pBuff++ = *pSrc++;

	*pBuff = 0x00;

	if( *pSrc == cSplit ) pSrc++;

	if( *pSrc == 0x00 ) return false;
	if( *pSrc == '\n' || *pSrc == '\r') return false;

	return true;
}


void insUserStatTo1541(STAT_DATA *sf_stat_data, char* & orgin_data, char *file_name)
{
    bool bNext;
    memset(sf_stat_data, 0x00, sizeof(STAT_DATA));
    int temp_flag = 0;


    //number of 1541 header only 3?

    bNext = getItem( orgin_data, sf_stat_data->RESERV_KEY1  , ',' );
    sf_stat_data->param_num_check = 1;
    sf_stat_data->service_id = COLLECTCALL_NUM;
    ///////////////////////////////////////////////////////////////////////////////////////////     
    if(strstr(file_name, "ssw"))
    {
        strcpy(sf_stat_data->STAT_NAME, SSW_STAT);
        sf_stat_data->NUM_OF_STAT = SSW_NUM;
    }
    else  if(strstr(file_name, "_ms_"))
    {
        strcpy(sf_stat_data->STAT_NAME, MS_STAT);
        sf_stat_data->NUM_OF_STAT = MS_NUM;
    }
    else  if(strstr(file_name, "_mscmd_"))
    {
        strcpy(sf_stat_data->STAT_NAME, MSCMD_STAT);
        sf_stat_data->NUM_OF_STAT = MSCMD_NUM;
    }
    else  if(strstr(file_name, "area"))
    {
        strcpy(sf_stat_data->STAT_NAME, AREA_STAT);
        sf_stat_data->NUM_OF_STAT = AREA_NUM;
    }
    else  if(strstr(file_name, "traffic"))
    {
        strcpy(sf_stat_data->STAT_NAME, TRAFFIC_STAT);
        sf_stat_data->NUM_OF_STAT = TRAFFIC_NUM;
    }
    else if(strstr(file_name, "military"))
    {
        strcpy(sf_stat_data->STAT_NAME, MILITARY_STAT);
        sf_stat_data->NUM_OF_STAT = MILITARY_NUM;
        LOGGER(TRACE_LEVEL2, "#### filename : military");
        LOGGER(TRACE_LEVEL2, "#### sf_stat_data->RESERV_KEY1 : [%s]", sf_stat_data->RESERV_KEY1);
    }
    else if(strstr(file_name, "direct_sms"))
    {
        strcpy(sf_stat_data->STAT_NAME, SMS_STAT);
        sf_stat_data->NUM_OF_STAT = SMS_NUM;
    }
    else
    {
        LOGGER(TRACE_LEVEL2, "NOT 1541 stat File : [%s]", file_name);
        sf_stat_data->ref = -1;
    }

}



void Parsing_CollectCall_Num(char* &pDest, int* pSrc, STAT_DATA *sf_stat_data)
{

    if( strstr(sf_stat_data->STAT_NAME, "AREA") )
    {
        InsertAreaSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "SSW") )
    {
        InsertSswSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "OFFICE") )
    {
        InsertOfficeSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "CALL") )
    {
        InsertMsSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "MESSAGE") )
    {
        InsertMscmdSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "MILITARY") )
    {
        InsertMiliSTS(pDest, pSrc);
    }
    else if( strstr(sf_stat_data->STAT_NAME, "SMS") )
    {
        InsertSMSSTS(pDest, pSrc);
    }
}


void InsertMsSTS(char* &pDest, int *pSrc)
{
    int i;
    for( i = 0; i < MS_DB; i++ )
    {
        pDest += sprintf( pDest, ",%d", pSrc[i] );
    }
}



void InsertOfficeSTS(char* &pDest, int *pSrc)
{

    int i;
    for(i=0 ;i < TRAFFIC_DB; i++)
    {
        if(i == 0 )pDest += sprintf( pDest, ",%d", pSrc[i] ); //att_cnt
        else if(i == 1)pDest += sprintf( pDest, ",%d", pSrc[1] + pSrc[2] + pSrc[3]+ pSrc[4] +pSrc[5] + pSrc[6]); //succ1_cnt
        else if(i == 2)pDest += sprintf( pDest, ",%d", pSrc[1] + pSrc[2]+ pSrc[3] + pSrc[4]); //voice_succ
        else if(i > 2 && i < 7)pDest += sprintf( pDest, ",%d", pSrc[ i-2 ]); //voice0 ~ voice3
        else if(i == 7)pDest += sprintf( pDest, ",%d", pSrc[5] + pSrc[6]); //video_succ
        else if(i > 7 && i < 10)pDest += sprintf( pDest, ",%d", pSrc[ i-3 ]); //video0 ~ video1
        else if(i == 10)pDest += sprintf( pDest, ",%d", pSrc[7] + pSrc[8] + pSrc[9]+ pSrc[10] +pSrc[11] + pSrc[12] ); //succ2_cnt
        else if(i == 11)pDest += sprintf( pDest, ",%d", pSrc[7] + pSrc[8] + pSrc[9]+ pSrc[10] ); //succ2_voice_cnt
        else if(i > 11 && i < 16)pDest += sprintf( pDest, ",%d", pSrc[ i-5 ]);  //succ2_voice
        else if(i == 16)pDest += sprintf( pDest, ",%d", pSrc[11] + pSrc[12]);       //succ2_video
        else if(i > 16 && i < 19)pDest += sprintf( pDest, ",%d", pSrc[ i-6 ]);  //video0 ~ video1
    }
}



void InsertAreaSTS(char* &pDest, int *pSrc)
{
    int i;
    for( i = 0; i < AREA_DB; i++ )
    {
        if(i <=1 )pDest += sprintf( pDest, ",%d", pSrc[i] ); //att_cnt, succ1_cnt
        else if(i == 2)pDest += sprintf( pDest, ",%d", pSrc[2] + pSrc[3] + pSrc[4] + pSrc[5] + pSrc[6] + pSrc[7]); //succ2_cnt
        else if(i == 3)pDest += sprintf( pDest, ",%d", pSrc[2] + pSrc[3] + pSrc[4] + pSrc[5]); //succ_voice_cnt
        else if(i > 3 && i <= 7)pDest += sprintf( pDest, ",%d", pSrc[ i-2 ]); //voice0 ~ voice 3
        else if(i == 8 )pDest += sprintf( pDest, ",%d", pSrc[6] + pSrc[7]); //succ_video_cnt
        else if(i > 8 && i < 11)pDest += sprintf( pDest, ",%d", pSrc[ i-3 ]); //video0 ~ video1
        else if(i >= 11 && i < 14)pDest += sprintf( pDest, ",%d", pSrc[ i-3 ]); //MILITARY_ORI ~ ETC_ORI
    }
}

void InsertSswSTS(char* &pDest, int *pSrc)
{
    int i;
    int digit_flag;


    for( i = 0; i < SSW_DB; i++ )
    {
        if(i < 5 )pDest += sprintf( pDest, ",%d", pSrc[i] ); //att_cnt, voice, vedio, etc, succ1_cnt
        else if(i == 5)pDest += sprintf( pDest, ",%d", pSrc[5] + pSrc[6]+ pSrc[7] +pSrc[8] + pSrc[9] + pSrc[10]); //succ2_cnt
        else if(i == 6)pDest += sprintf( pDest, ",%d", pSrc[5] + pSrc[6]+ pSrc[7] + pSrc[8]); //succ2_voice_cnt
        else if(i > 6 && i < 11)pDest += sprintf( pDest, ",%d", pSrc[ i-2 ]); //succ2_voice0 ~ succ2_voice3
        else if(i == 11)pDest += sprintf( pDest, ",%d", pSrc[9] + pSrc[10]); //succ_2video_cnt
        else if(i > 11 && i < 14)pDest += sprintf( pDest, ",%d", pSrc[ i-3 ]); //succ2_video0 ~ succ2_video1
        else if(i == 14)pDest += sprintf( pDest, ",%d", pSrc[0] - (pSrc[5] + pSrc[6]+ pSrc[7] +pSrc[8] + pSrc[9] + pSrc[10]) ); //fail_cnt                              
        else if(i == 15)digit_flag=pSrc[i-4];
        else if(i > 15 && i < 37)
        {
            if(i==36)
            {
                pDest += sprintf( pDest, ",%d", pSrc[ i-4 ]);
                pDest += sprintf(pDest, ",%d", digit_flag);
            }
            else
                pDest += sprintf( pDest, ",%d", pSrc[ i-4 ]); //ab0 ~ ab6, rc0 ~ rc13
        }
        else if(i == 37)pDest += sprintf( pDest, ",%d", pSrc[33] + pSrc[34]+ pSrc[35]); //fail_ms_cnt
        else if(i > 37 && i < 41)pDest += sprintf( pDest, ",%d", pSrc[ i-5 ]); //failms0 ~ failms2
        else if(i == 41)pDest += sprintf( pDest, ",%d", pSrc[36] + pSrc[37]+ pSrc[38]); //fail_scp_cnt                  
        else if(i > 41 && i < 45)pDest += sprintf( pDest, ",%d", pSrc[ i-6 ]); //fail_scp0 ~ fail_scp2
        else 
        {
            pDest += sprintf( pDest, ",%d", pSrc[i-6] );
        }
    }
}

void InsertMiliSTS(char* &pDest, int *pSrc)
{
    int i;
    int digit_flag;


    for( i = 0; i < MILITARY_DB; i++ )
    {
        if(i < 5 )pDest += sprintf( pDest, ",%d", pSrc[i] ); //att_cnt, voice, vedio, etc, succ1_cnt
        else if(i == 5)pDest += sprintf( pDest, ",%d", pSrc[5] + pSrc[6]+ pSrc[7] +pSrc[8] + pSrc[9] + pSrc[10]); //succ2_cnt
        else if(i == 6)pDest += sprintf( pDest, ",%d", pSrc[5] + pSrc[6]+ pSrc[7] + pSrc[8]); //succ2_voice_cnt
        else if(i > 6 && i < 11)pDest += sprintf( pDest, ",%d", pSrc[ i-2 ]); //succ2_voice0 ~ succ2_voice3
        else if(i == 11)pDest += sprintf( pDest, ",%d", pSrc[9] + pSrc[10]); //succ_2video_cnt
        else if(i > 11 && i < 14)pDest += sprintf( pDest, ",%d", pSrc[ i-3 ]); //succ2_video0 ~ succ2_video1
        else if(i == 14)pDest += sprintf( pDest, ",%d", pSrc[0] - (pSrc[5] + pSrc[6]+ pSrc[7] +pSrc[8] + pSrc[9] + pSrc[10]) ); //fail_cnt                              
        else if(i == 15)digit_flag=pSrc[i-4];
        else if(i > 15 && i < 37)
        {
            if(i==36)
            {
                pDest += sprintf( pDest, ",%d", pSrc[ i-4 ]);
                pDest += sprintf(pDest, ",%d", digit_flag);
            }
            else
                pDest += sprintf( pDest, ",%d", pSrc[ i-4 ]); //ab0 ~ ab6, rc0 ~ rc13
        }
        else if(i == 37)pDest += sprintf( pDest, ",%d", pSrc[33] + pSrc[34]+ pSrc[35]); //fail_ms_cnt
        else if(i > 37 && i < 41)pDest += sprintf( pDest, ",%d", pSrc[ i-5 ]); //failms0 ~ failms2
        else if(i == 41)pDest += sprintf( pDest, ",%d", pSrc[36] + pSrc[37]+ pSrc[38]); //fail_scp_cnt                  
        else if(i > 41 && i < 45)pDest += sprintf( pDest, ",%d", pSrc[ i-6 ]); //fail_scp0 ~ fail_scp2
    }
}

void InsertSMSSTS(char* &pDest, int *pSrc)
{
    int i;
    for( i = 0; i < SMS_DB; i++ )
    {
        pDest += sprintf( pDest, ",%d", pSrc[i] );
    }
}





