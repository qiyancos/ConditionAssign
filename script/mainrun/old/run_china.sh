#!/bin/bash
city=china
code=0
Datasrc=/data2/dataprocess/data
function ifErrorExit() {
    if [ $? -ne 0 ]; then
    {
        echo "process error,exit!!!!"
        exit 200
    }
    else
    {
        echo "process sucess ,go  go go !!!"
        return 0
    }
    fi
}

run_china_start=$(date +%s)
echo "start china process dataprocess" 
#cp china
cp -rp ../china $Datasrc/01_basic/02_Result/

cd ../ReCatalog/bin
echo "process 01basic/05Recatalog!"	
./ReCatalog $Datasrc/01_basic/02_Result  $Datasrc/01_basic/05_ReCatalog  $Datasrc/00_origin/Tencent ../conf ../log $city 0 china
ifErrorExit    
[ $? -eq 200 ]&&exit
cd ../../mainrun/		

cd  ../NewConditionAssign
echo "process 02_image/01_catalog!"	
./script/auto_run.sh NULL $Datasrc/01_basic/05_ReCatalog $Datasrc/02_image/01_catalog_rank $Datasrc/00_origin/Tencent ./conf ./log $city
ifErrorExit 
[ $? -eq 200 ]&&exit
cd ../../mainrun/

cd  ../rank/bin
echo "process 02image/01rank!"	
./rank  -d  ../conf -i  $Datasrc/02_image/01_catalog_rank -o  $Datasrc/02_image/01_catalog_rank -s ../data_s -c $city -l ../log
ifErrorExit 
[ $? -eq 200 ]&&exit
cd ../../mainrun/

cd ../datapro/bin
echo "process 02image/02datapro!"	
./datapro $Datasrc/02_image/01_catalog_rank  $Datasrc/02_image/02_datapro ../conf/datapro.conf ../conf/KindPriority.conf  ../conf/dictdata ../log $city 
ifErrorExit	
[ $? -eq 200 ]&&exit		
cd ../../mainrun/

#osm taiwan
cd ../Convertosm/bin
echo "process osm taiwan!" 
sh run.sh
ifErrorExit
[ $? -eq 200 ]&&exit
cd ../../mainrun/

    cp -p citylist ../chinaroadpro/conf/	
cd ../chinaroadpro/bin
echo "process 02image/02datapro+chinaroadpro!"	
./chinaroadpro $Datasrc/02_image/02_datapro $Datasrc/02_image/02_datapro ../conf ../log $city 128
ifErrorExit	
[ $? -eq 200 ]&&exit		
cd ../../mainrun

cd ../maplabel/bin
rm -r $Datasrc/02_image/03_label/$city
cp -r $Datasrc/02_image/02_datapro/$city  $Datasrc/02_image/03_label/
./maplabel $Datasrc/02_image/03_label $Datasrc/02_image/03_label ../conf/style.2d ../log  $city $code 4-10 0
ifErrorExit
[ $? -eq 200 ]&&exit
cd ../../mainrun

cd ../datacompiler/
sh run_china.sh
run_china_end=$(date +%s)
echo "run_china : $((run_china_end-run_china_start)) seconds " >> ../mainrun/log/run_china.log
echo "run_china : $(((run_china_end-run_china_start)/60)) minute " >> ../mainrun/log/run_china.log
echo "end process !"
exit 0;
