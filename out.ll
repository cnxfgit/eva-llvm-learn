; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@version = global i32 42, align 4
@0 = private unnamed_addr constant [13 x i8] c"version: %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @0, i32 0, i32 0), i32 42)
  ret i32 0
}
