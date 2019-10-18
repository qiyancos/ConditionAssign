#! /bin/bash
root=`dirname $0`
rm -rf $root/*.conf
excels=`ls $root/excel`
for file in $excels
do $root/tools/excel2txt.py $root/excel/$file $root
done
