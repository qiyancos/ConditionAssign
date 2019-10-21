#! /bin/bash
set -e

if [ $# -lt 2 ]
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

srcCount=`find $srcDir -name "*.conf" | wc -l`
targetCount=`find $targetDir -name "*.conf" | wc -l`
if [ $srcCount != $targetCount ]
then
    echo "Config files do not have the same count $srcCount:$targetCount"
    exit -1
fi

configFiles=`find $srcDir -name "*.conf"`
for filePath in $configFiles
do
    fileName=`basename $filePath`
    if [ "x`diff $srcDir/$fileName $targetDir/$fileName`" != x ]
    then
        echo ">> $fileName is different between two src."
        diff $srcDir/$fileName $targetDir/$fileName
        exit -1
    fi
done

echo ">> Check passed."
