clang++-14 -o eva-llvm `llvm-config-14 --cxxflags --ldflags --system-libs --libs core` eva-llvm.cpp

./eva-llvm

lli-14 ./out.ll

echo $?

printf "\n"