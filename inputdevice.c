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
static ssize_t dev_write(struct file *filp, const char *buffer, size_t length, loff_t * off)
{
	int oldLength = size_of_message, strLength, strStart = 0;
	const char *strPtr = buffer;
	const char phrase[] = "Undefeated 2018 National Champions UCF";

	// Loop used to filter the substring of "UCF" with the phrase "Undefeated 2018 National Champions UCF"
	// Pointer is moved to the first occurrence of "UCF" from it's current position
	while((strPtr = strstr(strPtr, "UCF")) != NULL)
	{
		// Sets the string length as the pointer position subtracted by the string start index
		strLength = strPtr - (buffer + strStart);

		// If the length of the string being added to the buffer is greater than the buffer length, then the buffer is maxed out
		if(size_of_message + strLength > BUFF_LEN)
		{
			copy_from_user(msg + size_of_message, buffer + strStart, BUFF_LEN - size_of_message);
			size_of_message = BUFF_LEN;
			printk(KERN_INFO "Input: System has obtained %d characters from user, 0 bytes are available\n", size_of_message - oldLength);
			msgptr = msg;
			return -EFAULT;
		}

		// Concatinates the string before the substring of "UCF" to the buffer
		// Adds the length of the new string added to the buffer length
		copy_from_user(msg + size_of_message, buffer + strStart, strLength);
		size_of_message += strLength;

		// Loop concatinates the replacement phrase to the buffer
		for(strLength = 0; strLength < (BUFF_LEN - size_of_message) && strLength < strlen(phrase); strLength++)
			msg[size_of_message + strLength] = phrase[strLength];

		// If the length of the phrase being added isn't equal to the known length, then the buffer is maxed out
		if(strLength != strlen(phrase))
		{
			size_of_message = BUFF_LEN;
			printk(KERN_INFO "Input: System has obtained %d characters from user, 0 bytes are available\n", size_of_message - oldLength);
			msgptr = msg;
			return -EFAULT;
		}

		// Adds the length of the new string added to the buffer length
		// The pointer used for finding the substring of "UCF" is incremented by 3, so it can find the next occurrence
		// Adjusts the string start index to be the character after the substring of "UCF"
		size_of_message += strLength;
		strPtr += 3;
		strStart = strPtr - buffer;
	}

	// String length is set as the number of remaining characters after the last occurrence of "UCF"
	strLength = length - strStart;

	// If the length of the string being added to the buffer is greater than the buffer length, then the buffer is maxed out
	if(size_of_message + strLength > BUFF_LEN)
	{
		copy_from_user(msg + size_of_message, buffer + strStart, BUFF_LEN - size_of_message);
		size_of_message = BUFF_LEN;
		printk(KERN_INFO "Input: System has obtained %d characters from user, 0 bytes are available\n", size_of_message - oldLength);
		msgptr = msg;
		return -EFAULT;
	}

	// Concatinates the remaining characters after the last substring to the buffer
	copy_from_user(msg + size_of_message, buffer + strStart, strLength);
	size_of_message += strLength;
	printk(KERN_INFO "Input: System has obtained %d characters from user, %d bytes are available\n", size_of_message - oldLength, BUFF_LEN - size_of_message);
	msgptr = msg;
	return 0;
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

