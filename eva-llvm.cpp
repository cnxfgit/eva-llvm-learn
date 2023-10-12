#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        //(var VERSION 42)
        //(begin 
        //    (var VERSION "hello")
        //    (printf "VERSION: %s\n\n" VERSION))
        (printf "VERSION: %d\n\n" VERSION)
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}