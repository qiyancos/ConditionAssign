#include <iostream>

class Progress {
public:
    // 构造函数
    Progress(const int totalCount) : totalCount_(totalCount) {
        for (int i = 0; i < 50; i++) {
            progressBar_[i] = '-';
        }
    }
    // 添加新的进度数值
    void addProgress(const int newProgress) {
        progressCount_ += newProgress;
        percentage_ = static_cast<double>(progressCount_ * 100) / totalCount_;
        if (percentage_ >= ((barCount_ + 1) << 1)) {
            progressBar_[barCount_++] = '#';
            dumpProgress();
        }
    }

private:
    // 打印信息
    void dumpProgress() {
        printf("[%s] %d/%d %.2f%%", progressBar_, progressCount_, totalCount_,
                percentage_);
        if (progressCount_ == totalCount_) {
            printf("\n");
        } else {
            printf("\r");
        }
    }

    // 当前进度的百分比
    double percentage_ = 0;
    // 当前要写的barCount
    int barCount_ = 0;
    // 进度条
    char progressBar_[51] {0};
    // 当前进度的实际数值
    int progressCount_ = 0;
    // 总进度数值
    const int totalCount_;
};
