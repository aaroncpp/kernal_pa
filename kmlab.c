#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "kmlab_given.h"
// Include headers as needed ...


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cougs"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 KM PA");

#define DEBUG 1

// store per process data (you may modify it as needed)
typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

// Global variables as needed ...


// kmlab_init - Called when the module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   // Insert your code here ...
   
   
   
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
   
   

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
