#! /bin/bash
root=`dirname $0`
excels=`ls $root/excel`
for file in $excels
do $root/tools/excel2txt.py $file $root
done
