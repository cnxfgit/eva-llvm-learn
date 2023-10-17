#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (def square (x) (* x x))
        (def sum ((a number) (b number)) -> number (+ a b))
        
        (printf "%d\n" (square 2))
        (printf "%d\n" (sum 2 4))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}