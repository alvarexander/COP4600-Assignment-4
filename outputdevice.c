/* Seperate Output device driver using the bufer instaniated in the input device

extern - declare character array as extern to use the same character array that was used in input.c

account for mutex locks in code
*/
//character mode linux device driver kernel module
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define SUCCESS 0
#define DEVICE_NAME "outputdevice"	//device name as it appears in /proc/devices
#define CLASS_NAME "chardriverR"
#define BUFF_LEN 1024			//max length of message

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Evangeliste, Eric Watson, Alexander ALvarez, Brandon Bradley");
MODULE_DESCRIPTION("Assignment 3 COP4600");
MODULE_VERSION("1.0");

static int 		Major;	//major number assigned to our device driver
extern  char 	*msgptr;	//the msg the device will give when asked
static int 		counter = 0;
extern  short	size_of_message;
extern struct mutex charMutex;
static struct 	class* charClass = NULL;
static struct 	device* charDevice = NULL;
static int 		dev_open(struct inode *, struct file *);
static int 		dev_release(struct inode *, struct file *);
static ssize_t 	dev_read(struct file *, char *, size_t, loff_t *);


//Operations struct
static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	//.write = dev_write,
	.release = dev_release,
};

//this function is called when the module is loaded
static int __init outputDevice_init(void)
{
	
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if(Major < 0)
	{
		printk(KERN_ALERT "Output: Registering char device failed with %d\n", Major);
		return Major;	
	}

	printk(KERN_INFO "Output: New Session Created: #%d\n", Major);

	//register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);

	if(IS_ERR(charClass))
	{
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Output: Failed to register device class\n");
		return PTR_ERR(charClass);
	}

	printk(KERN_INFO "Output: Device class created correctly\n");

	charDevice = device_create(charClass, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME);

	if(IS_ERR(charDevice))
	{
		class_destroy(charClass);
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Output: Failed to create device class\n");
		return PTR_ERR(charDevice);
	}

	printk(KERN_INFO "Output: Device class created correctly");
	return SUCCESS;
}

//this function is called when the module is unloaded
static void __exit outputDevice_exit(void)
{
	mutex_destroy(&charMutex);
	device_destroy(charClass, MKDEV(Major, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "Output: Module successfully removed!\n");
}

//called when a process tries to open the device file, like "cat/dev/mycharfile"
static int dev_open(struct inode *inode, struct file *file)
{
	if(!mutex_trylock(&charMutex))
	{
		printk(KERN_ALERT "Output: Device in use by another process");
		return -EBUSY;
	}

	counter++;
	printk(KERN_INFO "Output: Device opened! Access counter: %d\n", counter);
	return 0;
}

static ssize_t dev_read(struct file * filp, char *buffer, size_t len, loff_t * offset)
{
	
	 int bytesnotread = 0;
	 int x;
	
	 // handle variable size len - compare size of buffer currently to the length 
	 if(size_of_message > len)
	 {
		 int buffersize = (size_of_message - len);

		 // Will return the amount of bytes that were not successfully copied, returns 0 on sucess
		 bytesnotread = copy_to_user(buffer, msgptr, len);


		 // copy_to_user was succesful

		 if(bytesnotread == 0)
		 {
			 printk(KERN_INFO "chardevice: User received %d chars from system [%s]\n", len, msgptr); //null terminator included in len
			 size_of_message = buffersize;
			 	for(x = 0; x < BUFF_LEN; x++)
			 	{
			 		if(x < (BUFF_LEN-len))
			 		{
			 			msgptr[x]=msgptr[x+len];
			 		}
		 			else
					{
		 				msgptr[x]= '\0';
		 			}
		 		}
		 	return 0;

		
	 	 }
		// Upon failure to copy to user show an error
		 else
		 {
		 	printk(KERN_INFO "chardevice: user has failed to receive %d chars from system\n", bytesnotread);
		 	return -EFAULT;
		 }
	  }

	
		
	 
	 else
	 {
			 bytesnotread = copy_to_user(buffer, msgptr, size_of_message);
			 
			 if(bytesnotread == 0)
			 {
			 		printk(KERN_INFO "Output: user has received %d chars from system [%s]\n", size_of_message, msgptr);
				 for(x = 0; x < BUFF_LEN; x++)
				 {
				 		msgptr[x]= '\0';
				 }
				 return (size_of_message = 0);
			 }
			 
			// Upon failure to copy to user show an error
			 else
			 {
				 printk(KERN_INFO "Output: user has failed to obtain %d chars from system\n", bytesnotread);
				 return -EFAULT;
	 		  }
	}
	
}

//called when a process closes the device file
static int dev_release(struct inode *inode, struct file *file)
{
	mutex_unlock(&charMutex);
	printk(KERN_INFO "Output: Device successfully closed\n");
	return 0;
}


module_init(outputDevice_init);
module_exit(outputDevice_exit);
