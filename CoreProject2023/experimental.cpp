
extern "C"
int puts(const char* str);

extern "C"
int printf(const char* fmt, ...);

extern "C"
int system(const char* fmt);

extern "C"
void* malloc(unsigned long size);

union U {
	int i;
	double f;
	struct {
		char c;
		bool b;
	};
};

int main() {
	U u;
	u.i = 10;
	printf("%f", u.f);
	
	return 0;
}

#if 0
#include <setjmp.h>

/* Declare a global jmp_buf variable that is available to both func and main */
static jmp_buf env;

void func(void)
{
    /* Display a message indicating we are entering func */
    printf("Starting func\n");

    /* Return to main with a return code of 1 (can be anything except 0) */
    longjmp(env, 1);

    /* Display a message indicating we are leaving func */
    printf("Finishing func\n"); /* This will never be executed! */
}

int main(int argc, const char * argv[])
{
    /* Define temporary variables */
    int result;

    /* Display a message indicating we are starting main */
    printf("Starting main\n");

    /* Save the calling environment, marking where we are in main */
    result = _setjmp(env);

    /* If the result is not 0 then we have returned from a call to longjmp */
    if (result != 0)
    {
        /* Display a message indicating the call to longjmp */
        printf("longjmp was called\n");

        /* Exit main */
        return 0;
    }

    /* Call func */
    printf("Starting func\n");

    /* Return to main with a return code of 1 (can be anything except 0) */
    longjmp(env, 1);

    /* Display a message indicating we are leaving main */
    printf("Finished main\n");

    return 0;
}
#endif