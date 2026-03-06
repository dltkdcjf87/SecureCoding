#!/bin/bash
PRODUCT="AS"
SRC_PATH=.
VERSION_FILE="$SRC_PATH/VersionHistory.h"

function ParseVersion()
{
    DATE=`/bin/date --date '0 day ago' +'%Y%m%d'`
    VERSION=$1
    M_VERSION=$2
	AUTHOR=$3
    CNT=`echo $VERSION | awk -F. '{ print NF }'`;
    [ "$CNT" -lt 3 ] && return 1;
	CNT=`echo $M_VERSION | awk -F. '{ print NF }'`;
    [ "$CNT" -lt 3 ] && return 1;

    PVER1=`echo $VERSION | awk -F. '{ print int($1) }'`;
    PVER2=`echo $VERSION | awk -F. '{ print int($2) }'`;
    PVER3=`echo $VERSION | awk -F. '{ print int($3) }'`;
    MVER1=`echo $M_VERSION | awk -F. '{ print int($1) }'`;
    MVER2=`echo $M_VERSION | awk -F. '{ print int($2) }'`;
    MVER3=`echo $M_VERSION | awk -F. '{ print int($3) }'`;
   # REVNO=`git log -1 --abbrev-commit --pretty=oneline | awk -F" "  '{ print $1; }'`;
    REVNO=`echo "73d0548"`;
    PKGVER="${PVER1}.${PVER2}.${PVER3}_m${MVER1}.${MVER2}.${MVER3}_r$REVNO"
    BUILDVER="p${PVER1}.${PVER2}.${PVER3}-m${MVER1}.${MVER2}.${MVER3}"
    PKGNAME="${PRODUCT}v${PKGVER}"
    return 0;
}


function MakeVersionHeader()
{
    echo "" > $VERSION_FILE
    echo "#pragma once" >> $VERSION_FILE
    echo "" >> $VERSION_FILE
    echo "#ifndef PKGVER" >> $VERSION_FILE
    echo "#define PKGVER            (\"${PKGVER}\")" >> $VERSION_FILE
    echo "#define PRODUCT           (\"${PRODUCT}\")" >> $VERSION_FILE
    echo "#define PKGNAME           (\"${PKGNAME}\")" >> $VERSION_FILE
    echo "#define PVER1             ($PVER1)" >> $VERSION_FILE
    echo "#define PVER2             ($PVER2)" >> $VERSION_FILE
    echo "#define PVER3             ($PVER3)" >> $VERSION_FILE
    echo "#define MVER1             ($MVER1)" >> $VERSION_FILE
    echo "#define MVER2             ($MVER2)" >> $VERSION_FILE
    echo "#define MVER3             ($MVER3)" >> $VERSION_FILE
    echo "#define REVNO             (\"$REVNO\")" >> $VERSION_FILE
    echo "#define BUILD_DATE        (\"$DATE\")" >> $VERSION_FILE
    echo "#define AUTHOR        (\"${AUTHOR}\")" >> $VERSION_FILE
    echo "" >> $VERSION_FILE
    echo "" >> $VERSION_FILE
    echo "" >> $VERSION_FILE
    echo "void versionDisplay(int argc, char* argv[]);" >> $VERSION_FILE
    echo "#endif" >> $VERSION_FILE
    echo "" >> $VERSION_FILE
}

function Usage()
{
	echo "Usage: ./build_info.sh [PKG Version] [NOD Version] [AUTHOR]" 
	echo "      PKG Version: digit.digit.digit" 
	echo "      MOD Version: digit.digit.digit" 
	echo "      EX) ./build_info.sh 4.0.5 5.12.6 MIN" 
}

echo "VersionHistory is generating ..."
ParseVersion $1 $2 $3;
RET_VAL=$?;

if [ "$RET_VAL" -eq "1" ] 
then
	Usage;
	exit -1;
fi

sleep 1;
MakeVersionHeader;
echo "VersionHistory is generated."

sleep 1;
echo ".... make clean"
make clean -f Makefile.ses_noha; 
sleep 1;
echo ".... make"
make -f Makefile.ses_noha BUILD_INFO=${BUILDVER};


exit 0
