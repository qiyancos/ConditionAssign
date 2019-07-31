#! /bin/bash
set -e
root=`dirname $0`
cd $root
root=$PWD
cd ..
make -j8

# Argument list
OriginSourceLayers=`ls $root/data/*.mif`
if [ ${1}x = x ]
then MaxExecutors="1 4 8 16"
else MaxExecutors=$1
fi

index=0
for layer in $OriginSourceLayers
do
    layerName=`basename $layer | sed 's/.mif//g'`
    if [[ $layerName =~ ^.*_Out$ ]]
    then continue
    fi
    Modules[$index]="NULL"
    SourceLayers[$index]="$layer"
    TargetLayers[$index]="$root/data/${layerName}_Out.mif"
    LogPaths[$index]="$root/log/${layerName}.log"
    ConfPaths[$index]="$root/conf/${layerName}.conf"
    PluginLayers[$index]="NULL"
    SourceGeoTypes[$index]="POINT"
    index=$[index + 1]
done

for executorCnt in $MaxExecutors
do
    index=0
    while [ ${Modules[$index]}x != x ]
    do
        layerName=$(basename ${SourceLayers[$index]} | sed 's/.mif//g')
        echo -n ">> Running test [$layerName]"
        echo " with executor[$executorCnt]"
        cp $root/data/$layerName.mid $root/data/${layerName}_Out.mid
        cp $root/data/$layerName.mif $root/data/${layerName}_Out.mif
        echo -n "$root/../bin/ConditionsAssign ${Modules[$index]} "
        echo -n "${SourceLayers[$index]} ${TargetLayers[$index]} "
        echo -n "${executorCnt} ${LogPaths[$index]} ${ConfPaths[$index]} "
        echo "${PluginLayers[$index]}"
        time $root/../bin/ConditionAssign ${Modules[$index]} \
                ${SourceLayers[$index]} ${SourceGeoTypes[$index]} \
                ${TargetLayers[$index]} ${executorCnt} ${LogPaths[$index]} \
                ${ConfPaths[$index]} ${PluginLayers[$index]}
        index=$[index + 1]
    done
done
