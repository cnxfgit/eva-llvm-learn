#include "./src/EvaLLVM.h"
#include <string>
#include <fstream>
#include <iostream>

void printHelp()
{
    std::cout << "\nUseage: eva-llvm [options]\n\n"
              << "Options: \n"
              << "      -e, --expression Expression to parse\n"
              << "      -f, --file       File to parse\n\n";
}

int main(int argc, const char *argv[])
{
    if (argc != 3)
    {
        printHelp();
        return 0;
    }

    std::string mode = argv[1];

    std::string program;

    if (mode == "-e")
    {
        program = argv[2];
    }
    else if (mode == "-f")
    {
        std::ifstream programFile(argv[2]);
        std::stringstream buffer;
        buffer << programFile.rdbuf();

        program = buffer.str();
    }

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}