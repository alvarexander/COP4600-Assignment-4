//character mode linux device driver kernel module
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define SUCCESS 0
#define DEVICE_NAME "inputdevice"	//device name as it appears in /proc/devices
#define CLASS_NAME "chardriverW"
#define BUFF_LEN 1024			//max length of message

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Erick Evangeliste, Eric Watson, Alexander Alvarez, Brandon Bradley");
MODULE_DESCRIPTION("Assignment 3 COP4600");
MODULE_VERSION("1.0");

static int 		Major;					//major number assigned to our device driver
static char 	msg[BUFF_LEN];	//the msg the device will give when as
char *msgptr;
static int 		counter = 0;
int	size_of_message = 0;
static struct 	class* charClass = NULL;
static struct 	device* charDevice = NULL;
static DEFINE_MUTEX(charMutex);

static int 		dev_open(struct inode *, struct file *);
static int 		dev_release(struct inode *, struct file *);

static ssize_t 	dev_write(struct file *, const char *, size_t, loff_t *);

//Operations struct
static struct file_operations fops =
{
	.open = dev_open,
	//.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

//this function is called when the module is loaded
static int __init charDevice_init(void)
{
	mutex_init(&charMutex);
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if(Major < 0)
	{
		printk(KERN_ALERT "Input: Registering char device failed with %d\n", Major);
		return Major;	
	}

	printk(KERN_INFO "Input: New Session Created: #%d\n", Major);

	//register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);

	if(IS_ERR(charClass))
	{
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Input: Failed to register device class\n");
		return PTR_ERR(charClass);
	}

	printk(KERN_INFO "Input: Device class created correctly\n");

	charDevice = device_create(charClass, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME);

	if(IS_ERR(charDevice))
	{
		class_destroy(charClass);
		unregister_chrdev(Major, DEVICE_NAME);
		printk(KERN_ALERT "Char: Failed to create device class\n");
		return PTR_ERR(charDevice);
	}

	printk(KERN_INFO "Input: Device class created correctly");
        
	return SUCCESS;
}

//this function is called when the module is unloaded
static void __exit charDevice_exit(void)
{
	 mutex_destroy(&charMutex);
	device_destroy(charClass, MKDEV(Major, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(Major, DEVICE_NAME);
       

	printk(KERN_INFO "Input: Module succesfully removed!\n");
}

//called when a process tries to open the device file, like "cat/dev/mycharfile"
static int dev_open(struct inode *inode, struct file *file)
{
      if(!mutex_trylock(&charMutex))
    {
        printk(KERN_ALERT "Input: Device is being used by another process");
        return -EBUSY;
    }
	counter++;
	printk(KERN_INFO "Input: Device opened! Access counter: %d\n", counter);
	return 0;
}




//called when a process writes to dev file: echo "hi"
static ssize_t dev_write(struct file *filp, const char *buff, size_t length, loff_t * off)
{
	int i;

	char *ret;
	ret = strstr(buff,"UCF");
	
	printk(KERN_INFO "strstr returns:%s\n",ret);
	
	if((size_of_message + length) > BUFF_LEN)
	{
		for(i = 0; i < (BUFF_LEN - size_of_message); i++)
			msg[i + size_of_message] = buff[i];

		msgptr = msg;

		size_of_message = BUFF_LEN;
		
		if(i > 1)
		{
			printk(KERN_INFO "Input: System has obtained %d characters from user, 0 bytes are available\n", i);
			//mutex_unlock(&charMutex);
			return i;
		}
		else
		{
			return -EFAULT;
		}
	}
	else
	{
		if(strlen(msg) < 1)
			sprintf(msg, "%s", buff);
		else
			sprintf(msg, "%s%s", msg, buff);
		
		
		size_of_message += length;
		printk(KERN_INFO "Input: System has obtained %d characters from user, %d bytes are available\n", length, BUFF_LEN - size_of_message);
		//mutex_unlock(&charMutex);
		msgptr = msg;
		return length;
	}
}

//called when a process closes the device file
static int dev_release(struct inode *inode, struct file *file)
{
	mutex_unlock(&charMutex);
	printk(KERN_INFO "Input: Device successfully closed\n");
	return 0;
}
EXPORT_SYMBOL(size_of_message);
EXPORT_SYMBOL(msg);
EXPORT_SYMBOL(msgptr);
EXPORT_SYMBOL(charMutex);
module_init(charDevice_init);
module_exit(charDevice_exit);

