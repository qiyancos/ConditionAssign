#! /bin/bash
set -e
# 获取当前执行文件的路径
tempDir=/tmp
root=`dirname $0`
pushd $root
root=$PWD
thisFile=$root/`basename $0`
popd

# 合并文件相关信息
startLine="START_LINE"
md5sumBinary="MD5_BIN"
binaryLength="LENGTH_BIN"
md5sumLib="MD5_LIB"
libLength="LENGTH_LIB"

# 进行文件拆解
if [ ! -f $tempDir/binary_temp ]
then
    echo ">> Building new mifsearch..."
    head -n $[startLine + binaryLength] $thisFile | \
            tail -n $[binaryLength + 1] > $tempDir/binary_temp
    chmod +x $tempDir/binary_temp
elif [ `md5sum $tempDir/binary_temp | awk '{print $1}'` != $md5sumBinary ]
then
    echo ">> Updating new mifsearch..."
    head -n $[startLine + binaryLength] $thisFile | \
            tail -n $[binaryLength + 1] > $tempDir/binary_temp
    chmod +x $tempDir/binary_temp
fi

mkdir -p $tempDir/lib_temp
if [ ! -f $tempDir/lib_temp/libc.so.6 ]
then
    echo ">> Building new lib used by mifsearch..."
    tail -n $[libLength + 1] $thisFile > $tempDir/lib_temp/libc.so.6
    chmod +x $tempDir/lib_temp/libc.so.6
elif [ `md5sum $tempDir/lib_temp/libc.so.6 | awk '{print $1}'` != $md5sumLib ]
then
    echo ">> Updating new lib used by mifsearch..."
    tail -n $[libLength + 1] $thisFile > $tempDir/lib_temp/libc.so.6
    chmod +x $tempDir/lib_temp/libc.so.6
fi

# 修改动态链接库地址并执行主进程
# libcVersion=(`ldd --version | awk '/Ubuntu GLIBC/ {print $5}' | sed 's/\./ /g'`)
# if [ ${libcVersion[0]} = 2 -a ${libcVersion[1]} -ge 14 ]
# then
#     $tempDir/binary_temp $*
# else
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$tempDir/lib_temp
    $tempDir/binary_temp $*
# fi
exit
