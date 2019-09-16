#! /bin/bash
set -e
root=`dirname $0`
cd $root
root=$PWD
cd ..
date=`date +%Y%m%d`
make -j8

# Argument list
OriginSourceLayers=`ls $root/conf/*.conf`
if [ ${1}x = x ]
then MaxExecutors="1 4 8 16"
else MaxExecutors=$1
fi
allowNew=1
allowOld=1
if [ ${2}x = -oldx ]
then unset allowNew
elif [ ${2}x = -newx ]
then unset allowOld
fi

index=0
for layer in $OriginSourceLayers
do
    if [ x`echo $layer | grep "_Old"` != x ]
    then continue
    fi
    layerName=`basename $layer | sed 's/.conf//g'`
    if [ ! -f $root/data/$layerName.mif ]
    then continue
    fi
    layerNames="$layerName $layerNames"
    eval ${layerName}Index=$index
    Modules[$index]="NULL"
    SourceLayers[$index]="$root/data/${layerName}.mif"
    ConfPaths[$index]="$root/conf/${layerName}.conf"
    index=$[index + 1]
done

testNew(){
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
    if [ x${layerName:$[${#layerName} - 6]:6} = xFilter ]
    then targetLayer="$root/data/${layerName}_Out_New.mif<NEW>"
    elif [ x${layerName:$[${#layerName} - 4]:4} = x_New ]
    then targetLayer="$root/data/${layerName}_Out_New_New.mif<NEW>"
    else targetLayer="$root/data/${layerName}_Out_New.mif"
    fi
    logPath="$root/log_New"
    echo -n "$root/../bin/ConditionsAssign.bin ${Modules[$index]} NULL "
    echo -n "${SourceLayers[$index]} ${targetLayer} "
    echo -n "${executorCnt} ${logPath} ${ConfPaths[$index]} "
    echo "$pluginLayers"
    if [ ${1}x = -rawx -o ${2}x = -rawx ]
    then
        $root/../bin/ConditionAssign.bin ${Modules[$index]} NULL \
                ${SourceLayers[$index]} ${targetLayer} ${executorCnt} \
                ${logPath} ${ConfPaths[$index]} $pluginLayers
    else
        timeNew=`(time $root/../bin/ConditionAssign.bin ${Modules[$index]} \
                NULL ${SourceLayers[$index]} ${targetLayer} ${executorCnt} \
                ${logPath} ${ConfPaths[$index]} $pluginLayers) 2>&1 | \
                awk '/real/ {print $2}'`
        cat $root/log_New/log_$date.txt
    fi
    newTested=1
}

testOld() {
    # Old test
    rm -rf $root/log_Old/log_*.txt
    echo ">> Running with old condition assign:"
    targetLayer="$root/data/${layerName}_Out_Old.mif"
    confPath="$root/conf/${layerName}_Old.conf"
    logPath="$root/log_Old"
    echo -n "$root/../../ConditionAssign_old/bin/ConditionsAssign "
    echo -n "${SourceLayers[$index]} ${targetLayer} "
    echo "${ConfPaths[$index]} ${logPath} ${executorCnt}"
    if [ ${1}x = -rawx -o ${2}x = -rawx ]
    then
        $root/../../ConditionAssign_old/bin/ConditionAssign \
                ${SourceLayers[$index]} ${targetLayer} \
                ${confPath} ${logPath} ${executorCnt}
    else
        timeOld=`(time $root/../../ConditionAssign_old/bin/ConditionAssign \
                ${SourceLayers[$index]} ${targetLayer} \
                ${confPath} ${logPath} ${executorCnt}) 2>&1 | \
                awk '/real/ {print $2}'`
        cat $root/log_Old/log_$date.txt
    fi
    oldTested=1
}

select layerName in $layerNames
do
    unset newTested
    unset oldTested
    index=`eval echo -e '$'${layerName}Index`
    for executorCnt in $MaxExecutors
    do
        echo -n ">> Running test [$layerName]"
        echo " with executor[$executorCnt]"
        if [ ${allowNew}x = 1x ]
        then testNew $2 $3
        fi
        if [ -f $root/conf/${layerName}_Old.conf -a ${allowOld}x = 1x ]
        then testOld $2 $3
        fi
        if [ ${oldTested}x = 1x -a ${newTested}x = 1x ]
        then
            echo "Start Result Check:"
            $root/tools/mifdiff $root/data/${layerName}_Out_New.mif \
                    $root/data/${layerName}_Out_Old.mif
            echo 
        fi
        echo "Timing Report:"
        echo "Old Condition Assign: $timeOld"
        echo "New Condition Assign: $timeNew"
    done
    break
done
