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
	const char* hi = "Hello world!\n";
	puts(hi);
	reprint(">>> ");
	return 0;
}