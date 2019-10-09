#! /bin/bash
set -e
root=`dirname $0`
cd $root/..
root=$PWD
make -j8
cd $root/RoadCatalog
make -j8
cd $root
mkdir -p ./NewConditionAssign/bin
mkdir -p ./NewConditionAssign/conf
mkdir -p ./NewConditionAssign/data
cp ./bin/ConditionAssign ./NewConditionAssign/bin/
cp ./test/tools/bin/mifdiff ./NewConditionAssign/bin/
cp ./test/tools/bin/mifsearch ./NewConditionAssign/bin/
cp ./RoadCatalog/bin/RoadCatalog ./NewConditionAssign/bin/
$root/conf/autogen.sh
cp ./conf/*.conf ./NewConditionAssign/conf
cp -r ./script ./NewConditionAssign/
rm -rf ./NewConditionAssign/script/mainrun
tar -cvjf ./NewConditionAssign.tar.bz2 ./NewConditionAssign/*
sz ./NewConditionAssign.tar.bz2
rm -rf ./NewConditionAssign ./NewConditionAssign.tar.bz2
