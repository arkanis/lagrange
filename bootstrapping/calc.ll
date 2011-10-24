@format = internal constant [8 x i8] c"%ld %s\0A\00"
@str = internal constant [13 x i8] c"hello world\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main(){
	%format_ptr = getelementptr [8 x i8]* @format, i32 0, i32 0
	%str = getelementptr [13 x i8]* @str, i32 0, i32 0
	%value = call i64 @calc(i32 12)
	call i32 (i8*, ...)* @printf(i8* %format_ptr, i64 %value, i8* %str)
	ret i32 0
}

define i64 @calc(i32 %in){
	%ext_in = sext i32 %in to i64
	%result = mul nsw i64 8, %ext_in
	ret i64 %result
}