#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>  // For copy_from_user
#include <linux/slab.h>
#include <linux/cdev.h>     // For cdev structure
#include <linux/device.h>   // For device_create and class_create
#include <linux/mutex.h>    // For locking

#include "hfi_msgs.h"
#include "wrapper.h"

// Declare pre-allocated buffer pool
static struct args_data *buffer_pool[POOL_SIZE];
static bool buffer_used[POOL_SIZE];  // Track whether each buffer is in use
static struct mutex pool_lock;       // Protect access to the buffer pool

static dev_t dev_num;                // To store the dynamically allocated device number (major + minor)
static struct cdev my_cdev;          // Character device structure
static struct class *ioctl_class = NULL;  // Device class
static struct device *ioctl_device = NULL;  // Device struct

// Function prototypes
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static long device_ioctl(struct file *, unsigned int, unsigned long);
static struct args_data* get_free_buffer(void);
static void release_buffer(struct args_data*);

// File operations structure
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl,
};

// Get a free buffer from the pool
static struct args_data* get_free_buffer(void)
{
	int i;
	mutex_lock(&pool_lock);
	for (i = 0; i < POOL_SIZE; i++) {
		if (!buffer_used[i]) {
			buffer_used[i] = true;
			mutex_unlock(&pool_lock);
			return buffer_pool[i];
		}
	}
	mutex_unlock(&pool_lock);
	return NULL;  // No free buffers available
}

// Release a buffer back to the pool
static void release_buffer(struct args_data *buffer)
{
	int i;
	mutex_lock(&pool_lock);
	for (i = 0; i < POOL_SIZE; i++) {
		if (buffer_pool[i] == buffer) {
			buffer_used[i] = false;
			break;
		}
	}
	mutex_unlock(&pool_lock);
}

// Initialize the module
static int __init ioctl_init(void)
{
	int ret, i;

	// Initialize mutex
	mutex_init(&pool_lock);

	// Allocate buffer pool
	for (i = 0; i < POOL_SIZE; i++) {
		buffer_pool[i] = kmalloc(BUFFER_SIZE, GFP_KERNEL);
		if (!buffer_pool[i]) {
			printk(KERN_ALERT "Failed to allocate memory for buffer %d\n", i);
			ret = -ENOMEM;
			goto error_alloc;
		}
		buffer_used[i] = false;
	}

	// Dynamically allocate a major number
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ALERT "Failed to allocate major number\n");
		goto error_alloc;
	}

	// Initialize and add the cdev structure
	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;

	ret = cdev_add(&my_cdev, dev_num, 1);
	if (ret < 0) {
		printk(KERN_ALERT "Failed to add cdev\n");
		goto error_cdev;
	}

	// Create a device class
	ioctl_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ioctl_class)) {
		printk(KERN_ALERT "Failed to create device class\n");
		ret = PTR_ERR(ioctl_class);
		goto error_class;
	}

	// Create the device in /dev/
	ioctl_device = device_create(ioctl_class, NULL, dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(ioctl_device)) {
		printk(KERN_ALERT "Failed to create device\n");
		ret = PTR_ERR(ioctl_device);
		goto error_device;
	}

	printk(KERN_INFO "IOCTL wrapper module loaded with major number %d\n", MAJOR(dev_num));
	return 0;

error_device:
	class_destroy(ioctl_class);
error_class:
	cdev_del(&my_cdev);
error_cdev:
	unregister_chrdev_region(dev_num, 1);
error_alloc:
	for (i = 0; i < POOL_SIZE; i++) {
		if (buffer_pool[i]) {
			kfree(buffer_pool[i]);
		}
	}
	return ret;
}

// Cleanup the module
static void __exit ioctl_exit(void)
{
	int i;

	// Remove the device
	device_destroy(ioctl_class, dev_num);
	class_destroy(ioctl_class);

	// Remove the cdev structure and free the major/minor numbers
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);

	// Free the buffer pool
	for (i = 0; i < POOL_SIZE; i++) {
		if (buffer_pool[i]) {
			kfree(buffer_pool[i]);
		}
	}

	printk(KERN_INFO "IOCTL wrapper module unloaded\n");
}

// Open function
static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device opened\n");
	return 0;
}

// Release function
static int device_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device closed\n");
	return 0;
}

static int copyargs_from_user(struct args_data *buffer, unsigned long arg) {

	int ret;

	ret = copy_from_user(buffer, (struct args_data*)arg, BUFFER_SIZE);
	if (ret < 0) goto alloc_failed;

	return 0;

alloc_failed:
	return -ENOMEM;
}

// IOCTL function
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct args_data *input_buffer;
	int ret;

	// Get a free buffer from the pool
	input_buffer = get_free_buffer();
	if (!input_buffer) {
		printk(KERN_ALERT "No free buffers available\n");
		return -ENOMEM;
	}

	switch (cmd) {
		case IOCTL_CMD_WRAPPER:
			// Copy the string from user space
			ret = copyargs_from_user(input_buffer, arg);
			if (ret < 0) {
				printk(KERN_ALERT "Failed to copy data from user space\n");
				release_buffer(input_buffer);
				return -EFAULT;
			}

			//Cook the books
			input_buffer->info.shdr.hdr.size = 404;
			//input_buffer->info.data = kmalloc(sizeof(struct hfi_buffer_requirements), GFP_KERNEL);
			memcpy(&input_buffer->info.data[0], &input_buffer->reqs, sizeof(struct hfi_buffer_requirements));
			if (!input_buffer->info.data[0])
			{
				printk(KERN_ALERT "Data assignment failed\n");
				return -ENOBUFS;
			}

			struct hfi_buffer_requirements * ptr = (struct hfi_buffer_requirements *)&input_buffer->info.data[0];
			if (!ptr)
				printk(KERN_ALERT "Pointer assignment failed\n");

			// Make IOCTL call here
			printk(KERN_INFO "Check should be successful...\n");
			ret = session_get_prop_buf_req(&input_buffer->info, &input_buffer->reqs);
			if (HFI_ERR_NONE == ret) {
				printk(KERN_INFO "IOCTL no error\n");
			}
			else if (HFI_ERR_SESSION_INVALID_PARAMETER == ret) {
				printk(KERN_INFO "Invalid parameter\n");
				return -EINVAL;
			}
			else if (HFI_BUG == ret){
				printk(KERN_ALERT "Successfully triggered bug!\n");
				return -EFAULT;
			}

			// Release the buffer back to the pool
			release_buffer(input_buffer);
			break;

		default:
			printk(KERN_ALERT "Invalid IOCTL command\n");
			release_buffer(input_buffer);
			return -EINVAL;
	}

	return 0;
}

// Register module entry and exit points
module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christiaan Botha");
MODULE_DESCRIPTION("A wrapper for targeted fuzzing of android modules");
MODULE_VERSION("1.0");
