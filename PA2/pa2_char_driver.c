#include <linux/init.h>
#include <linux/module.h>

MODULE_AUTHOR("Earl");
MODULE_LICENSE("GPL");

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define BUFFER_SIZE 1024

static char *device_buffer;

/* Define device_buffer and other global data structures you will need here */

#define MY_MAJOR 240
#define MY_MINOR 0
#define MYDEV_NAME "earls_device"

static struct cdev *my_cdev;
static dev_t my_device;
static int count = 1;



ssize_t pa2_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/* length is the length of the userspace buffer*/
	/* offset will be set to current position of the opened file after read*/
	/* copy_to_user function: source is device_buffer and destination is the userspace buffer *buffer */
	
	// Copy the data from the kernel space to the user-space
	int nbytes;
	if ((length + *offset) > BUFFER_SIZE){
		pr_info("trying to read past end of device,"
			"aborting ...");
		return 0;
	nbytes = length - copy_to_user(buffer, device_buffer + *offset, length);
	*offset += nbytes;
	pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*offset);

	return 0;
}



ssize_t pa2_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_from_user function: destination is device_buffer and source is the userspace buffer *buffer */
	
	// Copy the data from user space to kernel-space
	int nbytes;
	if ((length + *offset) > BUFFER_SIZE){
		pr_info("Trying to read past end of device,"
			"aborting ... ");
		return 0;
	}
	nbytes = length - copy_from_user(device_buffer + *offset, buffer, length);
	*offset += nbytes;
	pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*offset);

	return nbytes;
}


int pa2_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	static int open_counter = 0;
	pr_info("attempting to open device: %s:\n", MYDEV_NAME);
	pr_info(" MAJOR number = %d, MINOR number = %d]n",
			imajor(pinode), iminor(pinode));
	open_counter++;

	pr_info(" successfully open device: %s:\n\n", MYDEV_NAME);
	pr_info("I have been opended %d times since being loaded\n", open_counter);
	pr_info("ref=%d\n", (int)module_refcount(THIS_MODULE));

	return 0;
}

int pa2_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	static int close_counter = 0;
	pr_info(" CLOSING device: %s:\n\n", MYDEV_NAME);
	
	close_counter++;

	pr_info(" successfully closed device: %s:\n\n", MYDEV_NAME);
	pr_info("I have been closed %d times since being loaded\n", close_counter);
	pr_info("ref=%d\n", (int)module_refcount(THIS_MODULE));

	return 0;
}

loff_t pa2_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	/* Update open file position according to the values of offset and whence */
	loff_t pos;
	switch(whence){
	
		case SEEK_SET:
			pos = offset;
			break;
		case SEEK_CUR:
			pos = pfile->f_pos + offset;
			break;
		case SEEK_END:
			pos = BUFFER_SIZE - offset;
			break;

		default:
			// Whence is not valid
			return -EINVAL; 		
	}
	if(pos > BUFFER_SIZE){
		pr_info("Seeking beyond buffer\n");
		return -1;
	}
	if( pos < 0){
		pr_info("Seeking before the start of buffer\n");
		return -1;
	}
	pfile->f_pos = pos;
	pr_info("Seeking to pos=%ld\n", (long)pos);
	return 0;
}

struct file_operations pa2_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	.read	 = pa2_char_driver_read,
	.write 	 = pa2_char_driver_write,
	.open	 = pa2_char_driver_open,
	.release = pa2_char_driver_close,
	.llseek  = pa2_char_driver_seek
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
};

int __init pa2_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	/* register the device */
	
	// Registering Major number

	my_device = MKDEV(MY_MAJOR, MY_MINOR);
	// Statically allocating major number
	if(register_chrdev_region(my_device, count, MYDEV_NAME)){
		pr_err("failed to register character device region\n");
		return -1;
	}
	
	// allocate cdev struct
	if(!(my_cdev = cdev_alloc())){
		pr_err("cdev_alloc() failed\n");
		unregister_chrdev_region(my_device, count);
		return -1;
	}
	
	// initialize cdev struct
	cdev_init(my_cdev, &pa2_char_driver_file_operations);
	

	device_buffer = kmalloc( BUFFER_SIZE, GFP_KERNEL);

	// NOTE -- may use cdev_device add 
	if(cdev_add(my_cdev, my_device, count) < 0) {
		pr_err("cdev_add() failed\n");
		cdev_del(my_cdev);
		unregister_chrdev_region(my_device, count);
		kfree(device_buffer);
		return -1;
	}


	// NOTE -- Maybe need to create device

	pr_info("\n Succeded in registering character device %s\n", MYDEV_NAME);

	return 0;
}

void __exit pa2_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	/* unregister  the device using the register_chrdev() function. */

	if (my_cdev){
		cdev_del(my_cdev);
	}
	unregister_chrdev_region(my_device,count);
	kfree(device_buffer);
	pr_info("\ndevice unregistered\n");
}

/* add module_init and module_exit to point to the corresponding init and exit function*/


module_init(pa2_char_driver_init);
module_exit(pa2_char_driver_exit);
