@s0 = internal constant [14 x i8] c"Hello: %d %d\0a\00"
@value_ptr = global i64 5

declare i64 @printf(i8*, ...)

define i64 @main(i64 %argc, i8** %argv){
  %val = load i64* @value_ptr
  %val2_ptr = alloca i32
  store i32 10, i32* %val2_ptr
  %val2 = load i32* %val2_ptr
  %r0 = getelementptr [14 x i8]* @s0, i32 0, i32 0
  %r1 = call i64 (i8*, ...)* @printf(i8* %r0, i64 128, i32 %val2)
  %r2 = add i64 0, 0
  ret i64 2
}
