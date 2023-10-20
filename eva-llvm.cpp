#include "./src/EvaLLVM.h"
#include <string>

int main(int argc, const char *argv[])
{
    std::string program = R"(
        (class Transformer null
            (begin
                (var factor 0)
                (def constructor (self factor) -> Transformer
                    (begin 
                        (set (prop self factor) factor)
                        self
                    )
                )
                (def __call__(self v)
                    (* (prop self factor) v)
                )
            )
        )
        (var transform (new Transformer 5))

        (printf "(transform 10) = %d\n" (transform 10))
        (def calculate (x (modify Transformer))
            (modify x)
        )
        (printf "(calculate 10 transform) = %d\n" (calculate 10 transform))

    )";

    EvaLLVM vm;

    vm.exec(program);

    return 0;
}