extern "C"
int puts(const char* str);

volatile bool b = true;

int main() {
	int a = b;
	
	return 0;
}