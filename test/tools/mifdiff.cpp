#include "spatial-base.h"
#include "type_factory.h"
#include "md5_helper.h"
#include "htk/str_helpers.h"
#include "progress.h"

#include <vector>
#include <string>
#include <map>
#include <stdio.h>
#include <unistd.h>

int formalizeFileName(std::string* mifFile, std::string* midFile) {
    std::string suffix = mifFile->substr(mifFile->size() - 4, 4);
    if (suffix == ".mif") {
        *midFile = mifFile->substr(0, mifFile->size() - 4) + ".mid";
    } else if (suffix == ".MIF") {
        *midFile = mifFile->substr(0, mifFile->size() - 4) + ".MID";
    } else {
        *midFile = *mifFile + ".mid";
        *mifFile += ".mif";
    }
    return 0;
}

bool checkSameMif(const std::string& file1, const std::string& file2) {
    std::string md5sum1 = md5file(file1);
    std::string md5sum2 = md5file(file2);
    if (md5sum1 == md5sum2) {
        std::cout << "Mif file match!" << std::endl;
        return true;
    } else {
        std::cout << "Mif File not match [" << md5sum1 << ":" <<
                md5sum2 << "]!" << std::endl;
        return false;
    }
}

bool checkSameMid(const std::string& file1, const std::string& file2) {
    wgt::MIF mif1, mif2;
    std::cout << "Loading mid file..." << std::endl;
    if (wgt::mif_to_wsbl(file1, mif1) < 0) {
        std::cout << "Failed to open file " << file1 << std::endl;
        exit(1);
    }
    if (wgt::mif_to_wsbl(file2, mif2) < 0) {
        std::cout << "Failed to open file " << file2 << std::endl;
        exit(1);
    }
    int mifSize = mif1.mid.size();
    if (mifSize != mif2.mid.size()) {
        std::cout << "Mif size mismatch [" << mifSize << ":" <<
                mif2.mid.size() << "]!" << std::endl;
        return false;
    }
    int colSize = mif1.mid[0].size();
    if (colSize != mif2.mid[0].size()) {
        std::cout << "Column size mismatch [" << colSize << ":" <<
                mif2.mid[0].size() << "]!" << std::endl;
        return false;
    }
    std::cout << "Start checking mid file." << std::endl;
    Progress progressCounter(mifSize);
    for (int mifIndex = 0; mifIndex < mifSize; mifIndex++) {
        for (int colID = 0; colID < colSize; colID++) {
            std::string tagVal1 = htk::trim(mif1.mid[mifIndex][colID], "\"");
            std::string tagVal2 = htk::trim(mif2.mid[mifIndex][colID], "\"");
            if (tagVal1 != tagVal2) {
                std::cout << "Tag mismatch [\"" << tagVal1 << "\" : \"" <<
                        tagVal2 << "\"] in mif item (" << mifIndex + 1 <<
                        ":" << colID + 1 << ")." << std::endl;
                return false;
            }
        }
        progressCounter.addProgress(1);
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: mifdiff <mif file1> <mif file2>" << std::endl;
        exit(1);
    }
    std::string mifFile1(argv[1]);
    std::string mifFile2(argv[2]);
    std::string midFile1, midFile2;
    formalizeFileName(&mifFile1, &midFile1);
    formalizeFileName(&mifFile2, &midFile2);
    if (checkSameMif(mifFile1, mifFile2) && checkSameMid(mifFile1, mifFile2)) {
        std::cout << "Result Match" << std::endl;
    } else {
        std::cout << "Result Not Match" << std::endl;
    }
}
