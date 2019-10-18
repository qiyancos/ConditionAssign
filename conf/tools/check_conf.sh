#! /bin/bash
set -e

if [ $# -lt 3 ]
then
    echo "$0 <src dir> <target dir>"
    exit 0
fi

srcDir=$1
targetDir=$2

if [ ! -d $srcDir ]
then
    echo "Error: No such directory \"$srcDir\"."
    exit -1;
fi

if [ ! -d $targetDir ]
then
    echo "Error: No such directory \"$targetDir\"."
    exit -1;
fi

srcCount=`find -name "*.conf" $srcDir | wc -l`
targetCount=`find -name "*.conf" $targetDir | wc -l`
if [ $srcCount -gt $targetCount ]
then
    configFiles=`find -name `
fi

configFiles
