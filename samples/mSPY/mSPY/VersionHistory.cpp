#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "VersionHistory.h"

//##############################################//
//##             VERSION HISTORY              ##//
//##############################################//

//##############################################################################################################//
//## VERSION  | DATE       |  NAME  |  DESCRIPTION                                                            ##//
//##==========================================================================================================##//
//##  1.0.1   | 2021.02.24 |   MIN  | init                                                                    ##//
//##          |            |        |                                                                         ##//
//##############################################################################################################//

void versionDisplay(int argc, char* argv[])
{
    if(argc > 2){
        printf(">> input arguments count mismatch\n");
        printf(">> Usage: [module name] -version\n");
        printf("          [module name] -v\n");
        exit(-1);
    }

    if(argc == 2){
        if(strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "-v") == 0) {
			printf(">>>> %s BRIDGETEC corp.\n",PRODUCT);
            printf(">>>> Package-name     : %s\n",PKGNAME);
            printf(">>>> Package-version  : %d.%d.%d\n",PVER1,PVER2,PVER3 );
            printf(">>>> Module-version   : %d.%d.%d\n",MVER1,MVER2,MVER3);
            printf(">>>> Revision-number  : %s\n",REVNO);
            printf(">>>> Build-date       : %s %s\n", __DATE__, __TIME__);
            printf(">>>> gcc %s\n", __VERSION__);
            exit(-1);
		}
		printf(">> unknown input option\n");
        printf(">> Usage: [module name] -version\n");
        printf("          [module name] -v\n");
        exit(-1);
	}
}
