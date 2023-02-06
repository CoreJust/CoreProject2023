extern "C"
int puts(const char*);

extern "C"
int getchar();

void reprint(const char* q) {
	puts(q);
	getchar();
	reprint(q);
}

int main() {
	puts("Hello world!\n");
	reprint(">>> ");
	return 0;
}