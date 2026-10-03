/*
 * smart_meter_driver.c
 * A very simple Linux kernel character device driver.
 *
 * What it does:
 *   1. Registers a character device.
 *   2. Creates the device file /dev/smart_meter
 *   3. Keeps one variable in memory: pulse_counter
 *   4. Increases the counter when a pulse is written to the device
 *   5. Lets user space read the counter, reset it, or add pulses
 *
 * Pulse idea:
 *   A real electricity meter gives one pulse (a click) for some fixed
 *   amount of energy. Here we only SIMULATE the pulse, so one pulse is
 *   "one click of the meter".
 *
 * Student: you do NOT need real hardware for this driver.
 */

#include <linux/module.h>       /* module registration (MODULE_LICENSE etc.) */
#include <linux/fs.h>           /* file_operations structure */
#include <linux/miscdevice.h>   /* easy way to create /dev/<name> device */
#include <linux/uaccess.h>      /* copy_to_user / copy_from_user */
#include <linux/mutex.h>        /* mutex lock/unlock */
#include <linux/kernel.h>       /* printk */
#include <linux/slab.h>         /* kmalloc/kfree (not used yet, good habit) */

#include "smart_meter_ioctl.h"  /* our shared commands */

/* ---- Global variables of the driver ------------------------------- */

/* The pulse counter itself. Lives in kernel memory while driver loaded. */
static unsigned long pulse_counter = 0;

/* Lock: protects pulse_counter when 2 programs use the device at once. */
static DEFINE_MUTEX(smart_meter_lock);

/* ---- Function 1: called when a user program OPENS /dev/smart_meter - */
static int smart_meter_open(struct inode *inode, struct file *file)
{
    pr_info("smart_meter: device opened\n");
    return 0;   /* 0 = success */
}

/* ---- Function 2: called when user program CLOSES the device ------- */
static int smart_meter_release(struct inode *inode, struct file *file)
{
    pr_info("smart_meter: device closed\n");
    return 0;
}

/* ---- Function 3: READ  -> send the pulse count to user space ----- *
 * User reads it like a text file:  cat /dev/smart_meter             */
static ssize_t smart_meter_read(struct file *file,
                                char __user *user_buf,
                                size_t count,
                                loff_t *ppos)
{
    char temp[32];               /* small buffer to build the text */
    int  len;
    unsigned long pulses;

    mutex_lock(&smart_meter_lock);
    pulses = pulse_counter;      /* safe copy while holding the lock */
    mutex_unlock(&smart_meter_lock);

    /* Turn the number into text, for example "1234\n" */
    len = scnprintf(temp, sizeof(temp), "%lu\n", pulses);
    if (len < 0)
        return len;

    /* Copy text from kernel memory to user memory */
    return copy_to_user(user_buf, temp, len);
}

/* ---- Function 4: WRITE -> a pulse is received --------------------- *
 * The user writes a NUMBER as text, for example "5".                  *
 * The driver adds that many pulses to the counter.                    *
 * This is the easiest possible "hardware simulation".                */
static ssize_t smart_meter_write(struct file *file,
                                 const char __user *user_buf,
                                 size_t count,
                                 loff_t *ppos)
{
    char temp[32];
    unsigned long pulses;
    int pulses_to_add;

    if (count == 0)
        return 0;
    if (count > sizeof(temp))
        count = sizeof(temp);

    /* Copy the written text from user memory to kernel memory */
    if (copy_from_user(temp, user_buf, count) != 0)
        return -EFAULT;

    temp[count] = '\0';          /* make it a normal C string */

    pulses_to_add = simple_strtol(temp, NULL, 10);

    mutex_lock(&smart_meter_lock);
    pulse_counter += pulses_to_add;
    pulses = pulse_counter;
    mutex_unlock(&smart_meter_lock);

    pr_info("smart_meter: pulse received, total pulses = %lu\n", pulses);

    return count;
}

/* ---- Function 5: IOCTL -> special commands ----------------------- *
 *  GET_PULSES : copy pulse count back to the user program            *
 *  RESET      : set pulse count to 0                                 *
 *  ADD_PULSE  : add 1 pulse (shortcut)                                */
static long smart_meter_ioctl(struct file *file,
                              unsigned int command,
                              unsigned long argument)
{
    unsigned long value = 0;
    int result;

    switch (command) {
    case SM_IOCTL_GET_PULSES:
        mutex_lock(&smart_meter_lock);
        value = pulse_counter;
        mutex_unlock(&smart_meter_lock);

        /* Send the value back to user space memory */
        result = copy_to_user((void __user *)argument, &value, sizeof(value));
        if (result != 0)
            return -EFAULT;
        pr_info("smart_meter: ioctl GET_PULSES = %lu\n", value);
        return 0;

    case SM_IOCTL_RESET:
        mutex_lock(&smart_meter_lock);
        pulse_counter = 0;
        mutex_unlock(&smart_meter_lock);
        pr_info("smart_meter: ioctl RESET done, counter is 0 now\n");
        return 0;

    case SM_IOCTL_ADD_PULSE:
        mutex_lock(&smart_meter_lock);
        pulse_counter = pulse_counter + 1;
        value = pulse_counter;
        mutex_unlock(&smart_meter_lock);
        pr_info("smart_meter: ioctl ADD_PULSE, total = %lu\n", value);
        return 0;

    default:
        pr_err("smart_meter: unknown ioctl command\n");
        return -EINVAL;   /* -EINVAL = invalid argument */
    }
}

/* ---- Table that tells Linux which function to call for what ------ */
static const struct file_operations smart_meter_fops = {
    .owner   = THIS_MODULE,
    .open    = smart_meter_open,
    .release = smart_meter_release,
    .read    = smart_meter_read,
    .write   = smart_meter_write,
    .unlocked_ioctl = smart_meter_ioctl,
    .compat_ioctl   = smart_meter_ioctl,   /* for 32-bit programs */
};

/* ---- Misc device info: this makes the name /dev/smart_meter ------ */
static struct miscdevice smart_meter_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "smart_meter",
    .fops  = &smart_meter_fops,
    .mode  = 0666,        /* read/write allowed for normal users */
};

/* ---- Called when we run:  insmod smart_meter.ko ------------------ */
static int __init smart_meter_init(void)
{
    int result;

    pr_info("smart_meter: driver loading...\n");

    result = misc_register(&smart_meter_device);
    if (result < 0) {
        pr_err("smart_meter: cannot register device (error %d)\n", result);
        return result;
    }

    pulse_counter = 0;      /* start fresh */
    pr_info("smart_meter: driver loaded, /dev/smart_meter is ready\n");
    return 0;
}

/* ---- Called when we run:  rmmod smart_meter --------------------- */
static void __exit smart_meter_exit(void)
{
    pr_info("smart_meter: final pulse count = %lu\n", pulse_counter);
    misc_deregister(&smart_meter_device);
    pr_info("smart_meter: driver removed\n");
}

/* Register / unregister the module */
module_init(smart_meter_init);
module_exit(smart_meter_exit);

/* Information shown by  modinfo  */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smart Energy Capstone Student");
MODULE_DESCRIPTION("Simple smart meter pulse counter kernel driver");
MODULE_VERSION("1.0");