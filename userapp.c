#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>    // time()
#include <unistd.h>  // getpid()
// Additional headers as needed


// Register process with kernel module
void register_process(unsigned int pid)
{
    FILE *file = fopen("/proc/kmlab/status", "w");

    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(file, "%u\n", pid);

    if (fclose(file) != 0) {
        perror("fclose");
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int __expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time has expired
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            break;
        }
    }

	return 0;
}

