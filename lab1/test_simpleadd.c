#include <stdio.h>
#include <linux/kernel.h>
#include <unistd.h>
#include <sys/syscall.h>

// used http://man7.org/linux/man-pages/man2/syscall.2.html

int main(int argc, char *argv[])
{
	int sum;
	int call_result = syscall(324, 1, 2, &sum);
	printf("sys_simple_add: returned %d\n", call_result);
	printf("sys_hello_world: 1 + 2 = %d\n", sum);
	return 0;
}