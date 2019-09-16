#! /bin/bash

initTest() {
    root=`dirname $0`
    cd $root/..
    root=$PWD
    if [ ! -f /tmp/lib_temp/libc.so.6 ]
    then $root/bin/mifsearch 2>&1 > /dev/null
    fi
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/tmp/lib_temp
    # we will only use half the cpu sources to run test
    executorCnt=`lscpu | awk '/^CPU\(s\):/{print $2}'`
    executorCnt=$[executorCnt / 2]

    if [ x$1 = x -o x$1 = "x-h" ]
    then
        echo "Usage: $0 [OPTION] <Data Path> <City Name>"
        echo "    -s, --single running single test"
        echo "    -c, --continue running after met the given city"
        exit 0
    else
        if [ x$1 = "x-s" -o x$1 = "x--single" ]
        then
            runSingle=1
            dataPath=$2
        else dataPath=$1
        fi
        if [ x$1 = "x-c" -o x$1 = "x--continue" ]
        then
            runCheckPoint=1
            dataPath=$2
        fi
        if [ x$dataPath = x ]
        then
            echo "Error: Data path is not given."
            exit 1
        elif [ ! -d $dataPath ]
        then
            echo "Error: No such directory \"$1\"."
            exit 1
        fi
    fi
   
    srcDataPath=$dataPath/01_basic/05_ReCatalog
    resultDataPath=$dataPath/02_image/01_catalog_rank
    rootDataPath=`dirname $dataPath`
    if [ x$runSingle = x1 ]
    then
        if [ x$3 = x ]
        then
            echo "Error: Cityname must be provided in single-test mode."
            exit 1
        elif [ ! -d $srcDataPath/$3 ]
        then
            echo "Error: No such directory \"$srcDataPath/$3\"."
            exit 1
        fi
        cities=$3
    else cities=`ls $srcDataPath`
    fi
    if [ x$runCheckPoint = x1 ]
    then startCity=$3
    fi
    mkdir -p $root/log
    mkdir -p $root/data
    if [ -f $root/skip_list.txt ]
    then skipList=$root/skip_list.txt
    fi
	layer=C_POI
}

poiTest() {
    cityName=$1
    layerName=$2
    echo ">> Processing with layer $layerName in city $cityName..."
    # Run POIFeature
    srcLayer="$srcDataPath/$cityName/${layerName}.mif"
    targetLayer="$root/data/$cityName/${layerName}.mif"
    cp $srcLayer $targetLayer
    cp $srcDataPath/$cityName/${layerName}.mid $root/data/$cityName/${layerName}.mid
    cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mid \
            $root/data/$cityName/C_AOI_FULL.mid
    cp -frap $dataPath/01_basic/02_Result/$cityName/ALL_Name_Area.mif \
            $root/data/$cityName/C_AOI_FULL.mif
    sh $rootDataPath/POIProcess/script/run_POIBaseFeatureProcess.sh \
            $root/data/$cityName $root/data/$cityName $cityName 5
    if [ $? != 0 ]
    then
        echo "Error: Failed to run POIFeatureProcess for $city."
        exit 1
    fi
    # Run ConditionAssign
    srcLayer=$targetLayer
    configFile="$root/conf/${layerName}_1.conf"
    configFile="$configFile;$root/conf/${layerName}_2.conf"
    configFile="$configFile;$root/conf/${layerName}_3.conf"
    configFile="$configFile;$root/conf/${layerName}_4.conf"
    configFile="$configFile;$root/conf/${layerName}_5.conf"
    configFile="$configFile;$root/conf/${layerName}_6.conf"
    logPath="$root/log/$cityName"
    mkdir -p $root/data/$cityName
    mkdir -p $logPath
    echo -n "Command: $root/bin/ConditionAssign NULL NULL $srcLayer "
    echo "$targetLayer $executorCnt $logPath $configFile NULL"
    totalTime=`(time $root/bin/ConditionAssign NULL NULL $srcLayer \
            $targetLayer $executorCnt $logPath $configFile NULL) \
            2>&1 | awk '/real/ {print $2}'`
    echo;echo ">> Checking Result of layer $layerName in city $cityName..."
    $root/bin/mifdiff $resultDataPath/$cityName/${layerName}.mif $targetLayer
    resultMatch=$?
    if [ $resultMatch = -1 ]
    then
        echo "Test Result [$city-$layerName]: Not Match"
        exit 1
    else
        echo "Test Time [$city-$layerName]: $totalTime"
        echo "Test Result [$city-$layerName]: Match"
    fi
    echo
}

checkSkip() {
    if [ x$skipList = x ]
    then echo 0
    elif [ "x`grep $2 $skipList`" = x$2 ]
    then echo 1
    elif [ "x`grep $1 $skipList | grep $2`" != x ]
    then echo 1
    else echo 0
    fi
}

initTest $*
if [ x$runSingle = x1 ]
then
    echo "Please select the layer you want to test (C_POI is not available)."
    if [ `checkSkip $cities $layer` = 0 ]
    then
        if [ -f $srcDataPath/$cities/$layer.mif ]
        then
            set -e
            poiTest $cities $layer
            set +e
        else echo ">> Layer $layer not exists in city $cities"
        fi
    else echo ">> Skip layer $layer in city $cities"
    fi
else
    startRun=0
    for city in $cities
    do
        if [ x$startCity = x -o x$city = x$startCity ]
        then startRun=1
        fi
        if [ $startRun = 1 ]
        then
            if [ `checkSkip $city $layer` = 0 ]
            then
                if [ -f $srcDataPath/$city/$layer.mif ]
                then
                    set -e
                    poiTest $city $layer
                    set +e
                else echo ">> Layer $layer not exists in city $city"
                fi
            else echo ">> Skip layer $layer in city $city"
            fi
       	fi
   	done
fi
