// Global.cpp: implementation of the CGlobal class.
//
//////////////////////////////////////////////////////////////////////

#include "Environment.h"
#include "Global.h"
#include "Utility.h"
#include "Logger.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGlobal& theGlobal(void)
{
	static CGlobal global;

	return global;
}

CGlobal::CGlobal()
{
	m_ucEMSIdx = GetPrivateProfileInt("SERVER", "EMS", 0, GLOBAL_CONFIG_FILE);
	m_ucASIdx = GetPrivateProfileInt("AS", "INDEX", 0, GLOBAL_CONFIG_FILE);

	sprintf(szCSIMConfigFile, CSIM_CONFIG_FILE, m_ucEMSIdx);

#ifdef _BULK_STS
	memset(bBulkMode, 0, sizeof(bBulkMode));
#endif
}


void CGlobal::setLogLevel(int nLevel)
{
	char szTemp[10];
	sprintf(szTemp, "%d", nLevel);
	WritePrivateProfileString("CSIM", "LOGLEVEL", szTemp, szCSIMConfigFile); 
}

int CGlobal::getLogLevel(void)
{
	return GetPrivateProfileInt("CSIM", "LOGLEVEL", 3, szCSIMConfigFile); 
}

void CGlobal::processCheck(char* pName)
{

#if !defined(_WIN32) && !defined(WIN32)
    char    psname[128];
    FILE    *fp;

    snprintf(psname, 127, "/var/run/%s.pid", pName);
    if((fp = fopen(psname, "w")) == 0) { 
		LOGGER(TRACE_INFO, "pid file open error : %s", psname);
		exit(0); 
	}

    fprintf(fp, "%d", getpid());
    fclose(fp);

#endif
	return;
}

#ifdef _BULK_STS
void CGlobal::readBulkMode(void)
{
	int		nSize, i, j, nID;
	char	szTemp[256], szID[12];

	memset(szTemp, 0x00, sizeof(szTemp));
	GetPrivateProfileStringv2("CSIM", "BULK_MODE_ID", "", szTemp, sizeof(szTemp), szCSIMConfigFile);

//	LOGGER(TRACE_INFO, "BULK_MODE_ID [%s](%d)", szTemp, strlen(szTemp));
	nSize = strlen(szTemp);
	if (nSize <= 0)
		return;

	i = j = 0;
	while(i <= nSize)
	{
		if (isdigit(szTemp[i]))
			szID[j++] = szTemp[i];
		else if ((i > 0 && i == nSize) || szTemp[i] == ',' || szTemp[i] == '\n' || szTemp[i] == '\r' || szTemp[i] == '\0')
		{
			if (j > 0)
			{
				szID[j] = '\0';
				nID = atoi(szID);
				if (nID < MAX_AS_COUNT)
				{
					bBulkMode[nID] = true;
					LOGGER(TRACE_INFO, "ASID = %d, bulk mode (true)", nID);
				}
			}
			j = 0;
		}
		i++;
	}
}

bool CGlobal::isBulkMode(int nASID)
{
	if (nASID < 0 || nASID >= MAX_AS_COUNT)
		return(false);

	return(bBulkMode[nASID]);
}
#endif
