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
	
	String(char* str) {
		this->data = str;
	}
};

volatile String r("Wow");

int main() {
	printf("%s", r.data);
	system("pause");
	return 0;
}