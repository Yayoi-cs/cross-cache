#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "e.h"

#define DEVICE_NAME "/dev/tsune"
#define IOCTL_CMD_POC 0x810

/*
~ # cat  /sys/kernel/slab/tsune_cache/object_size 
256
~ # cat  /sys/kernel/slab/tsune_cache/objs_per_slab 
16
~ # cat  /sys/kernel/slab/tsune_cache/cpu_partial 
52
*/
struct user_req {
    unsigned int cpu_partial;
    unsigned int objs_per_slab;
    unsigned int object_size;
};

int main(void) {
    info("cross-cache poc");

    int fd = SYSCHK(open(DEVICE_NAME, O_RDWR));
    hl(fd);

    struct user_req req;
    req.cpu_partial = 256;
    req.objs_per_slab = 16;
    req.object_size = 52;
    SYSCHK(ioctl(fd, IOCTL_CMD_POC, &req));
}

/*
    -serial tcp:127.0.0.1:9999,server,nowait \
    -gdb tcp::12345 \
*/
