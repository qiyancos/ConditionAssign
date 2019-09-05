#! /bin/bash
set -e
root=`dirname $0`
cd $root/..
root=$PWD
make -j8
mkdir -p ./ConditionAssign/bin
mkdir -p ./ConditionAssign/conf
mkdir -p ./ConditionAssign/data
cp ./bin/ConditionAssign ./ConditionAssign/bin/
cp ./test/tools/bin/mifdiff ./ConditionAssign/bin/
cp ./test/full_test.sh ./ConditionAssign/
$root/conf/autogen.sh
cp ./conf/*.conf ./ConditionAssign/conf
tar -cvjf ./ConditionAssign.tar.bz2 ./ConditionAssign/*
sz ./ConditionAssign.tar.bz2
rm -rf ./ConditionAssign ./ConditionAssign.tar.bz2
