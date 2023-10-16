#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (var x (* 32 10))
        //(if (== x 42)
        //    (set x 100)
        //    (set x 200)
        //)

        (printf "x = %d\n" (>= x 32))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}