#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (var x 42)
        (begin 
            (var (x string) "Hello")
            (printf "x: %s\n\n" x))

        (printf "x: %d\n\n" x)

        (set x 100)
        (printf "x: %d\n\n" x)
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}