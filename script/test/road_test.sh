#! /bin/bash
set -e
root=`dirname $0`
cd $root/../..
root=$PWD

srcDir="/data6/wenyongwang/dataprocess/data"
cityList=`ls $srcDir/01_basic/05_ReCatalog/`
for cityName in $cityList
do
	if [ -f $srcDir/01_basic/05_ReCatalog/$cityName/C_R.mif ]
	then
		mkdir -p $root/log/$cityName
		mkdir -p $root/data/$cityName
		echo ">> Processing City: $cityName..."
		echo "Command: $root/bin/RoadCatalog $srcDir/01_basic/05_ReCatalog/ $root/data/ $srcDir/00_origin/Tencent/ $root/conf/ $root/log/$cityName/ $cityName"
		$root/bin/RoadCatalog $srcDir/01_basic/05_ReCatalog/ \
				$root/data/ $srcDir/00_origin/Tencent/ \
				$root/conf/ $root/log/$cityName/ $cityName 
		echo
    	echo ">> Checking Result of layer C_R in city $cityName..."
    	$root/bin/middiff $srcDir/02_image/01_catalog_rank/$cityName/C_R.mif \
				$root/data/$cityName/C_R.mif
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
	fi
done
