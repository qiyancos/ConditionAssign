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

for conf in $originConfigs
do
    if [ x`echo $conf | grep "_Old"` != x ]
    then continue
    fi
    layerName=`basename $conf | sed 's/.conf//g'`
    layerNames="$layerName $layerNames"
done
layerNames=($layerNames)

while [ 1 ]
do
    index=1
    for layer in ${layerNames[*]}
    do
        echo "[$index] $layer"
        index=$[index + 1]
    done
    echo -n "Please input the config file index: "
    read input
    if [ "x$input" = x ]
    then echo "<Empty Input> Please reinput! "
    else break
    fi
done

serial=0
for index in $input
do
    layer=${layerNames[$[index - 1]]}
    if [ -f $root/data/$layer.mif ]
    then srcLayers="$srcLayers;$root/data/$layer.mif"
    else serial=1
    fi
    if [ "`ls $root/data/${layer}_plugin*.mif`"x != x ]
    then
        for plugin in $(ls $root/data/${layer}_plugin*.mif)
        do pluginLayers="$pluginLayers;$plugin"
        done
    fi
    targetLayers="$targetLayers;$root/data/${layer}_Out_New.mif"
    confFiles="$confFiles;$root/conf/$layer.conf"
done

confFiles=${confFiles:1:$[${#confFiles} - 1]}
srcLayers=${srcLayers:1:$[${#srcLayers} - 1]}
targetLayers=${targetLayers:1:$[${#targetLayers} - 1]}
pluginLayers=${pluginLayers:1:$[${#pluginLayers} - 1]}
if [ x$pluginLayers = x ]
then pluginLayers=NULL 
fi

# New test
for executorCnt in $MaxExecutors
do
    rm -rf $root/log_New/log_*.txt
    echo ">> Running with new condition assign:"
    logPath="$root/log_New"
    echo -n "$root/../bin/ConditionsAssign NULL $srcLayers $targetLayers "
    echo "${executorCnt} $logPath $confFiles $pluginLayers"
    if [ ${1}x = -rawx -o ${2}x = -rawx ]
    then
        $root/../bin/ConditionAssign NULL NULL $srcLayers $targetLayers \
                ${executorCnt} $logPath $confFiles $pluginLayers
    else
        timeNew=`(time $root/../bin/ConditionAssign NULL NULL $srcLayers \
                $targetLayers ${executorCnt} $logPath $confFiles \
                $pluginLayers) 2>&1 | awk '/real/ {print $2}'`
        cat $root/log_New/log_$date.txt
    fi
    
    if [ x$timeNew != x ]
    then
        echo "Running with executors[$executorCnt]"
        echo "Total time: $timeNew"
    fi
done
