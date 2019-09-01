#include "program_helper.h"

int main(int argc, char** argv) {
    if (argc < 2) return 0;
    std::vector<std::string> options;
    program_helper::listDir(argv[1], &options);
    std::vector<std::string> result;
    program_helper::manualSelect(options, &result);
    std::cout << "Select:" << std::endl;
    for (const std::string& name : result) {
        std::cout << name << std::endl;
    }
    return 0;
}
