#! /bin/bash
set -e
root=`dirname $0`
cd $root
root=$PWD
cd ..
date=`date +%Y%m%d`
make -j8

# Argument list
originConfigs=`ls $root/conf/*.conf`
if [ ${1}x = x ]
then MaxExecutors="1 4 8 16"
else MaxExecutors=$1
fi

index=0
for conf in $originConfigs
do
    if [ x`echo $conf | grep "_Old"` != x ]
    then continue
    fi
    layerName=`basename $layer | sed 's/.conf//g'`
    layerNames="$layerName $layerNames"
done
layerNames=($layerNames)

echo -n "Please input the config file index: "
while [1]
do
    index=0
    for layer in $layerNames
    do
        echo "[$index] $layer"
        index=$[index + 1]
    done
    read input
    if [ "x$input" = x ]
    then echo -n "<Empty Input> Please reinput: "
    else break
    fi
done

first=0
for index in $input
do
    layer=${layerNames[$index]}
    if [ $first = 0 ]
    then srcLayer
    first=$[first + 1]
done

    # New test
    rm -rf $root/log_New/log_*.txt
    echo ">> Running with new condition assign:"
    if [ "`ls $root/data/${layerName}_plugin*.mif`"x != x ]
    then
        for plugin in $(ls $root/data/${layerName}_plugin*.mif)
        do pluginLayers="$plugin;$pluginLayers"
        done
        pluginLayers=${pluginLayers:0:$[${#pluginLayers} - 1]}
    else pluginLayers="NULL"
    fi
    targetLayer="$root/data/${layerName}_Out_New.mif"
    logPath="$root/log_New"
    echo -n "$root/../bin/ConditionsAssign ${Modules[$index]} "
    echo -n "${SourceLayers[$index]} ${targetLayer} "
    echo -n "${executorCnt} ${logPath} ${ConfPaths[$index]} "
    echo "$pluginLayers"
    if [ ${1}x = -rawx -o ${2}x = -rawx ]
    then
        $root/../bin/ConditionAssign ${Modules[$index]} \
                ${SourceLayers[$index]} ${targetLayer} ${executorCnt} \
                ${logPath} ${ConfPaths[$index]} $pluginLayers
    else
        timeNew=`(time $root/../bin/ConditionAssign ${Modules[$index]} \
                ${SourceLayers[$index]} ${targetLayer} ${executorCnt} \
                ${logPath} ${ConfPaths[$index]} $pluginLayers) 2>&1 | \
                awk '/real/ {print $2}'`
        cat $root/log_New/log_$date.txt
    fi
    newTested=1
}
