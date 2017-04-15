#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_hello_world(void)
{
	printk(KERN_EMERG "hello world\n");
	return 0;
}