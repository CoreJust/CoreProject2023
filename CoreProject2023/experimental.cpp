extern "C"
int puts(const char* str);

extern "C"
int printf(const char* fmt, ...);

extern "C"
void* malloc(unsigned long size);
/*
typedef unsigned long long u64;
typedef long long i64;
typedef double f64;
typedef char c8;

bool lessOrEqual(u64 i, u64 j) {
	u64 s = (1ull << 63);
	while (s) {
		bool isI = bool(i & s);
		bool isJ = bool(j & s);
		
		if (isI && !isJ)
			return false;
		else if (!isI && isJ)
			return true;
		
		s = s >> 1;
	}
	
	return true;
}

void reverse(c8* str, u64 i, u64 j) {
	if (lessOrEqual(i, j)) return;
	
	c8 tmp = *(str + i);
	*(str + i) = *(str + j);
	*(str + j) = tmp;
	
	reverse(str, i + 1, j - 1);
}

c8* float2str(f64 x) {
	c8* result = (c8*)malloc(24);
	u64 i = 0;
	i64 whole = i64(x);
	x = x - whole;
	
	while(whole) {
		c8& symbol = *(result + i++);
		symbol = c8(whole % 10 + i64('0'));
		whole = whole / 10;
	}
	
	reverse(result, 0, i - 1);
	
	*(result + i++) = '.';
	while (x) {
		x = x * 10;
		whole = i64(x);
		x = x - whole;
		*(result + i++) = c8(whole + i64('0'));
	}
	
	*(result + i) = '\0';
	return result;
}
*/
int main() {
	//puts(float2str(1225.094832));
	printf("%s", "Hello world!");
	
	return 0;
}