#! /bin/bash
set -e

initTest() {
    root=`dirname $0`
    cd $root
    root=$PWD
    # we will only use half the cpu sources to run test
    executorCnt=`lscpu | awk '/^CPU\(s\):/{print $2}'`
    executorCnt=$[executorCnt / 2]

    if [ x$1 = x -o x$1 = "x-h" ]
    then
        echo "Usage: $0 [OPTION] <Data Path> <City Name>"
        echo "    -s, --single running single test"
        exit 0
    else
        if [ x$1 = -sx -o $1x = --singlex ]
        then
            runSingle=1
            dataPath=$2
        else dataPath=$1
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
   
    # TODO
    srcDataPath=$dataPath/
    resultDataPath=$dataPath/
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
    else cities=(`ls $srcDataPath | sed "s/china//g"`)
    fi
    layers=(`ls $root/conf | sed "s/\.conf//g"`)
    mkdir -p $root/log
    mkdir -p $root/data
}

singleTest() {
    cityName=$1
    layerName=$2
    echo ">> Processing with layer $layerName in city $cityName..."
    srcLayer="$srcDataPath/$cityName/${layerName}.mif"
    targetLayer="$root/data/${layerName}.mif"
    configFile="$root/conf/${layerName}.conf"
    logPath="$root/log/$cityName"
    echo -n "Command: $root/bin/ConditionsAssign NULL NULL $srcLayer "
    echo "$targetLayer $executorCnt $logPath $configFile NULL"
    totalTime=`(time $root/bin/ConditionAssign NULL NULL $srcLayer \
            $targetLayer $executorCnt $logPath $configFile NULL) \
            2>&1 | awk '/real/ {print $2}'`
    echo ">> Checking Result of layer $layerName in city $cityName..."
    $root/bin/mifdiff $resultDataPath/$cityName/${layerName}.mif $targetLayer
    resultMatch=$?
    if [ $resultMatch = -1 ]
    then echo "Test Result [$city-$layerName]: Not Match"
    else
        echo "Test Time [$city-$layerName]: $totalTime"
        echo "Test Result [$city-$layerName]: Match"
    fi
    echo
}

initTest
if [ x$runSingle = x1 ]
then
    echo "Please select the layer you want to test (C_POI is not available)."
    select layer in $layers
    do
        if [ -f $srcDataPath/$cities/$layer.mif ]
        then singleTest $cities $layer
        else echo ">> Layer $layer not exists in city $cities"
        fi
        break
    done
else
    for city in $cities
    do
        for layer in $layers
        do
            if [ -f $srcDataPath/$cityName/$layer.mif ]
            then singleTest $city $layer
            else echo ">> Layer $layer not exists in city $city"
            fi
        done
    done
fi
