#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (printf "version: %d\n" (var version 42))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}