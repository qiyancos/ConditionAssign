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
if [ ! -f $tempDir/ConditionAssign ]
then
    echo ">> Building new ConditionAssign..."
    head -n $[startLine + binaryLength] $thisFile | \
            tail -n $[binaryLength + 1] > $tempDir/ConditionAssign
    chmod +x $tempDir/ConditionAssign
elif [ `md5sum $tempDir/ConditionAssign | awk '{print $1}'` != $md5sumBinary ]
then
    echo ">> Updating new ConditionAssign..."
    head -n $[startLine + binaryLength] $thisFile | \
            tail -n $[binaryLength + 1] > $tempDir/ConditionAssign
    chmod +x $tempDir/ConditionAssign
fi

mkdir -p $tempDir/libConditionAssign
if [ ! -f $tempDir/libConditionAssign/libc.so.6 ]
then
    echo ">> Building new lib used by ConditionAssign..."
    tail -n $[libLength + 1] $thisFile > $tempDir/libConditionAssign/libc.so.6
    chmod +x $tempDir/libConditionAssign/libc.so.6
elif [ `md5sum $tempDir/libConditionAssign/libc.so.6 | awk '{print $1}'` != $md5sumLib ]
then
    echo ">> Updating new lib used by ConditionAssign..."
    tail -n $[libLength + 1] $thisFile > $tempDir/libConditionAssign/libc.so.6
    chmod +x $tempDir/libConditionAssign/libc.so.6
fi

# 修改动态链接库地址并执行主进程
libcVersion=(`ldd --version | sed -n 1p`)
libcVersion=(`echo ${libcVersion[$[${#libcVersion[*]} - 1]]} | sed 's/\./ /g'`)
if [ ${libcVersion[0]} -lt 2 -o ${libcVersion[1]} -lt 18 ]
then export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$tempDir/libConditionAssign
fi
$tempDir/ConditionAssign $*
exit
