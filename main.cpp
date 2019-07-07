/*************************************************
== ConditionAssign ==
Create on: 2018/08/27

����ͼ���������������ֶ�ֵ

֧�ֽ���������������������װ��һ��������(���ü��� "|" �ָ�)
����ֻ����һ��I/O����, ���ڴ������δ����������, ����ʱ�����

*************************************************/

#include <locale>
#include <iostream>
#include <tx_common.h>
#include <tx_platform.h>
#include "Processer.h"
#include "htk/str_helpers.h"

using namespace std;
using namespace basemap;

#define CHECK_STATUS(status, info) do { \
if (status < 0) \
{   \
    sys_log_println(_ERROR, "%s, err_code = %d\n", string(info).c_str(), status); \
    exit(status);  \
}   \
} while(0);

void Usage()
{
    cout << "Usage: ./ConditionAssign ";
    cout << "<input_layer> ";
    cout << "<output_layer> ";
    cout << "<condition_conf_list> ";
    cout << "<log_dir> ";
    cout << "[thread_num]" << endl;

    cout << "Example: ./ConditionAssign C_POI C_POI c1.conf|c2.conf|c3.conf log/ 5" << endl;
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "Chinese-simplified");
    if (argc < 5) {
        Usage();
        return -1;
    }

    string in_layer(argv[1]);
    string out_layer(argv[2]);
    string conf_file_list(argv[3]);
    string log_dir(argv[4]);
    int thread_num = (argc >= 6) ? atoi(argv[5]) : 1;

    time_t start = 0, end = 0;
    time(&start);

    // ������־
    char date_str[256];
    strftime(date_str, sizeof(date_str), "%Y%m%d", localtime(&start));
    sys_log_path(log_dir.c_str(), date_str);

    sys_log_println(_INFORANK, "=====================================\n");
    sys_log_println(_INFORANK, "          [ ConditionAssign ]        \n");
    sys_log_println(_INFORANK, "-------------------------------------\n");
    sys_log_println(_INFORANK, "input_layer: [%s]\n", in_layer.c_str());
    sys_log_println(_INFORANK, "output_layer: [%s]\n", out_layer.c_str());
    sys_log_println(_INFORANK, "thread_num = %d\n", thread_num);
    sys_log_println(_INFORANK, "log_dir: [%s]\n", log_dir.c_str());

    int status = 0;
    Processer proc;
    sys_log_println(_INFORANK, "loading data ...\n");
    status = proc.LoadInput(in_layer);
    CHECK_STATUS(status, "��ȡͼ���ļ�����");

    // ��˳�����δ��������ļ�
    sys_log_println(_INFORANK, "start process ...\n");
    vector<string> conf_file_vec = htk::split(conf_file_list, "|");
    int task_cnt = conf_file_vec.size();
    for (int i = 0; i < task_cnt; ++i)
    {
        time_t task_start = 0, task_end = 0;
        time(&task_start);

        sys_log_println(_INFORANK, "[%d/%d] conf_file: [%s]\n", 
            i + 1, task_cnt, conf_file_vec[i].c_str());

        status = proc.LoadConfig(conf_file_vec[i]);
        CHECK_STATUS(status, "�������ó���");
        sys_log_println(_INFORANK, "[%d/%d] config size = %d\n", i + 1, task_cnt, status);

        status = proc.run(thread_num);
        CHECK_STATUS(status, "�������");

        time(&task_end);
        sys_log_println(_INFORANK, "[%d/%d] succeed. cost time = %ds\n", 
            i + 1, task_cnt, (task_end - task_start));
    }

    status = proc.Save(out_layer);
    CHECK_STATUS(status, "����ͼ���ļ�����");

    time(&end);
    sys_log_println(_INFORANK, "done. total cost time = %ds\n", (end - start));
    sys_log_println(_INFORANK, "=====================================\n\n");
    sys_log_close();
    return 0;
}
