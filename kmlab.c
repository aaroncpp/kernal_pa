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


// Include headers as needed ...


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sanchez"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 KM PA");

#define DEBUG 1

// store per process data (you may modify it as needed)
typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

// Global variables as needed ...
static struct proc_dir_entry *directory;
static struct proc_dir_entry *status_entry;

static DEFINE_SPINLOCK(process_lock);

static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t count, loff_t *offset)
{
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t count, loff_t *offset)
{
    return -EINVAL;
}

static const struct file_operations file_operations = {
   .owner = THIS_MODULE,
   .read = device_read,
   .write = device_write,
};

static LIST_HEAD(process_list);

int add_entry(int pid){
   

   proc_list *new_entry = kmalloc((sizeof(*new_entry)), GFP_KERNEL);
   if (!new_entry){
      pr_info("Memory allocation failed\n");

      return -ENOMEM;
   }
   new_entry->pid = pid;
   new_entry->cpu_time = 0;
   
   // Spinlock
   unsigned long flags;
   spin_lock_irqsave(&process_lock, flags);

   list_add_tail(&(new_entry->list), &process_list);
   // spin lock released
   spin_unlock_irqrestore(&process_lock, flags);
   return 0;
   
}
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



// kmlab_init - Called when the module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   // Insert your code here ...
   
   pr_info("Hello World!\n");
   
   

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


   
   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when the module is unloaded
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif
   free_list();
   // Insert your code here ...
   proc_remove(status_entry);
   proc_remove(directory);

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);

