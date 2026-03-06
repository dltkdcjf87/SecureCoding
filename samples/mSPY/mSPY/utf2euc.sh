#!/bin/bash

for I in ./*.h ./*.cpp ;
    do iconv -c -f utf-8 -t euc-kr $I > $I.tmp
	sleep 1
	mv $I.tmp $I ;
done

