#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024

static char device_buffer[BUFFER_SIZE];

static const char device_name[23]= "simple_character_device";

static int open_count = 0;
static int close_count = 0;
static int write_length = 0;
static int read_length = 0;
ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace 		buffer *buffer */
	int buffer_length = strlen(device_buffer);

	if (length + *offset > BUFFER_SIZE) {
		length = BUFFER_SIZE - *offset;
	}

	// need to know how much we've read
	read_length += buffer_length - copy_to_user(buffer, device_buffer, length);
	
	if (length > 0){
		*offset += length;
		printk(KERN_ALERT "Bytes read: %d\n", read_length);
	}

	return length;
}

ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */

	if(length + *offset > BUFFER_SIZE) {
		length = BUFFER_SIZE - *offset;
	}

	copy_from_user(device_buffer + strlen(device_buffer), buffer, length);

	// know how much we wrote
	write_length += length;

	printk(KERN_ALERT "Bytes written: %d\n", write_length);

	return length;
}

int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	open_count++;
	
	printk(KERN_ALERT "Opened device. Current count: %d\n", open_count);
	return 0;
}

int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	close_count++;

	printk(KERN_ALERT "Closed device. Current count: %d\n", close_count); 
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
	.open = simple_char_driver_open,
	.release = simple_char_driver_close,
	.read = simple_char_driver_read,
	.write = simple_char_driver_write,
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_EMERG "simple_char_driver_init function called.");

	/* register the device */
	register_chrdev(240, &device_name, &simple_char_driver_file_operations);
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	printk(KERN_EMERG "simple_char_driver_exit function called.");
	/* unregister  the device using the unregister_chrdev() function. */
	unregister_chrdev(240, &device_name);
	return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);