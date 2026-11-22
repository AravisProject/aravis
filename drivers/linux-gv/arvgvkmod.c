#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emmanuel Pacaud <emmanuel.pacaud@free.fr>");

int
init_module(void)
{
	printk (KERN_ALERT "aravis: loaded\n");

	return 0;
}

void
cleanup_module(void)
{
	printk (KERN_ALERT "aravis: unloaded\n");
}
