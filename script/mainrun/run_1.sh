#!/bin/bash
if [ $# -ne 3 ]; then
	  echo "usage: ./run_1 citycode citypinyin cityname"
	  exit 1
fi
code=$1
ename=$2
city=$3

cd ../data
Datasrc=$(pwd)
cd ../mainrun

RootPath=$(cd `dirname $0`/..; pwd)
POIModulePath=$RootPath/POIProcess

function ifErrorExit()
{
    if [ $? -ne 0 ]; then
       {
        echo "process error,exit!!!!"
        perl /usr/local/support/bin/send_rtx.pl "henryqi,v_anpzhang,alechan" "车机运维模块异常[$city][$1]"
        perl /usr/local/support/bin/send_weixin.pl "henryqi,v_anpzhang,alechan" "车机运维模块异常[$city][$1]"
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

ExtractPOI_start=$(date +%s)
cd ../ExtractPOI/bin
./ExtractPOI $Datasrc/01_basic/02_Result/$city/poi.json $Datasrc/01_basic/02_Result/$city  ../conf ../log 5
ifErrorExit "ExtractPOI"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
ExtractPOI_end=$(date +%s)
echo "$city - ExtractPOI : $((ExtractPOI_end-ExtractPOI_start)) seconds " >> ./log/ExtractPOI.log
echo "$city - ExtractPOI : $(((ExtractPOI_end-ExtractPOI_start)/60)) minute " >> ./log/ExtractPOI.log

ExtractAOI_start=$(date +%s)
cd ../ExtractAOI/bin
./ExtractAOI $Datasrc/01_basic/02_Result/$city $Datasrc/../RegionProcess/data/$city $Datasrc/01_basic/02_Result/$city ../conf ../log 5
ifErrorExit "ExtractAOI"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
ExtractAOI_end=$(date +%s)
echo "$city - ExtractAOI : $((ExtractAOI_end-ExtractAOI_start)) seconds " >> ./log/ExtractAOI.log
echo "$city - ExtractAOI : $(((ExtractAOI_end-ExtractAOI_start)/60)) minute " >> ./log/ExtractAOI.log

ReCatalog_start=$(date +%s)
cd ../ReCatalog/bin
./ReCatalog $Datasrc/01_basic/02_Result  $Datasrc/01_basic/05_ReCatalog  $Datasrc/00_origin/Tencent ../conf ../log $city $code $ename
ifErrorExit "ReCatalog"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
ReCatalog_end=$(date +%s)
echo "$city - ReCatalog : $((ReCatalog_end-ReCatalog_start)) seconds " >> ./log/ReCatalog.log
echo "$city - ReCatalog : $(((ReCatalog_end-ReCatalog_start)/60)) minute " >> ./log/ReCatalog.log

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
cd ../GenerateOverpassConfig/bin
./GenerateOverpassConfig $Datasrc/02_image/01_catalog_rank $Datasrc/02_image/01_catalog_rank ../../rank/data_s $city 010A0C0201
ifErrorExit "GenerateOverpass"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
GenerateOverpassConfig_end=$(date +%s)
echo "$city - GenerateOverpassConfig : $((GenerateOverpassConfig_end-GenerateOverpassConfig_start)) seconds " >> ./log/GenerateOverpassConfig.log
echo "$city - GenerateOverpassConfig : $(((GenerateOverpassConfig_end-GenerateOverpassConfig_start)/60)) minute " >> ./log/GenerateOverpassConfig.log

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

NamePro_start=$(date +%s)
cd ../NamePro/bin
./NamePro $Datasrc/02_image/01_catalog_rank $Datasrc/02_image/01_catalog_rank ../conf ../log $city
ifErrorExit "NamePro"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
NamePro_end=$(date +%s)
echo "$city - NamePro : $((NamePro_end-NamePro_start)) seconds " >> ./log/NamePro.log
echo "$city - NamePro : $(((NamePro_end-NamePro_start)/60)) minute " >> ./log/NamePro.log


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


SmoothLine_start=$(date +%s)
cd ../SmoothLine
./SmoothLine ../data/02_image/01_catalog_rank/$city/C_R ../data/02_image/01_catalog_rank/$city/C_Z snakes 1 1
ifErrorExit "SmoothLine"
[ $? -eq 200 ]&&exit
cd ../mainrun/
SmoothLine_end=$(date +%s)
echo "$city - SmoothLine : $((SmoothLine_end-SmoothLine_start)) seconds " >> ./log/SmoothLine.log
echo "$city - SmoothLine : $(((SmoothLine_end-SmoothLine_start)/60)) minute " >> ./log/SmoothLine.log

datapro_start=$(date +%s)
cd ../datapro/bin
./datapro $Datasrc/02_image/01_catalog_rank  $Datasrc/02_image/02_datapro ../conf/datapro.conf ../conf/KindPriority.conf  ../conf/dictdata ../log $city 
ifErrorExit "Datapro"
[ $? -eq 200 ]&&exit
#shentu city
cp -r $Datasrc/02_image/03_label_shentu/$city/* $Datasrc/02_image/02_datapro/$city/
cd ../../mainrun/
datapro_end=$(date +%s)
echo "$city - datapro : $((datapro_end-datapro_start)) seconds " >> ./log/datapro.log
echo "$city - datapro : $(((datapro_end-datapro_start)/60)) minute " >> ./log/datapro.log


## AOIProcess
# --------------------
AOIProcess_start=$(date +%s)
sh $POIModulePath/script/run_AOIProcess.sh $Datasrc/02_image/01_catalog_rank/$city $Datasrc/02_image/02_datapro/$city $city 5
ifErrorExit "AOIProcess"
[ $? -eq 200 ]&&exit
AOIProcess_end=$(date +%s)
echo "$city - rank : $((AOIProcess_end-AOIProcess_start)) seconds " >> ./log/AOIProcess.log
echo "$city - rank : $(((AOIProcess_end-AOIProcess_start)/60)) minute " >> ./log/AOIProcess.log
# --------------------


## WhiteTask
# --------------------
WhiteTask_start=$(date +%s)
sh $POIModulePath/script/run_WhiteTask.sh $Datasrc/02_image/02_datapro/$city $Datasrc/02_image/02_datapro/$city $city 5
ifErrorExit "WhiteTask"
[ $? -eq 200 ]&&exit
WhiteTask_end=$(date +%s)
echo "$city - rank : $((WhiteTask_end-WhiteTask_start)) seconds " >> ./log/WhiteTask.log
echo "$city - rank : $(((WhiteTask_end-WhiteTask_start)/60)) minute " >> ./log/WhiteTask.log
# --------------------


#Hkzxp
maplabel_start=$(date +%s)
cd ../maplabel/bin
rm -rf $Datasrc/02_image/03_label/$city
cp -r $Datasrc/02_image/02_datapro/$city  $Datasrc/02_image/03_label/
./maplabel $Datasrc/02_image/03_label $Datasrc/02_image/03_label ../conf/style.2d ../log  $city $code  11-20 0
ifErrorExit	"Maplabel"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
maplabel_end=$(date +%s)
echo "$city - maplabel : $((maplabel_end-maplabel_start)) seconds " >> ./log/maplabel.log
echo "$city - maplabel : $(((maplabel_end-maplabel_start)/60)) minute " >> ./log/maplabel.log

FilterIndoorPOI_start=$(date +%s)
cd ../FilterIndoorPOI/bin
./FilterIndoorPOI $Datasrc/02_image/03_label $Datasrc/00_origin/Tencent/indoordata $Datasrc/02_image/03_label ../conf $city $code 17-20
ifErrorExit "FilterIndoor"
[ $? -eq 200 ]&&exit
cd ../../mainrun/
FilterIndoorPOI_end=$(date +%s)
echo "$city - FilterIndoorPOI : $((FilterIndoorPOI_end-FilterIndoorPOI_start)) seconds " >> ../../mainrun/log/FilterIndoorPOI.log
echo "$city - FilterIndoorPOI : $(((FilterIndoorPOI_end-FilterIndoorPOI_start)/60)) minute " >> ../../mainrun/log/FilterIndoorPOI.log

