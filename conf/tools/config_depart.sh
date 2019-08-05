#! /bin/bash
if [ ${1}x = x -o ${1}x = -hx ]
then
    echo "Usage: config_depart.sh <config file> <targetDir>"
    exit 0
fi

if [ ! -f $1 -o ! -d $2 ]
then
    echo "Error: No such file or directory."
    exit 1
fi

lines=`cat $1 | wc -l`
lineNum=1
rm -rf ./*.conf
while [ $lineNum -le $lines ]
do
    lineContent=`sed -n ${lineNum}p $1`
    if [ ${lineContent:0:1} = '#' ]
    then
        lineNum=$[lineNum + 1]
        continue
    fi
    targetLayer=`echo $lineContent | awk '{print $1}' | sed 's/;/ /g'`
    for layer in $targetLayer
    do echo "$lineContent" >> $2/$layer.conf
    done
    lineNum=$[lineNum + 1]
done
