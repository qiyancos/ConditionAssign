#ifndef CONDITION_ASSIGN_H
#define CONDITION_ASSIGN_H
/*************************************************
== ConditionAssign ==
Update on: 2019/07/10
Create on: 2018/08/27

����ͼ���������������ֶ�ֵ

֧�ֽ���������������������װ��һ��������(���ü��� "|" �ָ�)
����ֻ����һ��I/O����, ���ڴ������δ����������, ����ʱ�����

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
