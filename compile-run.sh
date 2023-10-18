clang++-14 -o eva-llvm `llvm-config-14 --cxxflags --ldflags --system-libs --libs core` -fexceptions eva-llvm.cpp

./eva-llvm

#lli-14 ./out.ll
clang++-14 -O3 -I/usr/include/gc ./out.ll -L/usr/lib/x86_64-linux-gnu/gc -lgc -o ./out

./out

echo $?

printf "\n"