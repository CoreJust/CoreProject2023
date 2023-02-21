extern "C"
int puts(const char* str);

extern "C"
int printf(const char* fmt, ...);

extern "C"
int system(const char* fmt);

extern "C"
void* malloc(unsigned long size);

union U {
	long long a;
	char* b;
	float d;
};

struct S {
	const char* data;
	unsigned long long size;
};

volatile U u;

int main() {
	volatile unsigned long a;
	volatile S s { "Hi", a };
	
	volatile char* arr;
	volatile char ch = arr[a];
	
	system("pause");
	return 0;
}