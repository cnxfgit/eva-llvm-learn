#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (class Point null 
            (begin 
                (var x 0)
                (var y 0)
                (def constructor (self x y)
                    (begin 
                        (set (prop self x) x)
                        (set (prop self y) y) 
                    )
                )

                (def calc (self)
                    (+ (prop self x) (prop self y))
                )
            )
        )
        (var p (new Point 10 20))
        (printf "%d\n" (prop p x))
    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}