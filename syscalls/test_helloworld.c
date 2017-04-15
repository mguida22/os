#include <stdio.h>
#include <linux/kernel.h>
#include <unistd.h>
#include <sys/syscall.h>

// used http://man7.org/linux/man-pages/man2/syscall.2.html

int main(int argc, char *argv[])
{
	int call_result = syscall(323);
	printf("sys_hello_world: %d\n", call_result);
	return 0;
}