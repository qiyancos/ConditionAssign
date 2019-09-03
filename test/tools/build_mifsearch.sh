#! /bin/bash
set -e
root=`dirname $0`
cd $root

startLine=`cat ./mifsearch.sh | wc -l`
startLine=$[startLine + 1]
cp ./bin/mifsearch.bin ./bin/mifsearch.bin.tmp
echo >> ./bin/mifsearch.bin.tmp
md5sumBinary=`md5sum ./bin/mifsearch.bin.tmp | awk '{print $1}'`
binaryLength=`cat ./bin/mifsearch.bin | wc -l`
md5sumLib=`md5sum ./lib/libc.so.6 | awk '{print $1}'`
libLength=`cat ./lib/libc.so.6 | wc -l`

cp ./mifsearch.sh ./bin/mifsearch
sed -i -e "s/START_LINE/$startLine/g" -e "s/MD5_BIN/$md5sumBinary/g" \
        -e "s/LENGTH_BIN/$binaryLength/g" -e "s/MD5_LIB/$md5sumLib/g" \
        -e "s/LENGTH_LIB/$libLength/g" ./bin/mifsearch
cat ./bin/mifsearch.bin.tmp >> ./bin/mifsearch
cat ./lib/libc.so.6 >> ./bin/mifsearch
rm ./bin/mifsearch.bin.tmp
chmod +x ./bin/mifsearch
