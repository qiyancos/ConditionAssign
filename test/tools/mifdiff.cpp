#include "mif_helper.h"

#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: mifdiff <mif file1> <mif file2>" << std::endl;
        exit(1);
    }
    std::string mifFile1(argv[1]);
    std::string mifFile2(argv[2]);
    std::string midFile1, midFile2;
    mif_helper::formalizeFileName(&mifFile1, &midFile1);
    mif_helper::formalizeFileName(&mifFile2, &midFile2);
    if (mif_helper::checkSameMif(mifFile1, mifFile2) &&
            mif_helper::checkSameMid(mifFile1, mifFile2)) {
        std::cout << "Result Match" << std::endl;
    } else {
        std::cout << "Result Not Match" << std::endl;
    }
    return 0;
}
