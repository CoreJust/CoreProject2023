extern "C"
int puts(const char* str);

int(*printer)(const char*) = puts;

int main() {
	printer("Why?");
	return 0;
}