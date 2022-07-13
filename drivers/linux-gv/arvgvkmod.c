#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emmanuel Pacaud <emmanuel@gnome.org>");

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
