#ifndef CONDITION_ASSIGN_H
#define CONDITION_ASSIGN_H
/*************************************************
== ConditionAssign ==
Update on: 2019/07/10
Create on: 2018/08/27

根据图层属性条件设置字段值

支持将几个连续的条件配置组装成一个配置流(配置间用 "|" 分隔)
这样只进行一次I/O操作, 在内存中依次处理各个配置, 减少时间损耗

*************************************************/

#include "executor.h"
#include "conf_helper.h"
#include "htk/str_helpers.h"

#include <locale>
#include <iostream>
#include <tx_common.h>
#include <tx_platform.h>

namespace condition_asssign {

#define CHECK_EXIT(expr, info) { \
    int errCode = expr; \
    if (errCode) { \
        sys_log_println(_ERROR, "%s, err_code = %d\n", \
                string(info).c_str(), errCode); \
        exit(status); \
    } \
}

#define CHECK_RET(expr, info) { \
    int errCode = expr; \
    if (errCode) { \
        sys_log_println(_ERROR, "%s, err_code = %d\n", \
                string(info).c_str(), errCode); \
        exit(status); \
    } \
}

} // namespace condition_asssign
