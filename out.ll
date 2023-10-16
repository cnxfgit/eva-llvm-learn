; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 42, align 4
@version = global i32 42, align 4
@0 = private unnamed_addr constant [8 x i8] c"x = %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 320, i32* %x, align 4
  %x1 = load i32, i32* %x, align 4
  %tmpcmp = icmp uge i32 %x1, 32
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @0, i32 0, i32 0), i1 %tmpcmp)
  ret i32 0
}
