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
    layerNames="$layerName $layerNames"
    eval ${layerName}Index=$index
    Modules[$index]="NULL"
    SourceLayers[$index]="$root/data/${layerName}.mif"
    ConfPaths[$index]="$root/conf/${layerName}.conf"
    PluginLayers[$index]="NULL"
    SourceGeoTypes[$index]="POINT"
    index=$[index + 1]
done

testNew(){
    # New test
    rm -rf $root/log_New/*
    echo ">> Running with new condition assign:"
    cp $root/data/$layerName.mid $root/data/${layerName}_Out_New.mid
    cp $root/data/$layerName.mif $root/data/${layerName}_Out_New.mif
    targetLayer="$root/data/${layerName}_Out_New.mif"
    logPath="$root/log_New"
    echo -n "$root/../bin/ConditionsAssign ${Modules[$index]} "
    echo -n "${SourceLayers[$index]} ${targetLayer} "
    echo -n "${executorCnt} ${logPath} ${ConfPaths[$index]} "
    echo "${PluginLayers[$index]}"
    if [ ${1}x = -rawx -o ${2}x = -rawx ]
    then
        $root/../bin/ConditionAssign ${Modules[$index]} \
                ${SourceLayers[$index]} ${SourceGeoTypes[$index]} \
                ${targetLayer} ${executorCnt} ${logPath} \
                ${ConfPaths[$index]} ${PluginLayers[$index]}
    else
        timeNew=`(time $root/../bin/ConditionAssign ${Modules[$index]} \
                ${SourceLayers[$index]} ${SourceGeoTypes[$index]} \
                ${targetLayer} ${executorCnt} ${logPath} \
                ${ConfPaths[$index]} ${PluginLayers[$index]}) 2>&1 | \
                awk '/real/ {print $2}'`
    fi
    cat $root/log_New/log_$date.txt
    newMidMd5Sum=$(md5sum $(echo $targetLayer | sed 's/.mif/.mid/') \
            | awk '{print $1}')
    newMifMd5Sum=$(md5sum $targetLayer | awk '{print $1}')
    newTested=1
}

testOld() {
    # Old test
    rm -rf $root/log_Old/*
    echo ">> Running with old condition assign:"
    cp $root/data/$layerName.mid $root/data/${layerName}_Out_Old.mid
    cp $root/data/$layerName.mif $root/data/${layerName}_Out_Old.mif
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
    fi
    cat $root/log_Old/log_$date.txt
    oldMidMd5Sum=$(md5sum $(echo $targetLayer | sed 's/.mif/.mid/') \
            | awk '{print $1}')
    oldMifMd5Sum=$(md5sum $targetLayer | awk '{print $1}')
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
            if [ ${oldMifMd5Sum}x != ${newMifMd5Sum}x -o \
                    ${oldMifMd5sum}x != ${newMidMd5Sum}x ]
            then
                echo "Result do not match."
                #exit 1
            fi
        fi
        echo "Timing Report:"
        echo "Old Condition Assign: $timeOld"
        echo "New Condition Assign: $timeNew"
    done
    break
done
