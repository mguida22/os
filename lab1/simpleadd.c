#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_simple_add(int x, int y, int *sum)
{
	*sum = x + y;
	printk(KERN_EMERG "simple_add: sum = %d\n", *sum);
	return 0;
}