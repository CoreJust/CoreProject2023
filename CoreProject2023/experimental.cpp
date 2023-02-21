extern "C"
int puts(const char* str);

extern "C"
int printf(const char* fmt, ...);

extern "C"
int system(const char* fmt);

extern "C"
void* malloc(unsigned long size);

struct String {
	char* data;
	long long size;
	long long capacity;
	
	String(char* str) {
		this->data = str;
	}
	
	void append(long long oth) {
		size = capacity > size;
	}
};

int main() {
	String r("Hi");
	r.append(10);
	printf("%s", r.data);
	
	system("pause");
	return 0;
}