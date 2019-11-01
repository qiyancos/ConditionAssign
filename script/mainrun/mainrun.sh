#!/bin/bash
source ~/.bash_profile
message="v_xpzzhang,alicepang,henryqi,alechan,v_yanlsong,v_anpzhang,kobefwang,devzhang,v_ltwu"
date_start=$(date)
sms_start=$(date +%s)
rttdata=/data1/deploy/
rttdata1=/data2/dataprocess/data/00_origin/NavInfo/RTIC/
dataVersionlist=/data8/version/dataVersion
dataVersionold=`sed -n "1,1p" $dataVersionlist`
dataVersionnew=`sed -n "2,2p" $dataVersionlist`
ip=$(ip addr |grep inet |grep -v inet6 |grep eth1|awk '{print $2}' |awk -F "/" '{print $1}')
#rm ../*/log/*
echo "start process base_pro" >> log
date "+%Y-%m-%d-%H:%M:%S" >> log
echo "start process base_pro"
echo "start all "date "+%Y-%m-%d-%H:%M:%S" >> ./log/compiletime.log


#perl /usr/local/support/bin/send_mail.pl "$message" "$ip底图数据运维【开始】"
## -- basepro --
base_pro_start=$(date +%s)
# 充电桩数据处理
cd /data2/dataprocess/ChargerProcess/
nohup sh run.sh &
cd -
cd ../base_pro/bin
sh run.sh
[ $? -ne 0 ]&&exit
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
:<<Hkzxp
yunweiguo=0
function rttyunwei()
{

if [[ -d $rttdata/RTIC_$dataVersionnew && -d $rttdata/china_rtic_$dataVersionnew ]]
then
	perl /usr/local/support/bin/send_sms.pl "v_xpzzhang" "当前版本路况数据存在已进行拷贝至运维环境！"
	cp -rp $rttdata/RTIC_$dataVersionnew/* $rttdata1
	cp -rp $rttdata/china_rtic_$dataVersionnew/* $rttdata1

	cd Check
	sh check.sh rtic.conf rtic.log

	if [ $? -eq 0 ]; then
		rticpro_start=$(date +%s)
		cd ../
		cp -r citylist ../rticpro/conf	
		cd ../rticpro
		sh rticpro.sh
		rticpro_end=$(date +%s)
		echo "rticpro : $((rticpro_end-rticpro_start)) seconds " >> ../mainrun/log/rticpro.log
		echo "rticpro : $(((rticpro_end-rticpro_start)/60)) minute " >> ../mainrun/log/rticpro.log
		echo "process link rtic"
		echo "success"
		cd ../link_traffic_proc_Ex
		sh run.sh &
		yunweiguo=1
		echo "end link rtic"
	else
		cd ../
	fi
else
	perl /usr/local/support/bin/send_sms.pl "v_xpzzhang" "当前版本路况数据不存在跳过路况运维！"

fi
}
Hkzxp
cd ../mainrun
sh run_china.sh &
#rttyunwei
sh rtt_mainrun.sh &
cd ../datacompiler
sh run.sh ../mainrun/citylist
wait
:<<Hkzxp
if [ $yunweiguo -eq 0 ] 
then
cd ../mainrun
rttyunwei
wait
fi
Hkzxp
cd ../mainrun
#./dataOrganize.sh
cd ../datacompiler
nohup sh run_closedroad.sh ../mainrun/citylist &
cd /data2/poicompile
nohup sh run.sh &

# 充电桩数据处理
#cd /data2/dataprocess/ChargerProcess/
#nohup sh run.sh &
#cd -

wait

sms_end=$(date +%s)
date_end=$(date)
xiaoshi=$(echo "scale=2;($sms_end-$sms_start)/60.0/60.0" | bc)
poiversion=`cat /data2/dataprocess/SyncPOI/version`
perl /usr/local/support/bin/send_mail.pl "$message" "$ip底图数据运维【完成】"
perl /usr/local/support/bin/send_sms.pl "v_xpzzhang" "207服务器:$dataVersionnew版本	手图底图运维开始时间:$date_start	运维结束时间:$date_end	运维整体时间:$(((sms_end-sms_start)/60))分钟	共计: $xiaoshi 小时"
perl /usr/local/support/bin/send_mail.pl "$message" "207服务器:$dataVersionnew版本数据<br>POI数据版本:$poiversion<br>手图底图运维开始时间:$date_start<br>运维结束时间:$date_end<br>运维整体时间:$(((sms_end-sms_start)/60))分钟<br>共计: $xiaoshi小时" "手图底图:$dataVersionnew数据运维已结束"
python /data8/xpzhang/data/Robot.py "207服务器:$dataVersionnew版本数据 POI数据版本:$poiversion 手图底图运维开始时间:$date_start 运维结束时间:$date_end 运维整体时间:$(((sms_end-sms_start)/60))分钟 共计: $xiaoshi小时 运维完毕！" 


#灵犀上报状态
dataall=`cat ../data_version`
python lingxiAPISample.py ${dataall} 1
date "+%Y-%m-%d-%H:%M:%S"
echo "end all process!!!!!" 
echo "end all "date "+%Y-%m-%d-%H:%M:%S" >> ./log/compiletime.log
cd ../dataprocess/mainrun/
#./upgriddata.sh &
exit 0 

