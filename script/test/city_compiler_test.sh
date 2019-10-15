#!/bin/bash
cd `dirname $0`/..
root=$PWD

Version=$1
if [ $# -ne 1 ]; then
    echo "sample: nohup sh mainrun.sh week_20170601_M06W1 &"
    exit -1
fi
dataVersionlist=$root/version/dataVersion
dataVersionold=`sed -n "2,2p" $dataVersionlist`
if [[ $dataVersionold != $Version ]]
then
sed -i "1s:.*:$dataVersionold:g" $dataVersionlist
sed -i "2s:.*:$Version:g" $dataVersionlist
fi
date_start=$(date)
sms_start=$(date +%s)

cd $root/mainrun
echo "start process base_pro"
echo "start all "date "+%Y-%m-%d-%H:%M:%S" >> ./log/compiletime.log
base_pro_start=$(date +%s)
cd ../base_pro/bin
sh run.sh
echo "end base_pro"  
cd ../../mainrun/
base_pro_end=$(date +%s)
echo "base_pro : $((base_pro_end-base_pro_start)) seconds " >> ./log/base_pro.log
echo "base_pro : $(((base_pro_end-base_pro_start)/60)) minute " >> ./log/base_pro.log

## -- SplitPOI --
splitpoi_start=$(date +%s)
echo "start SplitPOI"
sh run_splitpoi.sh
echo "end SplitPOI"
splitpoi_end=$(date +%s)
echo "splitpoi : $((splitpoi_end-splitpoi_start)) seconds " >> ./log/splitpoi.log
echo "splitpoi : $(((splitpoi_end-splitpoi_start)/60)) minute " >> ./log/splitpoi.log
## ------

## -- ExtractBusinessArea --
extract_business_start=$(date +%s)
echo "start SplitPOI"
sh run_extract_business.sh
echo "end SplitPOI"
extract_business_end=$(date +%s)
echo "extract_business : $((extract_business_end-extract_business_start)) seconds " >> ./log/extract_business.log
echo "extract_business : $(((extract_business_end-extract_business_start)/60)) minute " >> ./log/extract_business.log
## ------

imageprocess_start=$(date +%s)
echo "start imageprocess" 
cat citylist | xargs -n3 -P15 sh -c 'sh run_1.sh "$0" "$1" "$2"'
echo "end process imageprocess">> log
imageprocess_end=$(date +%s)
echo "imageprocess : $((imageprocess_end-imageprocess_start)) seconds " >> ./log/imageprocess.log
echo "imageprocess : $(((imageprocess_end-imageprocess_start)/60)) minute " >> ./log/imageprocess.log

cd ../datacompiler
sh run.sh ../mainrun/citylist &
echo "end datacompiler new" 

echo "compile china data"
cd ../mainrun
sh run_china.sh &
wait
cd ../version
./xp_version.sh ../data/02_image/07_compiler_no_rtic_new dat ./CarVersionNumber

sms_end=$(date +%s)
date_end=$(date)
xiaoshi=$(echo "scale=2;($sms_end-$sms_start)/60.0/60.0" | bc)
echo $xiaoshi
cd ../Diff
./compare.sh
perl /usr/local/support/bin/send_sms.pl "v_anpzhang" "217车机运维开始时间:$date_start  运维结束时间:$date_end  运维整体时间:$(((sms_end-sms_start)/60))分钟 - 共计: $xiaoshi 小时"
perl /usr/local/support/bin/send_mail.pl "$message" "217服务器:$Version版本数据<br>车机底图运维开始时间:$date_start<br>运维结束时间:$date_end<br>运维整体时间:$(((sms_end-sms_start)/60))分钟<br>共计: $xiaoshi 小时" "车机底图:$Version数据运维已结束"
echo "end all process!!!!!" 
echo "end all "date "+%Y-%m-%d-%H:%M:%S" >> ./log/compiletime.log
exit 0 

