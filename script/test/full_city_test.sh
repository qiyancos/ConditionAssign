#! /bin/bash
root=`dirname $0`
cd $root/../..
root=$PWD

if [ $# -lt 2 ]
then
	echo "$0 cityname datapath [binpath]"
	exit -1
fi

cityName=$1
dataPath=$2
if [ ! -d $2/01_basic/05_ReCatalog/$1 ]
then
	echo "Error: No such file or directory \"$2/01_basic/05_ReCatalog/$1\""
	exit -1
fi

if [ x$3 = x ]
then
	echo "Warn: Use data path as binary path."
	binPath=$2
else binPath=$3
fi

if [ ! -f $binPath/mainrun/mainrun.sh ]
then
	echo "Error: Can not find mainrun.sh in binary path \"$binPath\""
	exit -1
fi

quickCopy() {
	target="$2/`basename $1`"
	mkdir -p $target
	for file in $3
	do
		if [ -f $1/$file -o -d $1/$file ]
		then cp -r $1/$file $target/
		fi
	done
	for file in $4
	do mkdir -p $target/$file
	done
}

set -e
rm -rf $root/dataprocess
echo ">> Start init test environment"
echo "-- Copying test data..."
mkdir -p $root/dataprocess/mainrun/log
mkdir -p $root/dataprocess/data/01_basic/02_Result
ln -sf $dataPath/01_basic/02_Result/$cityName \
		$root/dataprocess/data/01_basic/02_Result/
mkdir -p $root/dataprocess/data/00_origin
ln -sf $dataPath/00_origin/* $root/dataprocess/data/00_origin/
mkdir -p $root/dataprocess/data/02_image/03_label_shentu/$cityName

cpList="bin conf script script.full"
mkdirList="log log.full"
echo "-- Copying test binary files..."
echo "-- Copying ReCatalog..."
quickCopy $binPath/ReCatalog $root/dataprocess "$cpList" "$mkdirList"
echo "-- Copying NewCA..."
quickCopy $root $root/dataprocess "$cpList" "$mkdirList"
echo "-- Copying rank..."
mkdir $root/dataprocess/rank
quickCopy $binPath/rank  $root/dataprocess "$cpList rankconf" \
		"$mkdirList data_s data_s.full"
echo "-- Copying GenerateOverpassConfig..."
quickCopy $binPath/GenerateOverpassConfig $root/dataprocess/ "$cpList" "$mkdirList"
echo "-- Copying NamePro..."
quickCopy $binPath/NamePro $root/dataprocess/ "$cpList" "$mkdirList"
echo "-- Copying POIProcess..."
mv $binPath/POIProcess/bin/Learning2Rank $binPath/temp_cp
quickCopy $binPath/POIProcess $root/dataprocess/ "init.sh $cpList" "$mkdirList"
sed -i '/MLRank/,$d' $root/dataprocess/POIProcess/script/run_RankTask.sh
mv $binPath/temp_cp $binPath/POIProcess/bin/Learning2Rank
echo "-- Copying SmoothLine..."
cp -r $binPath/SmoothLine $root/dataprocess/
echo "-- Copying datapro..."
quickCopy $binPath/datapro $root/dataprocess/ "$cpList" "$mkdirList"
echo "-- Copying maplabel..."
quickCopy $binPath/maplabel $root/dataprocess/ "$cpList conf.china" "$mkdirList"
echo "-- Copying CartoonAreaFilter..."
quickCopy $binPath/CartoonAreaFilter $root/dataprocess/ "$cpList" "$mkdirList"
echo "-- Copying FilterIndoorPOI..."
quickCopy $binPath/FilterIndoorPOI $root/dataprocess/ "$cpList" "$mkdirList"
echo "-- Copying datacompiler..."
quickCopy $binPath/datacompiler $root/dataprocess/ "$cpList conf.new" \
		"$mkdirList log.new"
cp $binPath/datacompiler/*sh $binPath/datacompiler/*py \
		$binPath/datacompiler/*all $binPath/datacompiler/*patch \
		$root/dataprocess/datacompiler
echo "-- Environment initiated."

echo ">> Start city process test."
arg=`grep $cityName $binPath/mainrun/citylist`
$root/script/test/city_test.sh $arg $root/dataprocess/data
