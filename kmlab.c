#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/init.h>
#include "kmlab_given.h"
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>


// Include headers as needed ...


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanchez"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 KM PA");

#define DEBUG 1
#define PROCFS_MAX_SIZE 32

// store per process data (you may modify it as needed)
typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

// Global variables as needed ...
static struct proc_dir_entry *directory;
static struct proc_dir_entry *status_entry;

static LIST_HEAD(process_list);
static DEFINE_SPINLOCK(process_lock);
static struct timer_list my_timer;
static void work_callback(struct work_struct *work);
static DECLARE_WORK(my_work, work_callback);
static bool shutting_down = false;

// adds an entry to the linked list
int add_entry(int pid){

    proc_list *new_entry;
    proc_list *entry;

    // Spinlock
    unsigned long flags;
   
    // allocates enough memory for a nodel
    new_entry = kmalloc((sizeof(*new_entry)), GFP_KERNEL);
    if (!new_entry){
       pr_info("Memory allocation failed\n");

       return -ENOMEM;
    }
    new_entry->pid = pid;
    new_entry->cpu_time = 0;
   
    spin_lock_irqsave(&process_lock, flags);

    // Prevent duplicate registration
    list_for_each_entry(entry, &process_list, list) {
        if (entry->pid == pid) {
            spin_unlock_irqrestore(&process_lock, flags);
            kfree(new_entry);
            return -EEXIST;
        }
    }

    list_add_tail(&(new_entry->list), &process_list);
    // spin lock released
    spin_unlock_irqrestore(&process_lock, flags);
    return 0;
   
}
// read function
static ssize_t device_read(struct file *file_pointer, char __user *buffer,
                           size_t buffer_length, loff_t *offset)
{
    proc_list *entry;
    char *output;
    size_t output_size = PAGE_SIZE;
    size_t len = 0;
    ssize_t ret;
    unsigned long flags;

    output = kmalloc(output_size, GFP_KERNEL);

    if (!output) {
        return -ENOMEM;
    }

    // Protect linked list while reading
    spin_lock_irqsave(&process_lock, flags);

    // Traverse registered processes
    list_for_each_entry(entry, &process_list, list) {

        if (len >= output_size - 1) {
            break;
        }

        len += scnprintf(output + len,
                         output_size - len,
                         "%u: %lu\n",
                         entry->pid,
                         entry->cpu_time);
    }

    spin_unlock_irqrestore(&process_lock, flags);

    // Copy data to user space
    ret = simple_read_from_buffer(buffer,
                                  buffer_length,
                                  offset,
                                  output,
                                  len);

    kfree(output);

    return ret;
}
// write function
static ssize_t device_write(struct file *file, const char __user *buff,
                            size_t len, loff_t *off)
{
    char procfs_buffer[PROCFS_MAX_SIZE];
    size_t procfs_buffer_size = len;
    int pid;
    int result;

    if (len == 0) {
        return 0;
    }

    // Check buffer size
    if (procfs_buffer_size >= PROCFS_MAX_SIZE) {
        return -EINVAL;
    }

    // Clear internal buffer
    memset(procfs_buffer, 0, sizeof(procfs_buffer));

    // Copy data from user space
    if (copy_from_user(procfs_buffer, buff, procfs_buffer_size)) {
        return -EFAULT;
    }

    // Add null terminator
    procfs_buffer[procfs_buffer_size] = '\0';

    // Convert string to integer PID
    result = kstrtoint(procfs_buffer, 10, &pid);

    if (result != 0) {
        return -EINVAL;
    }

    // Validate PID
    if (pid <= 0) {
        return -EINVAL;
    }

    // Register PID
    result = add_entry(pid);

    if (result != 0) {
        return result;
    }

    // Update file offset
    *off += procfs_buffer_size;

    pr_info("Registered PID: %d\n", pid);

    return procfs_buffer_size;
}

static const struct file_operations file_operations = {
   .owner = THIS_MODULE,
   .read = device_read,
   .write = device_write,
};



void traverse_list(void){
   proc_list *entry;
   // Spinlock
   unsigned long flags;
   spin_lock_irqsave(&process_lock, flags);

   list_for_each_entry(entry, &process_list, list){
      pr_info("PID: %u, CPU TIME: %lu\n", entry->pid, entry->cpu_time);
   }
   spin_unlock_irqrestore(&process_lock, flags);
}
void free_list(void){
   proc_list *entry;
   proc_list *next;
   // Spinlock
   unsigned long flags;
   spin_lock_irqsave(&process_lock, flags);

   list_for_each_entry_safe(entry, next, &process_list, list){
      list_del(&entry->list);
      kfree(entry);
   }
   spin_unlock_irqrestore(&process_lock, flags);

}
static void timer_callback(struct timer_list *timer)
{
    if (READ_ONCE(shutting_down))
        return;

    pr_info("Timer executed!\n");

    schedule_work(&my_work);

    if (!READ_ONCE(shutting_down))
        mod_timer(&my_timer, jiffies + 5 * HZ);
}

static void work_callback(struct work_struct *work)
{
    proc_list *entry;
    proc_list *next;
    unsigned long flags;
    unsigned long cpu_value;
    int result;

    // Lock the linked list
    spin_lock_irqsave(&process_lock, flags);

    // Traverse every registered process
    list_for_each_entry_safe(entry, next, &process_list, list) {

        // Get CPU time for this PID
        result = get_cpu_use(entry->pid, &cpu_value);

        if (result == 0) {

            // Update CPU time
            entry->cpu_time = cpu_value;

            pr_info("PID: %u, CPU Time: %lu\n",
                    entry->pid, entry->cpu_time);
        }
        else {

            // Process terminated, remove it
            pr_info("Removing terminated PID: %u\n", entry->pid);

            list_del(&entry->list);
            kfree(entry);
        }
    }

    // Unlock the linked list
    spin_unlock_irqrestore(&process_lock, flags);
}



// kmlab_init - Called when the module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   
   
   pr_info("Hello World!\n");
   shutting_down = false;
   

   directory = proc_mkdir("kmlab", NULL);
   if (directory == NULL) {
      pr_err("Failed to create /proc/kmlab directory\n");
      return -ENOMEM;
   }
   status_entry = proc_create("status",0666, directory, &file_operations);
   if (status_entry == NULL){
      pr_err("Failed to create /proc/kmlab/status entry\n");
      proc_remove(directory);
      return -ENOMEM;
   }

   timer_setup(&my_timer, timer_callback, 0);
   mod_timer(&my_timer, jiffies + 5 * HZ);


   
   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when the module is unloaded
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif

   WRITE_ONCE(shutting_down, true);
   del_timer_sync(&my_timer);
   cancel_work_sync(&my_work);
   free_list();
   // Insert your code here ...
   proc_remove(status_entry);
   proc_remove(directory);

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
