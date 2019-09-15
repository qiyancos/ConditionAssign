#! /bin/bash
set -e
withNewLayerList="C_N_New"
newLayerList="C_TrafficLight"
withPluginLayerList=""
pluginLayerList=""

checkFileDirExist() {
    if [ x$1 = x ]
    then
        echo "Error: no such file or directory \"$1\"."
        exit -1
    elif [ ! -f $1 -a ! -d $1 ]
    then
        echo "Error: no such file or directory \"$1\"."
        exit -1
    fi
}

completeMifLayerName() {
    if [ x$1 != x ]
    then
        if [ -f $1.mif ]
        then echo $1.mif
        elif [ -f $1.MIF ]
        then echo $1.MIF
        elif [ x$2 != xnoexit ]
        then
            echo "Error: Can not find mif file for layer \"$1\"."
            exit -1
        fi
    fi
}

mapFinder() {
    if [ x$1 != x ]
    then
        index=0
        for key in `eval echo \$$2`
        do
            if [ $1 = $key ]
            then
                eval echo \${$3[$index]}
                return
            fi
            index=$[index + 1]
        done
    fi
}

argParser() {
    root=`dirname $(dirname $0)`
    if [ $# != 9 ]
    then
        echo "Error: not enough or too many arguments."
        exit -1
    fi
    poiProcessScript=$1
    dataPath=`dirname $(dirname $2)`
    srcDataPath=$2/$7
    targetDataPath=$3/$7
    pluginDataPath=$4
    configFilePath=$5
    logPath=$6/$7
    cityName=$7
    threadNum=$8

    checkFileDirExist $poiProcessScript
    checkFileDirExist $srcDataPath
    checkFileDirExist $targetDataPath
    checkFileDirExist $pluginDataPath
    checkFileDirExist $configFilePath
    
    mkdir -p $logPath
}

findPluginLayer() {
    # TODO use eval to support multiple plugin layers
    layerName=`mapFinder $1 withPluginLayerList pluginLayerList`
    layerNameTemp=`completeMifLayerName $srcDataPath/$layerName noexit`
    if [ x$layerNameTemp = x ]
    then
        layerNameTemp=`completeMifLayerName $pluginDataPath/$cityName/$layerName noexit`
        if [ x$layerNameTemp = x ]
        then layerNameTemp=`completeMifLayerName $pluginDataPath/$layerName noexit`
            if [ x$layerNameTemp = x ]
            then
                echo "Error: Can not find plugin layer in any path for layer \"$1\"."
                exit -1
            fi
        fi
    fi
    echo $layerNameTemp
}

splitConfigFiles() {
    # find parallel layers
    for file in `ls $configFilePath`
    do
        layerName=${file:0:$[${#file} - 4]}
        if [ -f $srcDataPath/$layerName.mif -o -f $srcDataPath/$layerName.MIF ]
        then
            # For example: C_POI_1.conf is for serial mode
            if [ ! -f $configFilePath/${layerName}_1.conf ]
            then parallelLayers="$layerName $parallelLayers"
            else serialLayers="$layerName $serialLayers"
            fi
        elif [ "x${layerName:$[${#layerName} - 6]:6}" = "xFilter" ]
        then
            layerName=${layerName:0:$[${#layerName} - 7]}
            if [ -f $srcDataPath/$layerName.mif -o -f $srcDataPath/$layerName.MIF ]
            then filterLayers="$layerName $filterLayers"
            fi
        fi
    done
}

processParallelLayers() {
    # gen arguments for parallel layers
    for layerName in $parallelLayers
    do
        configFiles="${layerName}.conf;$configFiles"
        if [ ${layerName:$[${#layerName} - 4]:4} = _New ]
        then
            srcLayerName="`completeMifLayerName \
                    $srcDataPath/${layerName:0:$[${#layerName} - 4]}`"
            pluginLayerNames=`findPluginLayer $layerName`
            layerName=`mapFinder $layerName withNewLayerList newLayerList`
            if [ x$layerName = x ]
            then
                echo -n "Error: Can not find new output layer matched"
                echo " with config \"$file\"."
                exit -1
            fi
            if [ ${srcLayerName:$[${#srcLayerName} - 3]:3} = mif ]
            then targetLayerName="$targetDataPath/$layerName.mif<NEW>"
            else targetLayerName="$targetDataPath/$layerName.MIF<NEW>"
            fi
        else
            srcLayerName="`completeMifLayerName $srcDataPath/$layerName`"
            pluginLayerNames=`findPluginLayer $layerName`
            if [ ${srcLayerName:$[${#srcLayerName} - 3]:3} = mif ]
            then targetLayerName="$targetDataPath/$layerName.mif"
            else targetLayerName="$targetDataPath/$layerName.MIF"
            fi
        fi
        srcLayers="$srcLayerName;$srcLayers"
        targetLayers="$targetLayerName;$targetLayers"
        if [ "x$pluginLayerNames" != x ]
        then pluginLayers="$pluginLayerNames;$pluginLayers"
        fi
    done
    if [ "x$srcLayers" = x ]
    then return
    else
        srcLayers=${srcLayers:0:$[${#srcLayers} - 1]}
        targetLayers=${targetLayers:0:$[${#targetLayers} - 1]}
        configFiles=${configFiles:0:$[${#configFiles} - 1]}
    fi
    if [ "x$pluginLayers" = x ]
    then pluginLayers="NULL"
    else pluginLayers=${pluginLayers:0:$[${#pluginLayers} - 1]}
    fi
    echo ">> Running new condition assign in parallel mode:"
    echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayers"
    echo " $targetLayers $threadNum $logPath $configFiles $pluginLayers"
    $root/bin/ConditionAssign NULL NULL $srcLayers \
        $targetLayers $threadNum $logPath $configFiles $pluginLayers
}

poiFeatureProcess() {
    echo ">> Start POIFeatureProcess:"
    cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mid \
            $targetDataPath/C_AOI_FULL.mid
    cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mif \
            $targetDataPath/C_AOI_FULL.mif
    bash $poiProcessScript $targetDataPath $targetDataPath $cityName 5
    if [ $? != 0 ]
    then
        echo "Error: error(s) occurred while running poi freature process."
        exit -1
    fi
}

processSerialLayers() {
    for layerName in $serialLayers
    do
        srcLayer=`completeMifLayerName $srcDataPath/$layerName`
        if [ ${srcLayer:$[${#srcLayer} - 3]:3} = mif ]
        then targetLayer="$targetDataPath/$layerName.mif"
        else targetLayer="$targetDataPath/$layerName.MIF"
        fi
        pluginLayers=`findPluginLayer $layerName`
        if [ x$pluginLayers = x ]
        then pluginLayers="NULL"
        fi
        configFiles=$configFilePath/$layerName.conf
        index=1
        while [ -f $configFilePath/${layerName}_$index.conf ]
        do configFiles="$configFilePath/${layerName}_$index.conf"
        done
        echo ">> Running serial mode for layer \"$layerName\":"
        echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayer "
        echo "$targetLayer $threadNum $logPath $configFiles $pluginLayers"
        $root/bin/ConditionAssign NULL NULL $srcLayer \
            $targetLayer $threadNum $logPath $configFiles $pluginLayers
    done
}

processLayerFilter() {
    for layerName in $filterLayers
    do
        srcLayer=`completeMifLayerName $srcDataPath/$layerName`
        targetLayer="`completeMifLayerName $targetDataPath/$layerName`<New>"
        pluginLayer="NULL"
        configFile=$configFilePath/${layerName}_Filter.conf
        echo ">> Running layerFilter for layer \"$layerName\":"
        echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayer "
        echo "$targetLayer $threadNum $logPath $configFile $pluginLayer"
        $root/bin/ConditionAssign NULL NULL $srcLayer \
            $targetLayer $threadNum $logPath $configFile $pluginLayer
    done
}

argParser $*
splitConfigFiles
processParallelLayers
poiFeatureProcess
processSerialLayers
processLayerFilter
exit 0
