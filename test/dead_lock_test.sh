#! /bin/bash
root=`dirname $0`

while [ 1 ]
do
    $root/auto_test.sh $1
done
