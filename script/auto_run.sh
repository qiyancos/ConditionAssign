#! /bin/bash
set -e
enableDebug=0
# Max number of layers running in parallel mode
# make sure this will not be too large so that local memory is exceeded
maxParallelCnt=6
threshold=8
withNewLayerList="C_N_New"
newLayerList="C_TrafficLight"
C_SubwayST_Plugin="C_SubwayLine"
C_N_New_Plugin="C_N"
C_R_Plugin="mainroad"

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
        then exit -1
        fi
    fi
}

mapFinder() {
    name=`eval echo \$$1`
    if [ x$name != x ]
    then
        index=0
        for key in `eval echo \$$2`
        do
            if [ $name = $key ]
            then
                eval "$1=\${$3[$index]}"
                return
            fi
            index=$[index + 1]
        done
    fi
}

findPluginLayer() {
    pluginLayerNames=`eval echo -e '$'"${1}_Plugin"`
    if [ x$pluginLayerNames = x ]
    then
        unset $2
        return 
    fi
    unset layerNames
    for layerName in $pluginLayerNames
    do
        layerNameTemp=`completeMifLayerName $srcDataPath/$layerName noexit`
        if [ x$layerNameTemp = x ]
        then
            layerNameTemp=`find $pluginDataPath -name "$layerName.*"`
            candCnt=`find $pluginDataPath -name "$layerName.*" | wc -l`
            if [ $candCnt = 0 ]
            then
                echo -n "Error: Can not find plugin layer \"$layerName\""
                echo " in any path for layer \"$1\"."
                exit -1
            elif [ $candCnt != 1 ]
            then
                layerNameTemp=`find $pluginDataPath -name "$layerName.*" \
                        | grep $cityName | grep "mif\|MIF"`
                candCnt=`echo $layerNameTemp | wc -w`
                if [ $candCnt = 0 ]
                then
                    echo -n "Error: Can not find plugin layer \"$layerName\""
                    echo " in any path for layer \"$1\"."
                    exit -1
                elif [ $candCnt -gt 1 ]
                then
                    echo -n "Error: Too many candidate plugin layer "
                    echo "of \"$layerName\" for layer \"$1\"."
                    exit -1
                fi
            else
                layerNameTemp=`find $pluginDataPath -name "$layerName.mif"`
            fi
        fi
        layerNames="$layerNameTemp;$layerNames"
    done
    eval "$2=${layerNames:0:$[${#layerNames} - 1]}"
}

