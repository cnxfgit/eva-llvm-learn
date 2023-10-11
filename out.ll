; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@0 = private unnamed_addr constant [7 x i8] c"hello\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @0, i32 0, i32 0))
  ret i32 0
}
