#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (var x 10)
        (while (> x 0)
            (begin 
                (set x (- x 1))
                (printf "%d\n" x)
            )
        )
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}