argParser() {
    root=`dirname $(dirname $0)`
    if [ $# != 8 ]
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

splitConfigFiles() {
    # find parallel layers
    echo ">> Start Spliting Config Files..."
    for file in `ls $configFilePath/*.conf`
    do
        file=`basename $file`
        layerName=${file:0:$[${#file} - 5]}
        if [ -f $srcDataPath/$layerName.mif -o -f $srcDataPath/$layerName.MIF ]
        then
            # For example: C_POI_1.conf is for serial mode
            if [ ! -f $configFilePath/${layerName}_1.conf ]
            then
				echo "-- Parallel: $layerName"
				parallelLayers="$layerName $parallelLayers"
            else
				echo "-- Serial: $layerName"
				serialLayers="$layerName $serialLayers"
            fi
        elif [ "x${layerName:$[${#layerName} - 6]:6}" = "xFilter" ]
        then
            layerName=${layerName:0:$[${#layerName} - 7]}
            if [ -f $srcDataPath/$layerName.mif -o \
                    -f $srcDataPath/$layerName.MIF ]
            then
				echo "-- Filter: $layerName"
				filterLayers="$layerName $filterLayers"
			fi
        elif [ "x${layerName:$[${#layerName} - 3]:3}" = "xNew" ]
        then
            layerName=${layerName:0:$[${#layerName} - 4]}
            if [ -f $srcDataPath/$layerName.mif -o \
                    -f $srcDataPath/$layerName.MIF ]
            then
				echo "-- Parallel_New: ${layerName}_New"
				parallelLayers="${layerName}_New $parallelLayers"
            fi
		fi
    done
}

processParallelLayers() {
    totalCnt=`echo $parallelLayers | wc -w`
    workedCnt=0
    parallelLayers=($parallelLayers)
    while [ $workedCnt -lt $totalCnt ]
    do
        unset srcLayers targetLayers pluginLayers configFiles
        if [ $[totalCnt - workedCnt] -gt $threshold ]
        then endCnt=$[workedCnt + maxParallelCnt]
        else endCnt=$totalCnt
        fi
        # gen arguments for parallel layers
        while [ $workedCnt != $endCnt ]
        do
            layerName=${parallelLayers[$workedCnt]}
            configFiles="$configFilePath/${layerName}.conf;$configFiles"
            if [ ${layerName:$[${#layerName} - 4]:4} = _New ]
            then
                srcLayerName="`completeMifLayerName \
                        $srcDataPath/${layerName:0:$[${#layerName} - 4]}`"
                findPluginLayer $layerName pluginLayerNames
                mapFinder layerName withNewLayerList newLayerList
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
                findPluginLayer $layerName pluginLayerNames
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
            workedCnt=$[workedCnt + 1]
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
        echo;echo ">> Running new condition assign in parallel mode:"
        echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayers"
        echo " $targetLayers $threadNum $logPath $configFiles $pluginLayers"
        if [ x$enableDebug != x1 ]
        then
            $root/bin/ConditionAssign NULL NULL $srcLayers \
                $targetLayers $threadNum $logPath $configFiles $pluginLayers
        fi
    done
}

poiFeatureProcess() {
    echo;echo ">> Running POIFeatureProcess:"
    if [ x$enableDebug != x1 ]
    then
        cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mid \
                $targetDataPath/C_AOI_FULL.mid
        cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mif \
                $targetDataPath/C_AOI_FULL.mif
    fi
    echo -n "Command: bash $poiProcessScript $targetDataPath "
    echo "$targetDataPath $cityName 5"
    if [ x$enableDebug != x1 ]
    then bash $poiProcessScript $targetDataPath $targetDataPath $cityName 5
    fi
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
        findPluginLayer $layerName pluginLayers
        configFiles=$configFilePath/$layerName.conf
        index=1
        while [ -f $configFilePath/${layerName}_$index.conf ]
        do
            configFiles="$configFiles;$configFilePath/${layerName}_$index.conf"
            findPluginLayer ${layerName}_$index pluginLayerTemp
            if [ x$pluginLayerTemp != x ]
            then pluginLayers="$pluginLayerTemp;$pluginLayers"
            fi
            index=$[index + 1]
        done
        if [ x$pluginLayers = x ]
        then pluginLayers="NULL"
        else pluginLayers=${pluginLayers:0:$[${#pluginLayers} - 1]}
        fi
        echo;echo ">> Running serial mode for layer \"$layerName\":"
        echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayer "
        echo "$targetLayer $threadNum $logPath $configFiles $pluginLayers"
        if [ x$enableDebug != x1 ]
        then
            $root/bin/ConditionAssign NULL NULL $srcLayer \
                $targetLayer $threadNum $logPath $configFiles $pluginLayers
        fi
    done
}

processLayerFilter() {
    for layerName in $filterLayers
    do
        srcLayer=`completeMifLayerName $srcDataPath/$layerName`
        targetLayer="`completeMifLayerName $targetDataPath/$layerName`<NEW>"
        pluginLayer="NULL"
        configFile=$configFilePath/${layerName}_Filter.conf
        echo;echo ">> Running layerFilter for layer \"$layerName\":"
        # single thread will be faster for layer filter
        echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayer "
        echo "$targetLayer 1 $logPath $configFile $pluginLayer"
        if [ x$enableDebug != x1 ]
        then
            $root/bin/ConditionAssign NULL NULL $srcLayer \
                $targetLayer 1 $logPath $configFile $pluginLayer
        fi
    done
}

# 解析输入的参数
argParser $*
# 根据类型拆分配置文件
splitConfigFiles
# 处理并行模式运行的分类
processParallelLayers
# 处理POIBaseFeature
poiFeatureProcess
# 处理串行模式运行的分类
processSerialLayers
# 进行Layer过滤操作
processLayerFilter
exit 0
