#ifndef CONDITIONASSIGN_H
#define CONDITIONASSIGN_H
/*************************************************
== ConditionAssign ==
Update on: 2019/07/10
Create on: 2018/08/27

����ͼ���������������ֶ�ֵ

֧�ֽ���������������������װ��һ��������(���ü��� "|" �ָ�)
����ֻ����һ��I/O����, ���ڴ������δ����������, ����ʱ�����

*************************************************/

#include "htk/str_helpers.h"

#include <locale>
#include <iostream>
#include <tx_common.h>
#include <tx_platform.h>

namespace condition_asssign {

#define CHECK_EXIT(expr, info, ...) { \
    int errCode = expr; \
    if (errCode < 0) { \
        sys_log_println(_ERROR, (std::string(info) + " in [%s]." + \
                "Error code = %d\n").c_str(), __VA_ARGS__, __func__, errCode); \
        exit(status); \
    } \
}

#define CHECK_RET(expr, info, ...) { \
    int retCode = expr; \
    if (retCode < 0) { \
        sys_log_println(_ERROR, (std::string(info) + " in [%s]." + \
                "Return %d\n").c_str(), __VA_ARGS__, __func__, retCode); \
        return retCode; \
    } \
}

#define CHECK_ARGS(expr, info, ...) { \
    if (!expr) { \
        sys_log_println(_ERROR, (std::string(info) + " in [%s].").c_str(), \
                __VA_ARGS__, __func__); \
        return -1; \
    } \
}

} // namespace condition_asssign

#endif // CONDITIONASSIGN_H
