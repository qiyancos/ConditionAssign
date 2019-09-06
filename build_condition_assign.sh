#! /bin/bash
set -e
root=`dirname $0`
cd $root

startLine=`cat ./ConditionAssign.sh | wc -l`
startLine=$[startLine + 1]
cp ./bin/ConditionAssign.bin ./bin/ConditionAssign.bin.tmp
echo >> ./bin/ConditionAssign.bin.tmp
md5sumBinary=`md5sum ./bin/ConditionAssign.bin.tmp | awk '{print $1}'`
binaryLength=`cat ./bin/ConditionAssign.bin | wc -l`
md5sumLib=`md5sum ./test/tools/lib/libc.so.6 | awk '{print $1}'`
libLength=`cat ./test/tools/lib/libc.so.6 | wc -l`

cp ./ConditionAssign.sh ./bin/ConditionAssign
sed -i -e "s/START_LINE/$startLine/g" -e "s/MD5_BIN/$md5sumBinary/g" \
        -e "s/LENGTH_BIN/$binaryLength/g" -e "s/MD5_LIB/$md5sumLib/g" \
        -e "s/LENGTH_LIB/$libLength/g" ./bin/ConditionAssign
cat ./bin/ConditionAssign.bin.tmp >> ./bin/ConditionAssign
cat ./test/tools/lib/libc.so.6 >> ./bin/ConditionAssign
rm ./bin/ConditionAssign.bin.tmp
chmod +x ./bin/ConditionAssign
