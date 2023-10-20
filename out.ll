; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"
target triple = "x86_64-pc-linux-gnu"

@VERSION = global i32 42, align 4
@version = global i32 42, align 4

declare i32 @printf(i8*, ...)

declare i8* @GC_malloc(i64)

define i32 @main() {
entry:
  ret i32 0
}
