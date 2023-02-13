extern "C"
int puts(const char* str);

extern "C"
int printf(const char* fmt, ...);

extern "C"
int system(const char* fmt);

extern "C"
void* malloc(unsigned long size);

struct lldiv_t {
	long long quot;
	long long rem;
};

extern "C"
lldiv_t lldiv(long long numer, long long denom);

int main() {
	volatile lldiv_t r = lldiv(18, 5);
	printf("%lld : %lld", r.quot, r.rem);
	system("pause");
	return 0;
}