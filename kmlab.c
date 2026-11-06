#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "kmlab_given.h"
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

static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

static const struct file_operations file_operations = {
   .owner = THIS_MODULE;
   .read = device_read;
   .write = device_write;
};



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
   // Insert your code here ...
   proc_remove(status_entry);
   proc_remove(directory);

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);

