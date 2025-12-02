#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "e.h"

#define DEVICE_NAME "/dev/tsune"


int main(void) {
    info("kmemcache-1024");

    int fd = SYSCHK(open(DEVICE_NAME, O_RDWR));
    hl(fd);


}

/*
    -serial tcp:127.0.0.1:9999,server,nowait \
    -gdb tcp::12345 \
*/
