extern "C"
int puts(const char* str);

int fn(const int& a) {
	return a + a * a;
}

int main() {
	int a;
	fn(5);
	fn(a);
	
	return 0;
}