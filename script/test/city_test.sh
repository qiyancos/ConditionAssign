#!/bin/bash
if [ $# -ne 4 ]
then
    echo "usage: ./run_1 citycode citypinyin cityname srcpath"
    exit 1
fi

code=$1
ename=$2
city=$3

Datasrc=$4
cd $Datasrc
Datasrc=$(pwd)
cd ..
POIModulePath=$PWD/POIProcess
cd ./mainrun

function ifErrorExit()
{
    if [ $? -ne 0 ]; then
       {
        echo "process error,exit!!!!"
        # perl /usr/local/support/bin/send_rtx.pl "henryqi,v_anpzhang,alechan" "车机运维模块异常[$city][$1]"
        # perl /usr/local/support/bin/send_weixin.pl "henryqi,v_anpzhang,alechan" "车机运维模块异常[$city][$1]"
        exit 200
        }
     else
        {
        echo "process sucess ,go  go go !!!"
        return 0
        }
      fi
}

#:<<Hkzxp

mkdir -p $Datasrc/01_basic/05_ReCatalog
ReCatalog_start=$(date +%s)
echo;echo ">> Start ReCatalog"
cd ../ReCatalog/bin
./ReCatalog $Datasrc/01_basic/02_Result  $Datasrc/01_basic/05_ReCatalog  $Datasrc/00_origin/Tencent ../conf ../log $city $code $ename
ifErrorExit "ReCatalog"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
ReCatalog_end=$(date +%s)
echo "$city - ReCatalog : $((ReCatalog_end-ReCatalog_start)) seconds " >> ./log/ReCatalog.log
echo "$city - ReCatalog : $(((ReCatalog_end-ReCatalog_start)/60)) minute " >> ./log/ReCatalog.log


mkdir -p $Datasrc/02_image/01_catalog_rank
echo;echo ">> Start NewConditionAssign(Catalog, POIProcess, LayerFilter)"
# NewConditionAssign(Catalog + POIBaseFeatureProcess + LayerFilter) ----
NewCatalog_start=$(date +%s)
cd ../NewConditionAssign
./script/auto_run.sh $POIModulePath/script/run_POIBaseFeatureProcess.sh $Datasrc/01_basic/05_ReCatalog $Datasrc/02_image/01_catalog_rank $Datasrc/00_origin/Tencent/ ./conf ./log $city 8
ifErrorExit "NewCatalog"
[ $? -eq 200 ]&&exit
cd ../mainrun/ 
NewCatalog_end=$(date +%s)
echo "$city - catalog: $((NewCatalog_end-NewCatalog_start)) seconds " >> ./log/NewCatalog.log
echo "$city - catalog : $(((NewCatalog_end-NewCatalog_start)/60)) minute " >> ./log/NewCatalog.log


mkdir -p $Datasrc/00_origin/T/01_catalog_rank
echo;echo ">> Old rank conf"
## OldRank ------------------
rankconf_start=$(date +%s)
cd ../rank/rankconf/bin
./rankconf ../conf $Datasrc/02_image/01_catalog_rank $Datasrc/00_origin/T/01_catalog_rank  ../../data_s  $city	
ifErrorExit	"RankConf"
[ $? -eq 200 ]&&exit
cd ../../../mainrun/
rankconf_end=$(date +%s)
echo "$city - rankconf : $((rankconf_end-rankconf_start)) seconds " >> ./log/rankconf.log
echo "$city - rankconf : $(((rankconf_end-rankconf_start)/60)) minute " >> ./log/rankconf.log


GenerateOverpassConfig_start=$(date +%s)
echo;echo ">> Start GenerateOverpassConfig"
cd ../GenerateOverpassConfig/bin
./GenerateOverpassConfig $Datasrc/02_image/01_catalog_rank $Datasrc/02_image/01_catalog_rank ../../rank/data_s $city 010A0C0201
ifErrorExit "GenerateOverpass"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
GenerateOverpassConfig_end=$(date +%s)
echo "$city - GenerateOverpassConfig : $((GenerateOverpassConfig_end-GenerateOverpassConfig_start)) seconds " >> ./log/GenerateOverpassConfig.log
echo "$city - GenerateOverpassConfig : $(((GenerateOverpassConfig_end-GenerateOverpassConfig_start)/60)) minute " >> ./log/GenerateOverpassConfig.log


echo;echo ">> Start Old Rank"
rank_start=$(date +%s)
cd ../rank/bin
./rank  -d  ../conf -i  $Datasrc/02_image/01_catalog_rank -o  $Datasrc/02_image/01_catalog_rank -s ../data_s -c $city -l ../log
ifErrorExit "Rank"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
rank_end=$(date +%s)
echo "$city - rank : $((rank_end-rank_start)) seconds " >> ./log/rank.log
echo "$city - rank : $(((rank_end-rank_start)/60)) minute " >> ./log/rank.log
# ---------------------


echo;echo ">> Start Name Process"
NamePro_start=$(date +%s)
cd ../NamePro/bin
./NamePro $Datasrc/02_image/01_catalog_rank $Datasrc/02_image/01_catalog_rank ../conf ../log $city
ifErrorExit "NamePro"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
NamePro_end=$(date +%s)
echo "$city - NamePro : $((NamePro_end-NamePro_start)) seconds " >> ./log/NamePro.log
echo "$city - NamePro : $(((NamePro_end-NamePro_start)/60)) minute " >> ./log/NamePro.log


echo;echo ">> Start New Rank"
## NewRank
# --------------------
NewRank_start=$(date +%s)
sh $POIModulePath/script/run_RankTask.sh $Datasrc/02_image/01_catalog_rank/$city $Datasrc/02_image/01_catalog_rank/$city $city 5
ifErrorExit "NewRank"
[ $? -eq 200 ]&&exit
NewRank_end=$(date +%s)
echo "$city - rank : $((NewRank_end-NewRank_start)) seconds " >> ./log/NewRank.log
echo "$city - rank : $(((NewRank_end-NewRank_start)/60)) minute " >> ./log/NewRank.log
# --------------------

echo;echo ">> Start SmoothLine"
SmoothLine_start=$(date +%s)
cd ../SmoothLine
./SmoothLine ../data/02_image/01_catalog_rank/$city/C_R ../data/02_image/01_catalog_rank/$city/C_Z snakes 1 1
ifErrorExit "SmoothLine"
[ $? -eq 200 ]&&exit
cd ../mainrun/
SmoothLine_end=$(date +%s)
echo "$city - SmoothLine : $((SmoothLine_end-SmoothLine_start)) seconds " >> ./log/SmoothLine.log
echo "$city - SmoothLine : $(((SmoothLine_end-SmoothLine_start)/60)) minute " >> ./log/SmoothLine.log
